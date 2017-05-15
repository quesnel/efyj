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

#ifndef ORG_VLEPROJECT_EFYJ_DETAILS_PREDICTION_THREAD_HPP
#define ORG_VLEPROJECT_EFYJ_DETAILS_PREDICTION_THREAD_HPP

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#endif

#include <chrono>
#include <fstream>
#include <iterator>
#include <mutex>
#include <thread>

#include <chrono>
#include <efyj/details/model.hpp>
#include <efyj/details/options.hpp>
#include <efyj/details/post.hpp>
#include <efyj/details/private.hpp>
#include <efyj/details/solver-stack.hpp>
#include <map>

namespace efyj {

struct prediction_thread_evaluator
{
    std::shared_ptr<context> m_context;
    const Model& m_model;
    const Options& m_options;

    std::chrono::time_point<std::chrono::system_clock> m_start, m_end;
    std::vector<int> m_globalsimulated;
    std::vector<std::tuple<int, int, int>> m_updaters;
    std::vector<std::vector<int>> m_globalfunctions, m_functions;
    std::vector<int> simulated;
    std::vector<int> observed;
    for_each_model_solver solver;
    weighted_kappa_calculator kappa_c;
    unsigned long long int m_loop = 0;

    prediction_thread_evaluator(std::shared_ptr<context> context,
                                const Model& model,
                                const Options& options);

    std::vector<result> run(int line_limit,
                            double time_limit,
                            int reduce_mode,
                            unsigned int threads);
};

class Results
{
    std::shared_ptr<context> m_context;
    std::mutex m_container_mutex;

    struct Result
    {
        double kappa;
        unsigned long loop;
        std::vector<std::tuple<int, int, int>> updaters;
    };

    std::vector<Result> m_results;
    std::vector<int> m_level;
    const unsigned int m_threads;
    std::chrono::time_point<std::chrono::system_clock> m_start, m_end;

public:
    Results(std::shared_ptr<context> context, unsigned int threads)
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
                        const std::vector<std::tuple<int, int, int>>& updaters)
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
              const std::vector<std::tuple<int, int, int>>& updaters)
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

        vInfo(m_context,
              "| %d | %13.10f | %lu | %f |",
              step,
              m_results[step - 1].kappa,
              m_results[step - 1].loop,
              duration);

        print(m_context, m_results[step - 1].updaters);
        vInfo(m_context, "\n");
    }
};

template<typename Solver>
bool
init_worker(Solver& solver, const int thread_id)
{
    for (auto i = 0; i < thread_id; ++i)
        if (solver.next_line() == false)
            return false;

    return true;
}

void
parallel_prediction_worker(std::shared_ptr<context> context,
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

    for_each_model_solver solver(context, model);
    weighted_kappa_calculator kappa_c(model.attributes[0].scale.size());
    solver.reduce(options);

    std::size_t step = 1;
    std::size_t max_step = solver.get_attribute_line_tuple_limit();
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

            std::fill(m_globalsimulated.begin(), m_globalsimulated.end(), 0.);

            for (std::size_t opt = 0, endopt = options.size(); opt != endopt;
                 ++opt) {
                double kappa = 0.;

                solver.init_next_value();

                do {
                    std::fill(m_simulated.begin(), m_simulated.end(), 0.);

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

inline prediction_thread_evaluator::prediction_thread_evaluator(
  std::shared_ptr<context> context,
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

class my_logger : public logger
{
    FILE* m_stream;

public:
    my_logger(unsigned int id) noexcept
      : m_stream(fopen(make_new_name("", id).c_str(), "w"))
    {}

    bool is_open() const noexcept
    {
        return m_stream != nullptr;
    }

    void write(int priority,
               const char* file,
               int line,
               const char* fn,
               const char* format,
               va_list args) noexcept final
    {
        FILE* stream = m_stream;
        if (not is_open())
            stream = stdout;

        fprintf(stream, "%s (%d) in %s:\n", file, line, fn);
        vfprintf(stream, format, args);
        fprintf(stream, "\n");
    }

    void write(message_type /*m*/,
               const char* format,
               va_list args) noexcept final
    {
        FILE* stream = m_stream;
        if (not is_open())
            stream = stdout;

        vfprintf(stream, format, args);
        fprintf(stream, "\n");
    }
};

inline std::vector<result>
prediction_thread_evaluator::run(int line_limit,
                                 double time_limit,
                                 int reduce_mode,
                                 unsigned int threads)
{
    (void)time_limit;

    vInfo(m_context, "[Computation starts with %u thread(s)]\n", threads);

    Results results(m_context, threads);
    bool stop = false;

    std::vector<std::thread> workers{ threads };

    for (auto i = 0u; i != threads; ++i) {
        auto newctx = std::make_shared<efyj::context>();
        newctx->set_logger(std::make_unique<my_logger>(i));

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

} // namespace efyj

#endif
