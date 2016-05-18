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
                          << m_context->info().def() << '\n';

        for_each_model_solver solver(m_context, m_model);
        weighted_kappa_calculator kappa_c(m_options.options.rows(),
                                          m_model.attributes[0].scale.size());
        // solver.reduce(m_options);

        std::size_t max_step = solver.get_attribute_line_tuple_limit();
        std::size_t step = 1;

        m_context->info() << m_context->info().cyanb()
                          << "[Computation starts 1/"
                          << max_step << "]"
                          << m_context->info().def() << '\n';

        const std::size_t optmax = m_options.ordered.size();

        {
            m_start = std::chrono::system_clock::now();
            for (std::size_t opt = 0; opt != optmax; ++opt)
                m_globalsimulated[opt] = solver.solve(m_options.options.row(opt));

            auto ret = kappa_c.squared(m_options.observated, m_globalsimulated);

            m_end = std::chrono::system_clock::now();

            m_context->info() << "| line updated | kappa | kappa computed " \
                "| time (s) | tuple (attribute, line, value) updated |\n";

            m_context->info().printf("| %d | %13.10f | %" PRIuMAX
                                     " | %f | [] |\n", 0, ret, 1,
                                     std::chrono::duration<double>(
                                         m_end - m_start).count());
        }

        while (step < max_step) {
            m_start = std::chrono::system_clock::now();
            m_loop = 0;

            // std::vector<std::vector <std::tuple<int, int, int>>> to_explore;
            std::fill(std::begin(m_globalsimulated), std::end(m_globalsimulated), 0);

            for (std::size_t opt = 0; opt != optmax; ++opt) {
                solver.init_walkers(step);
                m_kappa = 0;

                do {
                    solver.init_next_value();

                    do {
                        std::fill(std::begin(m_simulated), std::end(m_simulated), 0);

                        for (auto x : m_options.ordered[opt])
                            m_simulated[x] = solver.solve(m_options.options.row(x));

                        auto ret = kappa_c.squared(m_options.observated, m_simulated);

                        m_loop++;

                        if (opt == 0 && step == 1) {
                            auto updater = solver.updaters();

                            m_context->info() << "   " << updater << ' ' << ret << ' ' << solver.string_functions() << '\n';
                        }

                        // if (ret >= m_kappa) {
                        // if (ret == m_kappa) {
                        //     if (to_explore.end() == std::find(std::begin(to_explore),
                        //                                       std::end(to_explore),
                        //                                       solver.updaters()))
                        //         to_explore.emplace_back(solver.updaters());
                        // } else {
                        // to_explore.clear();
                        if (ret > m_kappa) {
                            solver.get_functions(m_functions);
                            m_updaters = solver.updaters();
                            m_kappa = ret;
                        }
                    } while (solver.next_value() == true);
                } while (solver.next_line() == true);

                solver.set_functions(m_functions);
                m_globalsimulated[opt] = solver.solve(m_options.options.row(opt));

                if (step == 1)
                    m_context->info() << opt << ' ' << m_globalsimulated[opt] <<
                        ' ' << m_updaters << ' ' << m_kappa << '\n';
            }

            m_kappa = kappa_c.squared(m_options.observated, m_globalsimulated);
            m_loop++;
            m_end = std::chrono::system_clock::now();

            m_context->info().printf("| %d | %13.10f | %" PRIuMAX
                            " | %f | %ld | ",
                            step, m_kappa, m_loop,
                            std::chrono::duration<double>(
                                m_end - m_start).count());

            m_context->info() << m_updaters << " |\n";
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
