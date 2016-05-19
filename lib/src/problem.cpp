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
#ifdef HAVE_MPI
#include <mpi.h>
#endif

namespace {

std::tuple <unsigned long, double>
compute_best_kappa(std::shared_ptr<efyj::Context> context,
                   const efyj::Model& model,
                   const efyj::Options& options,
                   int walker_number)
{
    std::tuple <unsigned long, double> best {0, 0};
    std::vector <int> simulated(options.options.rows());
    efyj::for_each_model_solver solver(context, model, walker_number);
    efyj::weighted_kappa_calculator kappa_c(
        options.options.rows(),
        model.attributes[0].scale.size());

    const std::size_t optmax = options.options.rows();

    solver.init_walkers(walker_number);
    do {
        solver.init_next_value();
        do {
            for (std::size_t i = 0; i != optmax; ++i)
                simulated[i] = solver.solve(options.options.row(i));

            auto ret = kappa_c.squared(options.observated, simulated);

            std::get <1>(best) = std::max(ret, std::get<1>(best));
            std::get <0>(best)++;
        } while (solver.next_value() == true);
    } while (solver.next_line() == true);

    return best;
}

double
compute_kappa(const efyj::Model& model, const efyj::Options& options)
{
    efyj::solver_stack slv(model);
    std::vector <int> simulated(options.options.rows());
    efyj::weighted_kappa_calculator kappa_c(
        options.options.rows(),
        model.attributes[0].scale.size());

    for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
        simulated[i] = slv.solve(options.options.row(i));

    return kappa_c.squared(options.observated, simulated);
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
    {
#ifdef HAVE_MPI
        problem_type = PROBLEM_MPI_SOLVER;
#endif
    }

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
    : m_impl(std::unique_ptr<Problem::problem_impl>(
                 new Problem::problem_impl(context)))
{}

Problem::Problem(std::shared_ptr<Context> context,
                 unsigned int thread_number)
    : m_impl(std::unique_ptr<Problem::problem_impl>(
                 new Problem::problem_impl(context, thread_number)))
{}

Problem::~Problem()
{}

double
Problem::compute0(const Model& model, const Options& options)
{
    auto ret = compute_kappa(model, options);

    m_impl->context->info().printf("Kappa computed: %f", ret);

    return ret;
}

double
Problem::computen(const Model& model, const Options& options,
                  int walker_number)
{
    auto ret = compute_best_kappa(m_impl->context, model,
                                  options, walker_number);

    m_impl->context->info().printf("Lines changed: %" PRIuMAX "\n"
                                   "Best kappa: %f\n",
                                   std::get<0>(ret),
                                   std::get<1>(ret));

    return std::get<1>(ret);
}

double
Problem::compute_for_ever(const Model& model, const Options& options,
                          bool with_reduce)
{
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    std::vector <int> m_globalsimulated(options.observated.size());
    std::vector <int> m_simulated(options.observated.size());
    std::vector <std::vector<scale_id>> m_globalfunctions, m_functions;
    std::vector <std::tuple<int, int, int>> m_globalupdaters, m_updaters;

    for_each_model_solver solver(m_impl->context, model);
    weighted_kappa_calculator kappa_c(options.options.rows(),
                                      model.attributes[0].scale_size());
    if (with_reduce)
        solver.reduce(options);

    std::size_t step = 1;
    std::size_t max_step = solver.get_attribute_line_tuple_limit();
    unsigned long m_loop = 0;
    double m_kappa = 0;

    m_impl->context->info().printf("Needs to compute from 0 to %d updaters\n"
                                   "- 0 kappa: %f\n",
                                   max_step,
                                   compute_kappa(model, options));

    while (step < max_step) {
        m_kappa = 0;
        m_loop = 0;
        m_globalupdaters.clear();
        solver.init_walkers(step);

        start = std::chrono::system_clock::now();
        do {
            solver.init_next_value();

            do {
                m_loop++;

                std::fill(m_simulated.begin(), m_simulated.end(), 0);
                for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
                    m_simulated[i] = solver.solve(options.options.row(i));

                auto ret = kappa_c.squared(options.observated, m_simulated);

                if (ret > m_kappa) {
                    solver.get_functions(m_functions);
                    m_kappa = ret;
                    m_updaters = solver.updaters();
                }
            } while (solver.next_value() == true);
        } while (solver.next_line() == true);

        end = std::chrono::system_clock::now();

        m_impl->context->info().printf("- %d kappa: %f / loop: %" PRIuMAX
                                       " / updaters: ",
                                       step, m_kappa, m_loop);

        auto duration = std::chrono::duration<double>(end - start).count();
        m_impl->context->info() << m_updaters << " "
                                << duration
                                << "s\n";

        step++;
    }

    return m_kappa;
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
        auto duration = std::chrono::duration<double>(end - start).count();
        m_impl->context->info() << "walker " << step << " " << "("
                                << duration
                                << "s) and " << count << " duplicates\n";
        do {
            current = solver.string_functions();
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
#ifdef HAVE_MPI
        {
            int rank, world;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Comm_size(MPI_COMM_WORLD, &world);
            prediction_mpi(m_impl->context, model, options, rank, world);
            break;
        }
#endif
    default:
        m_impl->context->err() << "unknown prediction solver.\n";
    };
}

} // namespace efyj
