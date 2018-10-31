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

#include "adjustment.hpp"
#include "utils.hpp"

namespace efyj {

adjustment_evaluator::adjustment_evaluator(eastl::shared_ptr<context> context,
                                           const Model& model,
                                           const Options& options)
  : m_context(context)
  , m_model(model)
  , m_options(options)
  , simulated(options.options.rows())
  , observed(options.options.rows())
  , solver(context, model)
  , kappa_c(model.attributes[0].scale.size())
{}

eastl::vector<result>
adjustment_evaluator::run(int line_limit, double time_limit, int reduce_mode)
{
    (void)time_limit;

    eastl::vector<result> ret;

    info(m_context, "[Computation starts]\n");

    if (reduce_mode)
        solver.reduce(m_options);

    solver.get_functions(m_globalfunctions);
    assert(!m_globalfunctions.empty() &&
           "adjustment can not determine function");

    const size_t max_step =
      max_value(line_limit, solver.get_attribute_line_tuple_limit());
    const size_t max_opt = m_options.simulations.size();

    assert(max_step > 0 && "adjustment: can not determine limit");

    info(m_context, "[Computation starts 1/{}\n", max_step);

    {
        m_start = eastl::chrono::system_clock::now();
        for (size_t opt = 0; opt != m_options.size(); ++opt)
            simulated[opt] = solver.solve(m_options.options.row(opt));

        auto kappa = kappa_c.squared(m_options.observed, simulated);

        m_end = eastl::chrono::system_clock::now();

        info(m_context,
             "| line updated | kappa | kappa computed "
             "| time (s) | tuple (attribute, line, value) updated |\n");

        info(m_context,
             "| {} | {:13.10f} | {} | {} | [] |\n",
             0,
             kappa,
             1,
             eastl::chrono::duration<double>(m_end - m_start).count());

        ret.emplace_back();

        ret.back().kappa = kappa;
        ret.back().time =
          eastl::chrono::duration<double>(m_end - m_start).count();
        ret.back().kappa_computed = 1;
        ret.back().function_computed = m_options.size();
    }

    for (size_t step = 1; step <= max_step; ++step) {
        m_start = eastl::chrono::system_clock::now();
        long int loop = 0;

        solver.set_functions(m_globalfunctions);
        solver.init_walkers(step);
        double kappa = 0;

        do {
            solver.init_next_value();
            printf("\r");
            printf("%lu ", loop);

            do {
                for (size_t opt = 0; opt != max_opt; ++opt) {
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

        m_end = eastl::chrono::system_clock::now();
        auto time = eastl::chrono::duration<double>(m_end - m_start).count();

        ret.emplace_back();
        ret.back().kappa = kappa;
        ret.back().time = time;
        ret.back().kappa_computed = static_cast<unsigned long int>(loop);
        ret.back().function_computed = static_cast<unsigned long int>(0);

        info(m_context,
             "| {} | {:13.10f} | {} | {} | ",
             step,
             kappa,
             loop,
             eastl::chrono::duration<double>(m_end - m_start).count());

        print(m_context, m_updaters);
        info(m_context, "\n");
    }

    return ret;
}

} // namespace efyj
