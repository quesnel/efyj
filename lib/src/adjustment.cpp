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

#include "efyj.hpp"
#include "adjustment.hpp"
#include "post.hpp"
#include "solver-stack.hpp"
#include "model.hpp"
#include "context.hpp"
#include "utils.hpp"
#include "options.hpp"
#include <iterator>
#include <fstream>
#include <chrono>
#include <thread>
#include <cmath>

namespace {

constexpr std::size_t max_step(int need, std::size_t real) noexcept
{
    return need <= 0 ? real : std::min(static_cast<std::size_t>(need), real);
}

}

namespace efyj {

adjustment_evaluator::adjustment_evaluator(std::shared_ptr<Context> context,
                                           const Model& model,
                                           const Options& options)
    : m_context(context)
    , m_model(model)
    , m_options(options)
    , simulated(options.options.rows())
    , observed(options.options.rows())
    , solver(context, model)
    , kappa_c(model.attributes[0].scale.size())
{
}

std::vector<result> adjustment_evaluator::run(int line_limit,
                                              double time_limit,
                                              int reduce_mode)
{
    (void)time_limit;

    std::vector<result> ret;

    m_context->info() << m_context->info().cyanb()
                      << "[Computation start]"
                      << m_context->info().def() << '\n';

    if (reduce_mode)
        solver.reduce(m_options);

    solver.get_functions(m_globalfunctions);
    assert(not m_globalfunctions.empty()
           and "adjustment can not determine function");

    const std::size_t max_step = ::max_step(
        line_limit, solver.get_attribute_line_tuple_limit());
    const std::size_t max_opt = m_options.simulations.size();

    assert(max_step > 0 and
           "adjustment: can not determine limit");

    m_context->info() << m_context->info().cyanb()
                     << "[Computation starts 1/"
                     << max_step << "]"
                     << m_context->info().def() << '\n';

    {
        m_start = std::chrono::system_clock::now();
        for (std::size_t opt = 0; opt != m_options.size(); ++opt)
            simulated[opt] = solver.solve(m_options.options.row(opt));

        auto kappa = kappa_c.squared(m_options.observed, simulated);

        m_end = std::chrono::system_clock::now();

        m_context->info() << "| line updated | kappa | kappa computed "  \
            "| time (s) | tuple (attribute, line, value) updated |\n";

        m_context->info().printf("| %d | %13.10f | %" PRIuMAX
                                " | %f | [] |\n", 0, kappa, 1,
                                std::chrono::duration<double>(
                                    m_end - m_start).count());

        ret.emplace_back();

        ret.back().kappa = kappa;
        ret.back().time = std::chrono::duration<double>(m_end - m_start).count();
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

        m_context->info().printf("| %d | %13.10f | %" PRIuMAX
                                 " | %f | ",
                                 step, kappa, loop,
                                 std::chrono::duration<double>(
                                     m_end - m_start).count());

        m_context->info() << m_updaters << " |\n";
    }

    return ret;
}

} // namespace efyj