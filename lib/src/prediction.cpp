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

#include "prediction.hpp"
#include "utils.hpp"

namespace efyj {

prediction_evaluator::prediction_evaluator(const context& ctx,
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
{
}

bool
prediction_evaluator::is_valid() const noexcept
{
    return m_options.have_subdataset();
}

void
prediction_evaluator::run(const result_callback& cb,
                          int line_limit,
                          double time_limit,
                          int reduce_mode)
{
    (void)time_limit;

    result ret;

    info(m_context, "[Computation starts]\n");

    if (reduce_mode)
        solver.reduce(m_options);

    solver.get_functions(m_globalfunctions);
    assert(!m_globalfunctions.empty() &&
           "prediction can not determine function");

    const size_t max_step =
      max_value(line_limit, solver.get_attribute_line_tuple_limit());
    const size_t max_opt = m_options.simulations.size();

    assert(max_step > 0 && "prediction: can not determine limit");

    info(m_context, "[Computation starts 1/{}]\n", max_step);

    {
        m_start = std::chrono::system_clock::now();
        for (size_t opt = 0; opt != m_options.size(); ++opt)
            m_globalsimulated[opt] = solver.solve(m_options.options.row(opt));

        auto kappa = kappa_c.squared(m_options.observed, m_globalsimulated);

        m_end = std::chrono::system_clock::now();

        info(m_context,
             "| line updated | kappa | kappa computed "
             "| time (s) | tuple (attribute, line, value) updated |\n");

        info(m_context,
             "| {} | {:13.10f} | {} | {} | [] |\n",
             0,
             kappa,
             1,
             std::chrono::duration<double>(m_end - m_start).count());

        ret.kappa = kappa;
        ret.time = std::chrono::duration<double>(m_end - m_start).count();
        ret.kappa_computed = 1;
        ret.function_computed = numeric_cast<unsigned long>(m_options.size());
        
        if (!cb(ret))
            return;
    }

    for (size_t step = 1; step <= max_step; ++step) {
        m_start = std::chrono::system_clock::now();
        long int loop = 0;

        std::fill(m_globalsimulated.begin(), m_globalsimulated.end(), 0);

        // This cache stores best function found for the same
        // subdataset. This cache drastically improves computation time if
        // their is few subdataset.
        std::map<size_t, std::vector<std::vector<scale_id>>> cache;

        for (int opt = 0, e = static_cast<int>(max_opt); opt != e; ++opt) {

            {
                auto it = cache.find(m_options.identifier(opt));
                if (it != cache.end()) {
                    solver.set_functions(it->second);
                    m_globalsimulated[opt] =
                      solver.solve(m_options.options.row(opt));
                    continue;
                }
            }

            solver.set_functions(m_globalfunctions);
            solver.init_walkers(step);
            double kappa = 0;

            do {
                solver.init_next_value();

                do {
                    const auto size = m_options.get_subdataset(opt).size();
                    simulated.resize(size, 0);
                    observed.resize(size, 0);
                    assert(size > 0);

                    for (size_t i = 0; i != size; ++i) {
                        auto id = m_options.get_subdataset(opt)[i];
                        observed[i] = m_options.observed[id];
                        simulated[i] = solver.solve(m_options.options.row(id));
                    }

                    auto localkappa = kappa_c.squared(observed, simulated);
                    loop++;

                    if (localkappa > kappa) {
                        solver.get_functions(m_functions);
                        m_updaters = solver.updaters();
                        kappa = localkappa;
                    }

                } while (solver.next_value() == true);
            } while (solver.next_line() == true);

            solver.set_functions(m_functions);
            m_globalsimulated[opt] = solver.solve(m_options.options.row(opt));

            cache[m_options.identifier(opt)] = m_functions;
        }

        auto line_kappa =
          kappa_c.squared(m_options.observed, m_globalsimulated);
        m_end = std::chrono::system_clock::now();

        auto time = std::chrono::duration<double>(m_end - m_start).count();
        loop++;

        ret.kappa = line_kappa;
        ret.time = time;
        ret.kappa_computed = static_cast<unsigned long int>(loop);
        ret.function_computed = static_cast<unsigned long int>(0);
        ret.modifiers.clear();

        for (const auto elem : m_updaters)
            ret.modifiers.emplace_back(
              std::get<0>(elem), std::get<1>(elem), std::get<2>(elem));

        if (!cb(ret))
            return;
    }
}

} // namespace efyj
