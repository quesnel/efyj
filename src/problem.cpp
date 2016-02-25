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
        efyj::solver_details::for_each_model_solver solver(
            model, options, walker_number);

        do {
            for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
                simulated[i] = solver.solve(options.options.row(i));

            double ret = efyj::squared_weighted_kappa(
                options.observated,
                simulated,
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

    double ret = efyj::linear_weighted_kappa(options.observated,
                                             simulated,
                                             options.options.rows(),
                                             model.attributes[0].scale.size());

#ifndef NDEBUG
    for (auto i = 0ul, e = options.observated.size(); i != e; ++i) {
        if (options.observated[i] != simulated[i]) {
            efyj::err() << efyj::err().red() << "Error: "
                        << efyj::err().def();

            efyj::err().printf("observation (%d) != simulated (%d) at line %d\n",
                               options.observated[i],
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
    : m_impl(std::make_unique<Problem::problem_impl>(rank, world_size))
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
Problem::compute_for_ever(const Model& model, const Options& options,
                          bool with_reduce)
{
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    solver_details::for_each_model_solver solver(model, options, with_reduce);
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

        start = std::chrono::system_clock::now();
        do {
            for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
                simulated[i] = solver.solve(options.options.row(i));

            auto ret = squared_weighted_kappa(options.observated,
                                              simulated,
                                              options.options.rows(),
                                              model.attributes[0].scale.size());

            if (ret > std::get<1>(best)) {
                std::get<1>(best) = ret;
                bestupdaters = solver.updaters();
            }

            ++std::get<0>(best);
        } while (solver.next() == true);

        end = std::chrono::system_clock::now();

        efyj::out().printf("- %d kappa: %f / loop: %" PRIuMAX
                           " / updaters: ",
                           step,
                           std::get<1>(best),
                           std::get<0>(best));

        efyj::out() << bestupdaters << " "
                    << std::chrono::duration<double>(end - start).count()
                    << "s\n";

        bestkappa = std::max(bestkappa, std::get <1>(best));

        //
        // TODO: be carefull, solver.init can throw when end of
        // computation is reached.
        //

        solver.init(step + 1);
    }

    return bestkappa;
}

void
Problem::generate_all_models(const Model& model,
                             const Options& options,
                             std::ostream& os)
{
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    std::set<std::string> limit;
    solver_details::for_each_model_solver solver(model, options, true);
    int walker_number = solver.get_max_updaters();
    std::string current(256, '\0');
    long long int count = 0;

    for (int step = 1; step <= walker_number; ++step) {
        end = std::chrono::system_clock::now();
        efyj::out() << efyj::out().red() << "walker " << out().def()
                    << step << " " << "("
                    << std::chrono::duration<double>(end - start).count()
                    << "s) and " << count << " duplicates\n";
        do {
            solver.m_solver.string_functions(current);
            if (limit.emplace(current).second == true)
                os << solver.m_solver << "\n";
            else
                count++;
        } while (solver.next() == true);

        solver.init(step + 1);
    }
}

void
Problem::prediction(const Model& model, const Options& options)
{
    efyj::out() << efyj::out().redb() << "Prediction started\n"
                << efyj::out().def();

    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    std::vector <int> simulated(options.observated.size(), 0);
    std::vector <solver_details::line_updater> bestupdaters;

    solver_details::for_each_model_solver solver(model, options, true);
    int walker_number = solver.get_max_updaters();

    for (int step = 1; step < walker_number; ++step) {
        start = std::chrono::system_clock::now();
        std::tuple <unsigned long, double> best {0, 0};
        unsigned long long int number_bestkappa = 0;

        do {
            auto it = options.ordered.cbegin();
            while (it != options.ordered.cend()) {
                auto id = it->first;

                std::fill(simulated.begin(), simulated.end(), 0);
                for (; it != options.ordered.cend() && it->first == id; ++it)
                    simulated[it->second] = solver.solve(options.options.row(
                                                             it->second));

                auto ret = squared_weighted_kappa(
                    options.observated,
                    simulated,
                    options.options.rows(),
                    model.attributes[0].scale.size());

                if (ret > std::get<1>(best)) {
                    number_bestkappa = 0;
                    std::get<1>(best) = ret;
                    bestupdaters = solver.updaters();

                    efyj::out().printf("  - best kappa found: %f\n",
                                       std::get<1>(best));
                } else if (ret == std::get<1>(best)) {
                    number_bestkappa++;
                }

                ++std::get<0>(best);
            }
        } while (solver.next() == true);

        end = std::chrono::system_clock::now();

        efyj::out().printf("- %d kappa: %f / loop: %" PRIuMAX
                           " / updaters: %d ",
                           step,
                           std::get<1>(best),
                           std::get<0>(best),
                           number_bestkappa);

        efyj::out() << bestupdaters << " "
                    << std::chrono::duration<double>(end - start).count()
                    << "s\n";

        //
        // TODO: be carefull, solver.init can throw when end of
        // computation is reached.
        //

        solver.init(step + 1);
    }
}


} // namespace efyj
