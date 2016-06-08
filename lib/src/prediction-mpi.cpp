/* Copyright (C) 2015-2016 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "prediction.hpp"
#include "problem.hpp"
#include "solver-stack.hpp"
#include "model.hpp"
#include "context.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include "options.hpp"
#include "post.hpp"
#include <iterator>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <mpi.h>

namespace efyj
{

void pack_data(long long int loop,
               double kappa,
               int step,
               const std::vector<std::tuple<int, int, int>> &updaters)
{
    std::vector<int> linearupdaters;
    linearupdaters.reserve(updaters.size() * 3);

    std::vector<char> buffer(2 * (sizeof(loop) + sizeof(kappa) + sizeof(step) +
                                  sizeof(int) * 3 * updaters.size()));
    int pos = 0;

    MPI_Pack(&loop,
             1,
             MPI_LONG_LONG_INT,
             buffer.data(),
             buffer.size(),
             &pos,
             MPI_COMM_WORLD);

    MPI_Pack(&kappa,
             1,
             MPI_DOUBLE,
             buffer.data(),
             buffer.size(),
             &pos,
             MPI_COMM_WORLD);

    MPI_Pack(
        &step, 1, MPI_INT, buffer.data(), buffer.size(), &pos, MPI_COMM_WORLD);

    for (const auto &updater : updaters) {
        linearupdaters.push_back(std::get<0>(updater));
        linearupdaters.push_back(std::get<1>(updater));
        linearupdaters.push_back(std::get<2>(updater));
    }

    MPI_Pack(linearupdaters.data(),
             linearupdaters.size(),
             MPI_INT,
             buffer.data(),
             buffer.size(),
             &pos,
             MPI_COMM_WORLD);

    MPI_Send(buffer.data(), buffer.size(), MPI_PACKED, 0, 0, MPI_COMM_WORLD);
}

void unpack_data(long long int *loop,
                 double *kappa,
                 int *step,
                 std::vector<std::tuple<int, int, int>> *updaters,
                 int source,
                 int tag)
{
    MPI_Status status;
    std::vector<char> buffer(8192);

    MPI_Recv(buffer.data(),
             buffer.size(),
             MPI_PACKED,
             source,
             tag,
             MPI_COMM_WORLD,
             &status);

    int pos = 0;

    MPI_Unpack(buffer.data(),
               buffer.size(),
               &pos,
               loop,
               1,
               MPI_LONG_LONG_INT,
               MPI_COMM_WORLD);
    MPI_Unpack(buffer.data(),
               buffer.size(),
               &pos,
               kappa,
               1,
               MPI_DOUBLE,
               MPI_COMM_WORLD);
    MPI_Unpack(
        buffer.data(), buffer.size(), &pos, step, 1, MPI_INT, MPI_COMM_WORLD);

    std::vector<int> linearupdaters(*step * 3);

    MPI_Unpack(buffer.data(),
               buffer.size(),
               &pos,
               linearupdaters.data(),
               linearupdaters.size(),
               MPI_INT,
               MPI_COMM_WORLD);

    updaters->resize(*step);
    for (std::size_t i = 0, e = linearupdaters.size(); i != e; i += 3) {
        std::get<0>((*updaters)[i / 3]) = linearupdaters[i];
        std::get<1>((*updaters)[i / 3]) = linearupdaters[i + 1];
        std::get<2>((*updaters)[i / 3]) = linearupdaters[i + 2];
    }
}

class Results
{
    std::shared_ptr<Context> m_context;
    std::mutex m_container_mutex;

    struct Result {
        double kappa;
        unsigned long loop;
        std::vector<std::tuple<int, int, int>> updaters;
    };

    std::vector<Result> m_results;
    std::vector<int> m_level;
    const unsigned int m_threads;
    std::chrono::time_point<std::chrono::system_clock> m_start, m_end;

public:
    Results(std::shared_ptr<Context> context, unsigned int threads)
        : m_context(context)
        , m_threads(threads)
        , m_start(std::chrono::system_clock::now())
    {
        m_results.reserve(32);
        m_results.resize(1u);

        m_level.reserve(32);
        m_level.resize(1u);
        m_level[0] = threads;
    }

    void emplace_result(int i,
                        double kappa,
                        unsigned long loop,
                        const std::vector<std::tuple<int, int, int>> &updaters)
    {
        assert(static_cast<std::size_t>(i) < m_level.size() and
               static_cast<std::size_t>(i) < m_results.size());

        m_results[i].kappa = kappa;
        m_results[i].loop += loop;
        m_results[i].updaters = updaters;
    }

    void push(int step,
              double kappa,
              unsigned long loop,
              const std::vector<std::tuple<int, int, int>> &updaters)
    {
        std::lock_guard<std::mutex> locker(m_container_mutex);

        assert(step >= 1);

        if (static_cast<std::size_t>(step - 1) >= m_results.size()) {
            /* A new level (threshold) is reached we append new elements
               to the level and results vectors. */
            m_level.emplace_back(static_cast<int>(m_threads - 1));
            m_results.emplace_back();

            emplace_result(m_results.size() - 1, kappa, loop, updaters);
        } else {
            if (m_results[step - 1].kappa < kappa)
                emplace_result(step - 1, kappa, loop, updaters);
            else
                m_results[step - 1].loop += loop;
        }

        m_end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration<double>(m_end - m_start).count();

        m_context->info().printf("| %d | %13.10f | %" PRIuMAX " | %f | ",
                                 step,
                                 m_results[step - 1].kappa,
                                 m_results[step - 1].loop,
                                 duration);

        m_context->info() << m_results[step - 1].updaters << "|\n";
    }
};

template <typename Solver>
bool init_worker(Solver &solver, const int thread_id)
{
    for (auto i = 0; i < thread_id; ++i)
        if (solver.next_line() == false)
            return false;

    return true;
}

void parallel_mpi_prediction_worker(std::shared_ptr<Context> context,
                                    const Model &model,
                                    const Options &options,
                                    const int thread_id,
                                    const int thread_number,
                                    bool &stop)
{
    std::vector<int> m_globalsimulated(options.observated.size());
    std::vector<int> m_simulated(options.observated.size());
    std::vector<std::vector<scale_id>> m_globalfunctions, m_functions;
    std::vector<std::tuple<int, int, int>> m_globalupdaters, m_updaters;

    for_each_model_solver solver(context, model);
    weighted_kappa_calculator kappa_c(options.options.rows(),
                                      model.attributes[0].scale.size());
    solver.reduce(options);

    std::size_t max_step = solver.get_attribute_line_tuple_limit();
    std::size_t step = 1;
    unsigned long m_loop = 0;
    double m_kappa = 0.0;

    while (step < max_step) {
        m_kappa = 0;
        m_globalupdaters.clear();

        solver.init_walkers(step);

        if (init_worker(solver, thread_id) == false) {
            ++step;
            continue;
        }

        bool isend = false;
        while (not isend) {
            std::fill(m_globalsimulated.begin(), m_globalsimulated.end(), 0.);

            for (std::size_t opt = 0, endopt = options.ordered.size();
                 opt != endopt;
                 ++opt) {
                double kappa = 0.;

                solver.init_next_value();

                do {
                    std::fill(m_simulated.begin(), m_simulated.end(), 0.);

                    for (auto x : options.ordered[opt])
                        m_simulated[x] = solver.solve(options.options.row(x));

                    auto ret =
                        kappa_c.squared(options.observated, m_simulated);
                    m_loop++;

                    if (ret > kappa) {
                        solver.get_functions(m_functions);
                        m_updaters = solver.updaters();
                        kappa = ret;
                    }
                } while (solver.next_value() == true);

                solver.set_functions(m_functions);
                m_globalsimulated[opt] =
                    solver.solve(options.options.row(opt));
            }

            auto ret = kappa_c.squared(options.observated, m_globalsimulated);
            m_loop++;

            if (ret > m_kappa) {
                m_kappa = ret;
                m_globalupdaters = solver.updaters();
                m_globalfunctions = m_functions;
            }

            for (int i = 0; i < thread_number; ++i) {
                if (solver.next_line() == false) {
                    pack_data(m_loop, m_kappa, step, m_globalupdaters);
                    isend = true;
                    break;
                }
            }
        }
        step++;
    }

    stop = true;
}

void synchronize_result(std::shared_ptr<Context> context,
                        int /*rank*/,
                        int world,
                        bool &stop)
{
    Results results(context, world);

    for (;;) {
        MPI_Status status;
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        long long int loop;
        double kappa;
        int step;
        std::vector<std::tuple<int, int, int>> updaters;

        unpack_data(&loop,
                    &kappa,
                    &step,
                    &updaters,
                    status.MPI_SOURCE,
                    status.MPI_TAG);

        context->info().printf("Receives results from %d\n",
                               status.MPI_SOURCE);

        results.push(step, kappa, loop, updaters);

        if (stop)
            break;

        // is end of the simulation, stop all
        // if (stop == true) {
        //     bm::broadcast(world, 0, 0);

        //     // wait for all process and break
        //     return;
        // }
    }
}

void prediction_mpi(std::shared_ptr<Context> context,
                    const Model &model,
                    const Options &options,
                    int rank,
                    int world)
{
    bool stop = false;

    /* The first rank processor is use to synchronize results from other
     * processor. At first, it is an expensive operation but insignifiant
     * with large number of updaters.
     */
    if (rank == 0) {
        context->info() << context->info().cyanb()
                        << "[Computation starts with " << world
                        << " processors]" << context->info().def() << '\n';

        std::thread sync(
            synchronize_result, context, rank, world, std::ref(stop));
        sync.detach();
    }

    // TODO: default, we use standard output for MPI processes ?
    //
    // auto filepath = make_new_name(context->get_log_filename(), rank);
    // auto new_ctx = std::make_shared<efyj::Context>(context->log_priority());
    // auto ret = new_ctx->set_log_file_stream(filepath);
    // if (not ret)
    //     context->err().printf("Failed to assign '%s' to thread %d. Switch "
    //                           "to console.\n", filepath.c_str(), rank);

    parallel_mpi_prediction_worker(context, model, options, rank, world, stop);
}

} // namespace efyj
