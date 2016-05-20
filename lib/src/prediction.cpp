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
    std::vector <int> m_globalsimulated;
    std::vector <std::tuple<int, int, int>> m_updaters;
    std::vector <std::vector<scale_id>> m_globalfunctions, m_functions;

    unsigned long long int m_loop = 0;
    double m_kappa = 0.0;

    compute_prediction_0(std::shared_ptr<Context> context,
                         const Model& model,
                         const Options& options)
        : m_context(context)
        , m_model(model)
        , m_options(options)
        , m_globalsimulated(options.observed.size(), 0)
        , m_loop(0)
        , m_kappa(0)
    {
        assert(m_options.options.rows() > 0 and "prediction: empty option");
    }

    void run()
    {
        m_context->info() << m_context->info().cyanb()
                          << "[Computation start]"
                          << m_context->info().def() << '\n';

        std::vector<int> simulated(m_options.options.rows());
        std::vector<int> observed(m_options.options.rows());
        for_each_model_solver solver(m_context, m_model);
        weighted_kappa_calculator kappa_c(m_model.attributes[0].scale.size());
        // options.reduce();
        solver.reduce(m_options);

        solver.get_functions(m_globalfunctions);
        assert(not m_globalfunctions.empty()
               and "prediction can not determine function");

        const std::size_t max_step = solver.get_attribute_line_tuple_limit();
        assert(max_step > 0 and "prediction: can not determine limit");


        m_context->info() << m_context->info().cyanb()
                          << "[Computation starts 1/"
                          << max_step << "]"
                          << m_context->info().def() << '\n';

        const std::size_t optmax = m_options.size();

        {
            m_start = std::chrono::system_clock::now();
            for (std::size_t opt = 0; opt != optmax; ++opt)
                m_globalsimulated[opt] = solver.solve(
                    m_options.options.row(opt));

            auto ret = kappa_c.squared(m_options.observed, m_globalsimulated);

            m_end = std::chrono::system_clock::now();

            m_context->info() << "| line updated | kappa | kappa computed " \
                "| time (s) | tuple (attribute, line, value) updated |\n";

            m_context->info().printf("| %d | %13.10f | %" PRIuMAX
                                     " | %f | [] |\n", 0, ret, 1,
                                     std::chrono::duration<double>(
                                         m_end - m_start).count());
        }


        for (std::size_t step = 1; step < max_step; ++step) {
            m_start = std::chrono::system_clock::now();
            m_loop = 0;

            std::fill(std::begin(m_globalsimulated),
                      std::end(m_globalsimulated), 0);

            std::map<std::size_t, std::vector <std::vector<scale_id>>> cache;
            for (std::size_t opt = 0; opt != optmax; ++opt) {

                {
                    auto it = cache.find(m_options.identifier(opt));
                    if (it != cache.end()) {
                        solver.set_functions(it->second);
                        m_globalsimulated[opt] = solver.solve(
                            m_options.options.row(opt));
                        continue;
                    }
                }

                solver.set_functions(m_globalfunctions);
                solver.init_walkers(step);
                m_kappa = 0;

                do {
                    solver.init_next_value();

                    do {
                        const auto size = m_options.get_subdataset(opt).size();
                        simulated.resize(size, 0);
                        observed.resize(size, 0);

                        for (std::size_t i = 0; i != size; ++i) {
                            auto id = m_options.get_subdataset(opt)[i];
                            observed[i] = m_options.observed[id];
                            simulated[i] = solver.solve(
                                m_options.options.row(id));
                        }

                        auto ret = kappa_c.squared(observed, simulated);
                        m_loop++;

                        if (ret > m_kappa) {
                            solver.get_functions(m_functions);
                            m_updaters = solver.updaters();
                            m_kappa = ret;
                        }
                    } while (solver.next_value() == true);
                } while (solver.next_line() == true);

                solver.set_functions(m_functions);
                m_globalsimulated[opt] = solver.solve(
                    m_options.options.row(opt));

                cache[m_options.identifier(opt)] = m_functions;
            }

            m_kappa = kappa_c.squared(m_options.observed, m_globalsimulated);
            m_loop++;
            m_end = std::chrono::system_clock::now();

            m_context->info().printf("| %d | %13.10f | %" PRIuMAX
                            " | %f | ",
                            step, m_kappa, m_loop,
                            std::chrono::duration<double>(
                                m_end - m_start).count());

            m_context->info() << m_updaters << " |\n";
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
