/* Copyright (C) 2015 INRA
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

#ifndef INRA_EFYj_DETAILS_PROBLEM_IMPLEMENTATION_HPP
#define INRA_EFYj_DETAILS_PROBLEM_IMPLEMENTATION_HPP

#include <efyj/solver-stack.hpp>
#include <efyj/model.hpp>
#include <efyj/context.hpp>
#include <efyj/exception.hpp>
#include <efyj/utils.hpp>
#include <efyj/options.hpp>
#include <efyj/post.hpp>
#include <iterator>
#include <fstream>
#include <chrono>

namespace efyj {

inline
Model model_read(Context ctx, const std::string &filename)
{
    std::ifstream ifs(filename);

    if (!ifs) {
        efyj_err(ctx, boost::format("fails to open model %1%") % filename);
        throw efyj::xml_parser_error(filename, "fail to open");
    }

    Model model;

    ifs >> model;

    return std::move(model);
}

inline
Options option_read(Context ctx, const Model& model, const std::string &filename)
{
    std::ifstream ifs(filename);

    if (!ifs) {
        efyj_err(ctx, boost::format("fails to open option %1%") % filename);
        throw efyj::csv_parser_error(filename, "fail to open");
    }

    return array_options_read(ctx, ifs, model);
}

inline
void option_extract(Context ctx, const Model& model, const std::string& filename)
{
    std::ofstream ofs(filename);

    if (!ofs) {
        efyj_err(ctx, boost::format("fails to extract options in file %1%") % filename);
        throw efyj::efyj_error("bad extract options file");
    }

    model.write_options(ofs);
}

inline double
compute_kappa(Context ctx, const Model& model, const Options& options)
{
    solver_stack slv(model);

    std::vector <int> simulated(options.options.rows());

    for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
        simulated[i] = slv.solve(options.options.row(i));

    double ret = linear_weighted_kappa(model, options, simulated,
                                       options.options.rows(),
                                       model.attributes[0].scale.size());

    if (ctx->log_priority() >= LOG_OPTION_INFO) {
        for (auto i = 0ul, e = options.ids.size(); i != e; ++i) {
            if (options.ids[i].observated != simulated[i]) {
                efyj_info(ctx,
                          boost::format("observation (%1%) != simulated (%2%)"
                                        " at line %3%")
                          % options.ids[i].observated % simulated[i] % i);
            }
        }
    }

    return ret;
}

inline double
compute0(Context ctx, const Model& model, const Options&
         options, int rank, int world_size)
{
    (void)rank;
    (void)world_size;

    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    double ret = compute_kappa(ctx, model, options);

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    efyj_info(ctx, boost::format(
                  "finished computation at %1% elapsed time: %2% s.\n") %
              std::ctime(&end_time) % elapsed_seconds.count());

    efyj_info(ctx, boost::format("kappa founded: %1%") % ret);

    return ret;
}

template <typename Solver>
inline std::tuple <unsigned long, double>
compute_best_kappa(const Model& model, const Options& options, int walker_number)
{
    std::tuple <unsigned long, double> best {0, 0};
    std::vector <int> simulated(options.options.rows());

    {
        for_each_model_solver <Solver> solver(model, walker_number);

        do {
            for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
                simulated[i] = solver.solve(options.options.row(i));

            double ret = squared_weighted_kappa(model, options, simulated,
                                                options.options.rows(),
                                                model.attributes[0].scale.size());

            std::get <1>(best) = std::max(ret, std::get<1>(best));
            std::get <0>(best)++;
        } while (solver.next() == true);
    }

    return best;
}

inline double
computen(Context ctx, const Model& model, const Options& options,
         int rank, int world_size, int walker_number)
{
    (void)rank;
    (void)world_size;
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    auto ret = compute_best_kappa<solver_stack>(model, options, walker_number);

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    efyj_info(ctx, boost::format("Lines changed: %1%\n"
                                 "Best kappa: %2%\n"
                                 "Computation ends at: %3%\n"
                                 "Elapsed time: %4%\n")
              % std::get<0>(ret) % std::get<1>(ret)
              % std::ctime(&end_time) % elapsed_seconds.count());

    return std::get <1>(ret);
}

inline double
compute_for_ever(Context ctx, const Model& model, const Options& options,
                 int rank, int world_size)
{
    (void)rank;
    (void)world_size;

    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    for_each_model_solver<solver_stack> solver(model);
    std::vector <int> simulated(options.options.rows());
    std::vector <line_updater> bestupdaters;
    double bestkappa = 0;
    int walker_number = solver.get_max_updaters();

    efyj_info(ctx,
              boost::format("Needs to compute from 0 to %1% updaters\n"
                            "- 0 kappa: %2%\n")
              % walker_number % compute_kappa(ctx, model, options));

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

        efyj_info(ctx,
                  boost::format("- %1% kappa: %2% / loop: %3% / updaters: %4%\n")
                  % step % std::get<1>(best) % std::get<0>(best) % bestupdaters);

        bestkappa = std::max(bestkappa, std::get <1>(best));

        //
        // TODO: be carefull, solver.init can throw when end of
        // computation is reached.
        //

        solver.init(step + 1);
    }

    return bestkappa;
}

inline void
show(const Model &model, std::size_t att, std::size_t space,
     std::ostream &os)
{
    os << std::string(space, ' ') << model.attributes[att].name << '\n';

    for (const auto &sc : model.attributes[att].scale.scale)
        os << std::string(space, ' ') << "| " << sc.name << '\n';

    if (model.attributes[att].is_aggregate()) {
        os << std::string(space + 1, ' ')
           << "\\ -> (fct: "
           << model.attributes[att].functions.low
           << "), (scale size: "
           << model.attributes[att].scale_size()
           << ")\n";

        for (std::size_t child : model.attributes[att].children) {
            show(model, child, space + 2, os);
        }
    }
}

inline
void model_show(const Model &model, std::ostream &os)
{
    show(model, 0, 0, os);
}

}

#endif
