/* Copyright (C) 2016 INRA
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

#ifndef ORG_VLEPROJECT_EFYj_INTERNAL_ADJUSTMENT_HPP
#define ORG_VLEPROJECT_EFYj_INTERNAL_ADJUSTMENT_HPP

#include <chrono>
#include <efyj/details/model.hpp>
#include <efyj/details/options.hpp>
#include <efyj/details/post.hpp>
#include <efyj/details/private.hpp>
#include <efyj/details/solver-stack.hpp>

namespace efyj {

struct adjustment_evaluator {
    std::shared_ptr<context> m_context;
    const Model &m_model;
    const Options &m_options;

    std::chrono::time_point<std::chrono::system_clock> m_start, m_end;
    std::vector<std::tuple<int, int, int>> m_updaters;
    std::vector<std::vector<int>> m_globalfunctions;
    std::vector<int> simulated;
    std::vector<int> observed;
    for_each_model_solver solver;
    weighted_kappa_calculator kappa_c;
    unsigned long long int m_loop = 0;

    adjustment_evaluator(std::shared_ptr<context> context,
                         const Model &model,
                         const Options &options);

    std::vector<result>
    run(int line_limit, double time_limit, int reduce_mode);
};

inline adjustment_evaluator::adjustment_evaluator(
    std::shared_ptr<context> context,
    const Model &model,
    const Options &options)
    : m_context(context)
    , m_model(model)
    , m_options(options)
    , simulated(options.options.rows())
    , observed(options.options.rows())
    , solver(context, model)
    , kappa_c(model.attributes[0].scale.size())
{
}

inline std::vector<result>
adjustment_evaluator::run(int line_limit, double time_limit, int reduce_mode)
{
    (void)time_limit;

    std::vector<result> ret;

    vInfo(m_context, "[Computation starts]\n");

    if (reduce_mode)
        solver.reduce(m_options);

    solver.get_functions(m_globalfunctions);
    assert(not m_globalfunctions.empty() and
           "adjustment can not determine function");

    const std::size_t max_step =
        max_value(line_limit, solver.get_attribute_line_tuple_limit());
    const std::size_t max_opt = m_options.simulations.size();

    assert(max_step > 0 and "adjustment: can not determine limit");

    vInfo(m_context, "[Computation starts 1/%zu\n", max_step);

    {
        m_start = std::chrono::system_clock::now();
        for (std::size_t opt = 0; opt != m_options.size(); ++opt)
            simulated[opt] = solver.solve(m_options.options.row(opt));

        auto kappa = kappa_c.squared(m_options.observed, simulated);

        m_end = std::chrono::system_clock::now();

        vInfo(m_context,
              "| line updated | kappa | kappa computed "
              "| time (s) | tuple (attribute, line, value) updated |\n");

        vInfo(m_context,
              "| %d | %13.10f | %d | %f | [] |\n",
              0,
              kappa,
              1,
              std::chrono::duration<double>(m_end - m_start).count());

        ret.emplace_back();

        ret.back().kappa = kappa;
        ret.back().time =
            std::chrono::duration<double>(m_end - m_start).count();
        ret.back().kappa_computed = 1;
        ret.back().function_computed = m_options.size();
    }

    for (std::size_t step = 1; step <= max_step; ++step) {
        m_start = std::chrono::system_clock::now();
        long int loop = 0;

        solver.set_functions(m_globalfunctions);
        solver.init_walkers(step);
        double kappa = 0;

        do {
            solver.init_next_value();

            do {
                for (std::size_t opt = 0; opt != max_opt; ++opt) {
                    observed[opt] = m_options.observed[opt];
                    simulated[opt] = solver.solve(m_options.options.row(opt));
                }

                auto localkappa = kappa_c.squared(observed, simulated);
                loop++;

                if (localkappa > kappa) {
                    m_updaters = solver.updaters();
                    kappa = localkappa;
                }
            } while (solver.next_value() == true);
        } while (solver.next_line() == true);

        m_end = std::chrono::system_clock::now();
        auto time = std::chrono::duration<double>(m_end - m_start).count();

        ret.emplace_back();
        ret.back().kappa = kappa;
        ret.back().time = time;
        ret.back().kappa_computed = static_cast<unsigned long int>(loop);
        ret.back().function_computed = static_cast<unsigned long int>(0);

        vInfo(m_context,
              "| %zu | %13.10f | %zu | %f | ",
              step,
              kappa,
              loop,
              std::chrono::duration<double>(m_end - m_start).count());

        // TODO
        // m_context->info() << m_updaters << " |\n";
    }

    return ret;
}

} // namespace efyj

#endif
