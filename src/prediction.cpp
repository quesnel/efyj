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

#include "prediction.hpp"
#include "problem.hpp"
#include "post.hpp"
#include "solver-stack.hpp"
#include "model.hpp"
#include "context.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include "options.hpp"
#include <iterator>
#include <fstream>
#include <chrono>
#include <thread>
#include <cmath>

namespace efyj {

struct compute_prediction_0
{
    std::shared_ptr<Context> m_context;
    const Model& m_model;
    const Options& m_options;

    std::chrono::time_point<std::chrono::system_clock> m_start, m_end;
    std::vector <int> m_globalsimulated, m_simulated;
    std::vector <std::tuple<int, int, int>> m_globalupdaters, m_updaters;
    std::vector <std::vector<scale_id>> m_globalfunctions, m_functions;

    unsigned long long int m_loop = 0;
    double m_kappa = 0.0;

    compute_prediction_0(std::shared_ptr<Context> context,
                         const Model& model,
                         const Options& options)
        : m_context(context)
        , m_model(model)
        , m_options(options)
        , m_globalsimulated(options.observated.size(), 0)
        , m_simulated(options.observated.size(), 0)
        , m_loop(0)
        , m_kappa(0)
    {
    }

    void run()
    {
        m_context->info() << m_context->info().cyanb()
                          << "[Computation start]"
                          << m_context->info().def()
                          << '\n';

        for_each_model_solver solver(m_context, m_model);
        weighted_kappa_calculator kappa_c(m_options.options.rows(),
                                          m_model.attributes[0].scale.size());
        solver.reduce(m_options);

        std::size_t max_step = solver.get_attribute_line_tuple_limit();
        std::size_t step = 1;
        m_loop = 0;

        m_context->info() << m_context->info().cyanb()
                          << "[Computation starts 1/"
                          << max_step << "]"
                          << m_context->info().def()
                          << '\n';

        {
            m_start = std::chrono::system_clock::now();
            for (std::size_t i = 0, e = m_options.options.rows(); i != e; ++i)
                m_globalsimulated[i] = solver.solve(m_options.options.row(i));

            auto ret = kappa_c.squared(m_options.observated, m_globalsimulated);

            m_end = std::chrono::system_clock::now();

            m_context->info() << "| line updated | kappa | kappa computed " \
                "| time (s) | tuple (attribute, line, value) updated |\n";

            m_context->info().printf("| %d | %13.10f | %" PRIuMAX
                                     " | %f | [] |\n",
                                     0,
                                     ret,
                                     1,
                                     std::chrono::duration<double>(
                                         m_end - m_start).count());
        }

        while (step < max_step) {
            m_start = std::chrono::system_clock::now();
            m_kappa = 0;
            m_globalupdaters.clear();

            solver.init_walkers(step);

            do {
                std::fill(m_globalsimulated.begin(),
                          m_globalsimulated.end(), 0.);

                for (std::size_t opt = 0, endopt = m_options.ordered.size();
                     opt != endopt; ++opt) {
                    // Attribute and line of walker(s) are fixed here. Now we
                    // compute the best kappa for value of walkers.
                    double kappa = 0.;

                    solver.init_next_value();

                    do {
                        std::fill(m_simulated.begin(), m_simulated.end(), 0.);

                        for (auto x : m_options.ordered[opt])
                            m_simulated[x] = solver.solve(
                                m_options.options.row(x));

                        auto ret = kappa_c.squared(m_options.observated,
                                                   m_simulated);

                        m_loop++;

                        if (ret > kappa) {
                            solver.get_functions(m_functions);
                            m_updaters = solver.updaters();
                            kappa = ret;
                        }
                    } while (solver.next_value() == true);

                    // We assign the best updaters an get simulated value of
                    // the current line option.
                    solver.set_functions(m_functions);
                    m_globalsimulated[opt] = solver.solve(
                        m_options.options.row(opt));
                }

                auto ret = kappa_c.squared(m_options.observated,
                                           m_globalsimulated);

                m_loop++;

                if (ret > m_kappa) {
                    m_kappa = ret;
                    m_globalupdaters = m_updaters;
                    m_globalfunctions = m_functions;
                }
            } while (solver.next_line() == true);

            m_end = std::chrono::system_clock::now();

            m_context->info().printf("| %d | %13.10f | %" PRIuMAX
                                     " | %f | ",
                                     step, m_kappa, m_loop,
                                     std::chrono::duration<double>(
                                         m_end - m_start).count());

            m_context->info() << m_globalupdaters << " |\n";

            step++;
        }
    };
};

void prediction_0(std::shared_ptr<Context> context,
                  const Model& model,
                  const Options& options)
{
    compute_prediction_0 computer(context, model, options);

    computer.run();
}

} // namespace efyj
