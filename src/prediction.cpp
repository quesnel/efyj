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

#include "problem.hpp"
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

namespace efyj {

struct squared_weighted_kappa
{
    Eigen::ArrayXXd observed = Eigen::ArrayXXd::Zero(NC, NC);
    Eigen::ArrayX2d distributions = Eigen::ArrayXXd::Zero(NC, 2);
    Eigen::ArrayXXd expected = Eigen::ArrayXXd::Zero(NC, NC);
    Eigen::ArrayXXd weighted = Eigen::ArrayXXd(NC, NC);
    std::size_t N;
    std::size_t NC;

    inline
    squared_weighted_kappa(std::size_t N_, std::size_t NC_)
        : observed(Eigen::ArrayXXd::Zero(NC_, NC_))
        , distributions(Eigen::ArrayXXd::Zero(NC_, 2))
        , expected(Eigen::ArrayXXd::Zero(NC_, NC_))
        , weighted(Eigen::ArrayXXd(NC_, NC_))
        , N(N_)
        , NC(NC_)
    {}

    inline
    double
    run(const std::vector <int>& observated,
        const std::vector <int>& simulated)
    {
        observed.setZero();
        distributions.setZero();

        for (int i = 0; i != (int)N; ++i) {
            ++observed(observated[i], simulated[i]);
            ++distributions(observated[i], 0);
            ++distributions(simulated[i], 1);
        }

        observed /= (double)N;
        distributions /= (double)N;

        for (int i = 0; i != (int)NC; ++i)
            for (int j = 0; j != (int)NC; ++j)
                expected(i, j) = distributions(i, 0) * distributions(j, 1);

        for (int i = 0; i != (int)NC; ++i)
            for (int j = 0; j != (int)NC; ++j)
                weighted(i, j) = std::abs(i - j) * std::abs(i - j);

        return 1.0 - ((weighted * observed).sum() /
                      (weighted * expected).sum());
    }
};

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
        squared_weighted_kappa kappa_compute(m_options.options.rows(),
                                             m_model.attributes[0].scale.size());
        std::size_t step = 1;
        m_loop = 0;

        solver.reduce(m_options);

        m_context->info() << m_context->info().cyanb()
                          << "[Computation starts]"
                          << m_context->info().def()
                          << '\n';

        {
            m_start = std::chrono::system_clock::now();
            for (std::size_t i = 0, e = m_options.options.rows(); i != e; ++i)
                m_globalsimulated[i] = solver.solve(m_options.options.row(i));

            auto ret = kappa_compute.run(m_options.observated, m_globalsimulated);

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

        while (step < std::numeric_limits<std::size_t>::max()) {
            m_start = std::chrono::system_clock::now();
            m_kappa = 0;
            m_globalupdaters.clear();

            solver.init_walkers(step);

            do {
                std::fill(m_globalsimulated.begin(), m_globalsimulated.end(), 0.);

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

                        auto ret = kappa_compute.run(m_options.observated,
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

                auto ret = kappa_compute.run(m_options.observated,
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
