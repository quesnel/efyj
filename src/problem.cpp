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
#include "post.hpp"
#include <iterator>
#include <fstream>
#include <chrono>

namespace {

template <typename Solver>
std::tuple <unsigned long, double>
compute_best_kappa(const efyj::Model& model,
                   const efyj::Options& options,
                   int walker_number)
{
    std::tuple <unsigned long, double> best {0, 0};
    std::vector <int> simulated(options.options.rows());

    {
        efyj::solver_details::for_each_model_solver <Solver> solver(
            model, walker_number);

        do {
            for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
                simulated[i] = solver.solve(options.options.row(i));

            double ret = efyj::squared_weighted_kappa(
                model, options, simulated,
                options.options.rows(),
                model.attributes[0].scale.size());

            std::get <1>(best) = std::max(ret, std::get<1>(best));
            std::get <0>(best)++;
        } while (solver.next() == true);
    }

    return best;
}

double
compute_kappa(const efyj::Model& model, const efyj::Options& options)
{
    efyj::solver_details::solver_stack slv(model);

    std::vector <int> simulated(options.options.rows());

    for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
        simulated[i] = slv.solve(options.options.row(i));

    double ret = linear_weighted_kappa(model, options, simulated,
                                       options.options.rows(),
                                       model.attributes[0].scale.size());

#ifndef NDEBUG
    for (auto i = 0ul, e = options.ids.size(); i != e; ++i) {
        if (options.ids[i].observated != simulated[i]) {
            efyj::err() << efyj::err().red() << "Error: "
                        << efyj::err().def();

            efyj::err().printf("observation (%d) != simulated (%d) at line %d\n",
                               options.ids[i].observated,
                               simulated[i],
                               i);
        }
    }
#endif

    return ret;
}

} // anonymous namespace

namespace efyj {

struct Problem::problem_impl
{
    problem_impl(int rank_, int world_size_)
        : rank(rank_)
        , world_size(world_size_)
    {}

    int rank, world_size;
};

Problem::Problem(int rank, int world_size)
    : m_impl(new Problem::problem_impl(rank, world_size))
{
}

Problem::~Problem()
{
}

double
Problem::compute0(const Model& model, const Options& options)
{
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    double ret = compute_kappa(model, options);

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    efyj::out().printf("finished computation at %f elapsed time: %f s.\n"
                       "kappa founded: %f\n",
                       std::ctime(&end_time),
                       elapsed_seconds.count(),
                       ret);

    return ret;
}

double
Problem::computen(const Model& model, const Options& options,
                  int walker_number)
{
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    auto ret = compute_best_kappa<solver_details::solver_stack>(
        model, options, walker_number);

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    efyj::out().printf("Lines changed: %" PRIuMAX "\n"
                       "Best kappa: %f\n"
                       "Computation ends at: %s\n"
                       "Elapsed time: %f\n",
                       std::get<0>(ret),
                       std::get<1>(ret),
                       std::ctime(&end_time),
                       elapsed_seconds.count());

    return std::get <1>(ret);
}

double
Problem::compute_for_ever(const Model& model, const Options& options)
{
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    solver_details::for_each_model_solver<solver_details::solver_stack> solver(
        model);
    std::vector <int> simulated(options.options.rows());
    std::vector <solver_details::line_updater> bestupdaters;
    double bestkappa = 0;
    int walker_number = solver.get_max_updaters();

    efyj::out().printf("Needs to compute from 0 to %d updaters\n"
                       "- 0 kappa: %f\n",
                       walker_number,
                       compute_kappa(model, options));

    for (int step = 1; step <= walker_number; ++step) {
        std::tuple <unsigned long, double> best {0, 0};

        do {
            for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
                simulated[i] = solver.solve(options.options.row(i));

            double ret = squared_weighted_kappa(model, options, simulated,
                                                options.options.rows(),
                                                model.attributes[0].scale.size());

            if (ret > std::get<1>(best)) {
                std::get<1>(best) = ret;
                bestupdaters = solver.updaters();
            }

            ++std::get<0>(best);
        } while (solver.next() == true);

        efyj::out().printf("- %d kappa: %f / loop: %" PRIuMAX
                           " / updaters: ",
                           step,
                           std::get<1>(best),
                           std::get<0>(best));
        efyj::out() << bestupdaters << "\n";

        bestkappa = std::max(bestkappa, std::get <1>(best));

        //
        // TODO: be carefull, solver.init can throw when end of
        // computation is reached.
        //

        solver.init(step + 1);
    }

    return bestkappa;
}

} // namespace efyj
