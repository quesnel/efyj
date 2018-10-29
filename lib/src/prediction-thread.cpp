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

#include <EASTL/chrono.h>
#include <EASTL/iterator.h>
#include <EASTL/map.h>

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
parallel_prediction_worker(eastl::shared_ptr<context> context,
                           const Model& model,
                           const Options& options,
                           const unsigned int thread_id,
                           const unsigned int thread_number,
                           const bool& stop,
                           Results& results)
{
    eastl::vector<int> m_globalsimulated(options.observed.size());
    eastl::vector<int> m_simulated(options.observed.size());
    eastl::vector<eastl::vector<scale_id>> m_globalfunctions, m_functions;
    eastl::vector<eastl::tuple<int, int, int>> m_globalupdaters, m_updaters;

    for_each_model_solver solver(context, model);
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
        while (not isend) {
            if (stop)
                return;

            eastl::fill(
              m_globalsimulated.begin(), m_globalsimulated.end(), 0.);

            for (size_t opt = 0, endopt = options.size(); opt != endopt;
                 ++opt) {
                double kappa = 0.;

                solver.init_next_value();

                do {
                    eastl::fill(m_simulated.begin(), m_simulated.end(), 0.);

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
                m_globalsimulated[opt] =
                  solver.solve(options.options.row(opt));
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
                    results.push(step, m_kappa, m_loop, m_globalupdaters);
                    isend = true;
                    break;
                }
            }
        }

        step++;
    }
}

prediction_thread_evaluator::prediction_thread_evaluator(
  eastl::shared_ptr<context> context,
  const Model& model,
  const Options& options)
  : m_context(context)
  , m_model(model)
  , m_options(options)
  , m_globalsimulated(options.observed.size(), 0)
  , simulated(options.options.rows())
  , observed(options.options.rows())
  , solver(context, model)
  , kappa_c(model.attributes[0].scale.size())
{
    if (not options.have_subdataset())
        throw solver_error(
          "options does not have enough data to build the training set");
}

eastl::vector<result>
prediction_thread_evaluator::run(int line_limit,
                                 double time_limit,
                                 int reduce_mode,
                                 unsigned int threads)
{
    (void)time_limit;

    info(m_context, "[Computation starts with %u thread(s)]\n", threads);

    Results results(m_context, threads);
    bool stop = false;

    eastl::vector<std::thread> workers{ threads };

    for (auto i = 0u; i != threads; ++i) {
        auto newctx = copy_context(m_context);

        workers[i] = std::thread(parallel_prediction_worker,
                                 newctx,
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

    return {};
}

Results::Results(eastl::shared_ptr<context> context, unsigned int threads)
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

void
Results::emplace_result(
  int i,
  double kappa,
  unsigned long loop,
  const eastl::vector<eastl::tuple<int, int, int>>& updaters)
{
    assert(static_cast<size_t>(i) < m_level.size() and
           static_cast<size_t>(i) < m_results.size());

    m_results[i].kappa = kappa;
    m_results[i].loop += loop;
    m_results[i].updaters = updaters;
}

void
Results::push(int step,
              double kappa,
              unsigned long loop,
              const eastl::vector<eastl::tuple<int, int, int>>& updaters)
{
    std::lock_guard<std::mutex> locker(m_container_mutex);

    assert(step >= 1);

    if (static_cast<size_t>(step - 1) >= m_results.size()) {
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

    info(m_context,
         "| {} | {:13.10f} | {} | {} |",
         step,
         m_results[step - 1].kappa,
         m_results[step - 1].loop,
         duration);

    print(m_context, m_results[step - 1].updaters);
    info(m_context, "\n");
}

} // namespace efyj
