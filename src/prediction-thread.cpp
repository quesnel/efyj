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

namespace efyj {

class Results
{
    std::shared_ptr<Context> m_context;
    std::mutex m_container_mutex;

    struct Result {
        double kappa;
        unsigned long loop;
        std::vector <std::tuple<int, int, int>> updaters;
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

    void emplace_result(int i, double kappa,
                        unsigned long loop,
                        const std::vector <std::tuple<int, int, int>>& updaters)
    {
        assert(static_cast<std::size_t>(i) < m_level.size() and
               static_cast<std::size_t>(i) < m_results.size());

        m_results[i].kappa = kappa;
        m_results[i].loop += loop;
        m_results[i].updaters = updaters;
    }

    void push(int step, double kappa, unsigned long loop,
              const std::vector <std::tuple<int, int, int>>& updaters)
    {
        std::lock_guard <std::mutex> locker(m_container_mutex);

        assert(step >= 1);

        if (static_cast<std::size_t>(step - 1) >= m_results.size()) {
            /* A new level (threshold) is reached we append new elements
               to the level and results vectors. */
            m_level.emplace_back(static_cast<int>(m_threads - 1));
            m_results.emplace_back();

            emplace_result(m_results.size() - 1, kappa, loop, updaters);
            // m_level[m_results.size()]--;
        } else {
            if (m_results[step - 1].kappa < kappa)
                emplace_result(step - 1, kappa, loop, updaters);
            else
                m_results[step - 1].loop += loop;

            // m_level[step - 1]--;
        }

//        if (m_level[step - 1] <= 0) {
            m_end = std::chrono::system_clock::now();
            auto duration = std::chrono::duration<double>(m_end - m_start).count();

            m_context->info().printf("| %d | %13.10f | %" PRIuMAX
                                     " | %f | ",
                                     step,
                                     m_results[step - 1].kappa,
                                     m_results[step - 1].loop,
                                     duration);

            m_context->info() << m_results[step - 1].updaters << '\n';
//        }
    }
};

void parallel_prediction_worker(std::shared_ptr<Context> context,
                                const Model& model,
                                const Options& options,
                                const unsigned int thread_id,
                                const unsigned int thread_number,
                                const bool& stop,
                                Results& results)
{
    std::vector <int> m_globalsimulated(options.observated.size());
    std::vector <int> m_simulated(options.observated.size());
    std::vector <std::vector<scale_id>> m_globalfunctions, m_functions;
    std::vector <std::tuple<int, int, int>> m_globalupdaters, m_updaters;
    for_each_model_solver solver(context, model);
    std::size_t step = 1;
    unsigned long m_loop = 0;

    double m_kappa = 0.0;

    solver.reduce(options);

    while (step < std::numeric_limits<std::size_t>::max()) {
        m_kappa = 0;
        m_globalupdaters.clear();

        solver.init_walkers(step);

        for (unsigned int i = 0; i < thread_id; ++i) {
            if (solver.next_line() == false) {
                step++;
                continue;
            }
        }

        bool isend = false;
        while (not isend) {
            if (stop)
                return;

            std::fill(m_globalsimulated.begin(), m_globalsimulated.end(), 0.);

            for (std::size_t opt = 0, endopt = options.ordered.size();
                 opt != endopt; ++opt) {
                double kappa = 0.;

                solver.init_next_value();

                do {
                    std::fill(m_simulated.begin(), m_simulated.end(), 0.);

                    for (auto x : options.ordered[opt])
                        m_simulated[x] = solver.solve(
                            options.options.row(x));

                    auto ret = squared_weighted_kappa(
                        options.observated,
                        m_simulated,
                        options.options.rows(),
                        model.attributes[0].scale.size());

                    m_loop++;

                    if (ret > kappa) {
                        solver.get_functions(m_functions);
                        m_updaters = solver.updaters();
                        kappa = ret;
                    }
                } while (solver.next_value() == true);

                solver.set_functions(m_functions);
                m_globalsimulated[opt] = solver.solve(
                    options.options.row(opt));
            }

            // We need to send results here.

            auto ret = squared_weighted_kappa(
                options.observated,
                m_globalsimulated,
                options.options.rows(),
                model.attributes[0].scale.size());

            m_loop++;

            if (ret > m_kappa) {
                m_kappa = ret;
                m_globalupdaters = solver.updaters();
                m_globalfunctions = m_functions;
            }

            for (unsigned int i = 0; i < thread_number; ++i) {
                if (solver.next_line() == false) {
                    results.push(step, m_kappa, m_loop, m_globalupdaters);
                    isend = true;
                    break;
                }
            }
        }

        step++;
    }
}

void prediction_n(std::shared_ptr<Context> context,
                  const Model& model,
                  const Options& options,
                  unsigned int threads)
{
    context->info() << context->info().cyanb()
                    << "[Computation starts with " << threads << " threads]"
                    << context->info().def()
                    << '\n';

    assert(threads > 1);

    Results results(context, threads);
    bool stop = false;

    std::vector<std::thread> workers { threads };

    for (auto i = 0u; i != threads; ++i) {
        auto filepath = make_new_name(context->get_log_filename(), i);
        auto new_ctx = std::make_shared<efyj::Context>(context->log_priority());
        auto ret = new_ctx->set_log_file_stream(filepath);

        if (not ret)
            context->err().printf("Failed to assign '%s' to thread %d. Switch "
                                  "to console.\n", filepath.c_str(), i);

        workers[i] = std::thread(parallel_prediction_worker,
                                 new_ctx,
                                 std::cref(model),
                                 std::cref(options),
                                 i,
                                 threads,
                                 std::cref(stop),
                                 std::ref(results));
    }

    /* Here try to read outputs of compuation from prediction workers. */
    /* TODO what use? */

    /* Stop work and wait for all prediction workers. */
    /* TODO stop = true; */

    for (auto& w : workers)
        w.join();
}

} // namespace efyj
