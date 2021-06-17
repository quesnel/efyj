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

#include <chrono>
#include <iterator>
#include <map>

#include "model.hpp"
#include "options.hpp"
#include "post.hpp"
#include "prediction-thread.hpp"
#include "private.hpp"
#include "solver-stack.hpp"
#include "utils.hpp"

#include <chrono>
#include <mutex>
#include <thread>

namespace efyj {

template<typename Solver>
static bool
init_worker(Solver& solver, const int thread_id)
{
    for (auto i = 0; i < thread_id; ++i)
        if (solver.next_line() == false)
            return false;

    return true;
}

static void
parallel_prediction_worker(context& ctx,
                           const Model& model,
                           const Options& options,
                           const unsigned int thread_id,
                           const unsigned int thread_number,
                           const bool& stop,
                           Results& results)
{
    std::vector<int> m_globalsimulated(options.observed.size());
    std::vector<int> m_simulated(options.observed.size());
    std::vector<std::vector<scale_id>> m_globalfunctions, m_functions;
    std::vector<std::tuple<int, int, int>> m_globalupdaters, m_updaters;

    for_each_model_solver solver(ctx, model);
    weighted_kappa_calculator kappa_c(model.attributes[0].scale.size());
    solver.reduce(options);

    size_t step = 1;
    size_t max_step = solver.get_attribute_line_tuple_limit();
    unsigned long m_loop = 0;
    double m_kappa = 0.0;

    while (step != max_step) {
        m_kappa = 0;
        m_globalupdaters.clear();

        solver.init_walkers(step);

        if (init_worker(solver, thread_id) == false) {
            ++step;
            continue;
        }

        bool isend = false;
        while (isend == false) {
            if (stop)
                return;

            std::fill(m_globalsimulated.begin(), m_globalsimulated.end(), 0);

            for (auto opt = 0, endopt = length(options); opt != endopt; ++opt) {
                double kappa = 0.;

                solver.init_next_value();

                do {
                    std::fill(m_simulated.begin(), m_simulated.end(), 0);

                    for (auto x : options.get_subdataset(opt))
                        m_simulated[x] = solver.solve(options.options.row(x));

                    auto ret = kappa_c.squared(options.observed, m_simulated);
                    m_loop++;

                    if (ret > kappa) {
                        solver.get_functions(m_functions);
                        m_updaters = solver.updaters();
                        kappa = ret;
                    }
                } while (solver.next_value() == true);

                solver.set_functions(m_functions);
                m_globalsimulated[opt] = solver.solve(options.options.row(opt));
            }

            // We need to send results here.

            auto ret = kappa_c.squared(options.observed, m_globalsimulated);
            m_loop++;

            if (ret > m_kappa) {
                m_kappa = ret;
                m_globalupdaters = solver.updaters();
                m_globalfunctions = m_functions;
            }

            for (unsigned int i = 0; i < thread_number; ++i) {
                if (solver.next_line() == false) {
                    results.push(static_cast<int>(step),
                                 m_kappa,
                                 m_loop,
                                 m_globalupdaters);
                    isend = true;
                    break;
                }
            }
        }

        step++;
    }
}

prediction_thread_evaluator::prediction_thread_evaluator(context& ctx,
                                                         const Model& model,
                                                         const Options& options)
  : m_context(ctx)
  , m_model(model)
  , m_options(options)
  , m_globalsimulated(options.observed.size(), 0)
  , simulated(options.options.rows())
  , observed(options.options.rows())
  , solver(ctx, model)
  , kappa_c(model.attributes[0].scale.size())
{}

bool
prediction_thread_evaluator::is_valid() const noexcept
{
    return m_options.have_subdataset();
}

status
prediction_thread_evaluator::run(
  result_callback /*callback*/,
  void* /*user_data_callback*/,
  [[maybe_unused]] int line_limit,
  [[maybe_unused]] double time_limit,
  [[maybe_unused]] int reduce_mode,
  unsigned int threads,
  [[maybe_unused]] const std::string& output_directory)
{
    info(m_context, "[Computation starts with %u thread(s)]\n", threads);

    Results results(m_context, m_model, threads);
    bool stop = false;

    std::vector<std::thread> workers{ threads };

    for (auto i = 0u; i != threads; ++i) {
        workers[i] = std::thread(parallel_prediction_worker,
                                 std::ref(m_context),
                                 std::cref(m_model),
                                 std::cref(m_options),
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

    return status::success;
}

Results::Results(context& ctx, const Model& mdl, unsigned int threads)
  : m_context(ctx)
  , m_model(mdl)
  , m_threads(threads)
  , m_start(std::chrono::system_clock::now())
{
    m_results.reserve(32);
    m_results.resize(1u);

    m_level.reserve(32);
    m_level.resize(1u);
    m_level[0] = threads;
}

status
Results::init(const std::string& output_directory)
{
    if (auto ret = m_writer.init(output_directory); is_bad(ret))
        return ret;

    info(m_context, "[Output directory]\n{}\n", m_writer.directory.string());

    return status::success;
}

void
Results::emplace_result(int i,
                        double kappa,
                        unsigned long loop,
                        const std::vector<std::tuple<int, int, int>>& updaters)
{
    assert(static_cast<size_t>(i) < m_level.size() &&
           static_cast<size_t>(i) < m_results.size());

    m_results[i].kappa = kappa;
    m_results[i].loop += loop;
    m_results[i].updaters = updaters;
}

void
Results::push(int step,
              double kappa,
              unsigned long loop,
              const std::vector<std::tuple<int, int, int>>& updaters)
{
    std::lock_guard<std::mutex> locker(m_container_mutex);

    assert(step >= 1);

    if (static_cast<size_t>(step - 1) >= m_results.size()) {
        /* A new level (threshold) is reached we append new elements
           to the level and results vectors. */
        m_level.emplace_back(static_cast<int>(m_threads - 1));
        m_results.emplace_back();

        emplace_result(
          static_cast<int>(m_results.size()) - 1, kappa, loop, updaters);
    } else {
        if (m_results[step - 1].kappa < kappa)
            emplace_result(step - 1, kappa, loop, updaters);
        else
            m_results[step - 1].loop += loop;
    }

    m_end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration<double>(m_end - m_start).count();

    info(m_context,
         "| {} | {:13.10f} | {} | {} |",
         step,
         m_results[step - 1].kappa,
         m_results[step - 1].loop,
         duration);

    m_writer.store(m_context, m_model, m_results[step - 1].updaters);
    // print(m_context, m_results[step - 1].updaters);
    info(m_context, "\n");
}

} // namespace efyj
