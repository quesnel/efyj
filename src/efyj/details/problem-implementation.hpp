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

#include <efyj/solver-bigmem.hpp>
#include <efyj/solver-stack.hpp>
#include <efyj/model.hpp>
#include <efyj/context.hpp>
#include <efyj/exception.hpp>
#include <efyj/utils.hpp>
#include <efyj/options.hpp>
#include <efyj/post.hpp>

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

    return array_options_read(ifs, model);
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

template <typename Solver>
double compute0(Context ctx, const Model& model, const Options&
                options, int rank, int world_size)
{
    (void)rank;
    (void)world_size;

    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    Solver slv(model);

    std::vector <int> simulated(options.options.rows());

    for (std::size_t i = 0, e = options.options.rows(); i != e; ++i) {
        try {
            simulated[i] = slv.solve(options.options.row(i));
        } catch (const std::exception &e) {
            efyj_info(ctx, boost::format("solve failure option at row %1%: %2%") % i %
                      e.what());
        }
    }

    double ret = linear_weighted_kappa(model, options, simulated,
                                       options.options.rows(),
                                       model.attributes[0].scale.size());
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    efyj_info(ctx,
              boost::format("finished computation at %1% elapsed time: %2% s.\n") %
              std::ctime(&end_time) % elapsed_seconds.count());
    {
        std::ofstream ofs("obs-sim.csv");

        if (ofs) {
            ofs << "observated;simulated\n";

            for (auto i = 0ul, e = options.ids.size(); i != e; ++i)
                ofs << options.ids[i].observated << ';'
                    << simulated[i] << '\n';
        }
    }
    return ret;
}

template <typename Solver>
double compute1(Context ctx, const Model& model, const Options&
                options, int rank, int world_size)
{
    (void)rank;
    (void)world_size;
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    Solver slv(model);
    double best_kappa = 0.0;
    std::ofstream ofs("output.csv");

    line_updater current {0, 0, 0};
    line_updater best;
    slv.update_line_init();

    int computed = 0;
    ofs << "attribte;line;value;kappa\n";

    std::vector <int> simulated(options.options.rows());

    do {
        for (std::size_t i = 0, e = options.options.rows(); i != e; ++i) {
            try {
                simulated[i] = slv.solve(options.options.row(i));
            } catch (const std::exception &e) {
                efyj_info(ctx, boost::format(
                        "solve failure option at row %1%: %2%") % i % e.what());
            }
        }

        double ret = squared_weighted_kappa(model, options, simulated,
                                            options.options.rows(),
                                            model.attributes[0].scale.size());
        ofs << current.attribute << ';' << current.line << ';'
            << current.value << ';' << ret << '\n';

        if (ret > best_kappa) {
            best_kappa = ret;
            best = current;
        }

        ++computed;
    } while (slv.update_line(current));

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    efyj_info(ctx, boost::format("Best kappa %1% (for attribute %2%"
        " line %3% with value %4%)\nDefault kappa: %5%\n%6% lines"
        " changed\nComputation ends at %7% elapsed time: %8%") % best_kappa %
              best.attribute % best.line % best.value %
              compute0 <Solver>(ctx, model, options, rank,
        world_size) % computed % std::ctime(&end_time) %
        elapsed_seconds.count());

    return best_kappa;
}

inline
void show(const Model &model, std::size_t att, std::size_t space,
          std::ostream &os)
{
    os << std::string(space, ' ') << model.attributes[att].name << '\n';

    for (const auto &sc : model.attributes[att].scale.scale)
        os << std::string(space, ' ') << "| " << sc.name << '\n';

    if (model.attributes[att].is_aggregate()) {
        os << std::string(space + 1,
                          ' ') << "\\ -> (" << model.attributes[att].functions.low << ")\n";

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
