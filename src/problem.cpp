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
#include "prediction.hpp"
#include <iterator>
#include <fstream>
#include <chrono>
#include <memory>
#include <thread>

namespace {

template <typename Solver>
std::tuple <unsigned long, double>
compute_best_kappa(std::shared_ptr<efyj::Context> context,
                   const efyj::Model& model,
                   const efyj::Options& options,
                   int walker_number)
{
    std::tuple <unsigned long, double> best {0, 0};
    std::vector <int> simulated(options.options.rows());

    {
        efyj::for_each_model_solver solver(context, model, walker_number);

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
        } while (solver.next_value() == true);
    }

    return best;
}

double
compute_kappa(const efyj::Model& model, const efyj::Options& options)
{
    efyj::solver_stack slv(model);

    std::vector <int> simulated(options.options.rows());

    for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
        simulated[i] = slv.solve(options.options.row(i));

    return efyj::linear_weighted_kappa(options.observated,
                                       simulated,
                                       options.options.rows(),
                                       model.attributes[0].scale.size());
}

} // anonymous namespace

namespace efyj {

struct Problem::problem_impl
{
    enum problem_solver_type {
        PROBLEM_MONO_SOLVER,
        PROBLEM_THREAD_SOLVER,
        PROBLEM_MPI_SOLVER
    };

    problem_impl(std::shared_ptr<Context> context_)
        : context(context_)
        , problem_type(PROBLEM_MONO_SOLVER)
    {}

    problem_impl(std::shared_ptr<Context> context_, unsigned int thread)
        : context(context_)
        , thread_number(thread)
        , problem_type(PROBLEM_THREAD_SOLVER)
    {
        if (thread == 0)
            thread_number = get_hardware_concurrency();
    }

    unsigned int get_thread_number()
    {
        return thread_number;
    }

    std::shared_ptr<Context> context;
    int rank, world_size;
    unsigned int thread_number;
    problem_solver_type problem_type;
};

Problem::Problem(std::shared_ptr<Context> context)
    : m_impl(std::make_unique<Problem::problem_impl>(context))
{}

Problem::Problem(std::shared_ptr<Context> context,
                 unsigned int thread_number)
    : m_impl(std::make_unique<Problem::problem_impl>(context, thread_number))
{}

Problem::~Problem()
{}

double
Problem::compute0(const Model& model, const Options& options)
{
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    double ret = compute_kappa(model, options);

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    m_impl->context->info().printf("finished computation at %f elapsed time: %f s.\n"
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

    auto ret = compute_best_kappa<solver_stack>(
        m_impl->context, model, options, walker_number);

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    m_impl->context->info().printf("Lines changed: %" PRIuMAX "\n"
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

    for_each_model_solver solver(m_impl->context, model);
    if (with_reduce)
        solver.reduce(options);

    std::vector <int> simulated(options.options.rows());
    std::vector <std::tuple<int, int, int>> bestupdaters;
    double bestkappa = 0;
    int walker_number = INT_MAX;

    m_impl->context->info().printf("Needs to compute from 0 to %d updaters\n"
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
        } while (solver.next_value() == true);

        end = std::chrono::system_clock::now();

        m_impl->context->info().printf("- %d kappa: %f / loop: %" PRIuMAX
                           " / updaters: ",
                           step,
                           std::get<1>(best),
                           std::get<0>(best));

        m_impl->context->info() << bestupdaters << " "
                                << std::chrono::duration<double>(end - start).count()
                                << "s\n";

        bestkappa = std::max(bestkappa, std::get <1>(best));

        //
        // TODO: be carefull, solver.init can throw when end of
        // computation is reached.
        //

        solver.init_walkers(step + 1);
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
    for_each_model_solver solver(m_impl->context, model);
    solver.reduce(options);
    int walker_number = INT_MAX;
    std::string current(256, '\0');
    long long int count = 0;

    for (int step = 1; step <= walker_number; ++step) {
        end = std::chrono::system_clock::now();
        m_impl->context->info() << "walker " << step << " " << "("
                                << std::chrono::duration<double>(end - start).count()
                                << "s) and " << count << " duplicates\n";
        do {
            solver.m_solver.string_functions(current);
            if (limit.emplace(current).second == true)
                os << solver.m_solver << "\n";
            else
                count++;
        } while (solver.next_value() == true);

        solver.init_walkers(step + 1);
    }
}

void
Problem::prediction(const Model& model, const Options& options)
{
    switch (m_impl->problem_type) {
    case Problem::problem_impl::PROBLEM_MONO_SOLVER:
        prediction_0(m_impl->context, model, options);
        break;
    case Problem::problem_impl::PROBLEM_THREAD_SOLVER:
        prediction_n(m_impl->context, model, options,
                     m_impl->get_thread_number());
        break;
    case Problem::problem_impl::PROBLEM_MPI_SOLVER:
    default:
        m_impl->context->err() << "unknown prediction solver.\n";
    };
}

} // namespace efyj
