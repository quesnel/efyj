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

prediction_evaluator::prediction_evaluator(eastl::shared_ptr<context> context,
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
prediction_evaluator::run(int line_limit, double time_limit, int reduce_mode)
{
    (void)time_limit;

    eastl::vector<result> ret;

    vInfo(m_context, "[Computation starts]\n");

    if (reduce_mode)
        solver.reduce(m_options);

    solver.get_functions(m_globalfunctions);
    assert(not m_globalfunctions.empty() and
           "prediction can not determine function");

    const size_t max_step =
      max_value(line_limit, solver.get_attribute_line_tuple_limit());
    const size_t max_opt = m_options.simulations.size();

    assert(max_step > 0 and "prediction: can not determine limit");

    vInfo(m_context, "[Computation starts 1/%zu]\n", max_step);

    {
        m_start = std::chrono::system_clock::now();
        for (size_t opt = 0; opt != m_options.size(); ++opt)
            m_globalsimulated[opt] = solver.solve(m_options.options.row(opt));

        auto kappa = kappa_c.squared(m_options.observed, m_globalsimulated);

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

    for (size_t step = 1; step <= max_step; ++step) {
        m_start = std::chrono::system_clock::now();
        long int loop = 0;

        eastl::fill(m_globalsimulated.begin(), m_globalsimulated.end(), 0);

        // This cache stores best function found for the same
        // subdataset. This cache drastically improves computation time if
        // their is few subdataset.
        eastl::map<size_t, eastl::vector<eastl::vector<scale_id>>> cache;

        for (size_t opt = 0; opt != max_opt; ++opt) {

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

        ret.emplace_back();
        ret.back().kappa = line_kappa;
        ret.back().time = time;
        ret.back().kappa_computed = static_cast<unsigned long int>(loop);
        ret.back().function_computed = static_cast<unsigned long int>(0);

        vInfo(m_context,
              "| %zu | %13.10f | %zu | %f | ",
              step,
              line_kappa,
              loop,
              std::chrono::duration<double>(m_end - m_start).count());

        print(m_context, m_updaters);
        vInfo(m_context, "\n");
    }

    return ret;
}

} // namespace efyj
