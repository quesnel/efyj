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

#include <chrono>
#include <iterator>
#include <map>

#include "model.hpp"
#include "options.hpp"
#include "post.hpp"
#include "private.hpp"
#include "solver-stack.hpp"

#include <mutex>
#include <thread>

namespace efyj {

struct prediction_thread_evaluator
{
    const context& m_context;
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

    prediction_thread_evaluator(const context& ctx,
                                const Model& model,
                                const Options& options);

    bool is_valid() const noexcept;

    void run(const result_callback& cb,
             int line_limit,
             double time_limit,
             int reduce_mode,
             unsigned int threads);
};

class Results
{
    const context& m_context;
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
    Results(const context& ctx, unsigned int threads);

    void emplace_result(
      int i,
      double kappa,
      unsigned long loop,
      const std::vector<std::tuple<int, int, int>>& updaters);

    void push(int step,
              double kappa,
              unsigned long loop,
              const std::vector<std::tuple<int, int, int>>& updaters);
};

} // namespace efyj

#endif
