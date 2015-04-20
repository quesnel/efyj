/* Copyright (C) 2014-2015 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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

problem::problem(const Context &ctx, const std::string &file)
    : m_context(ctx)
{
    std::ifstream ifs(file);

    if (!ifs)
        throw efyj::xml_parser_error(file, "fail to open");

    ifs >> m_model;
}

void problem::extract(const std::string& file)
{
    std::ofstream ofs(file);

    if (!ofs)
        throw efyj::efyj_error("bad extract options file");

    m_model.write_options(ofs);
}

void problem::options(const std::string& file)
{
    std::ifstream ifs(file);

    if (!ifs)
        throw efyj::csv_parser_error(file, "fail to open");

    m_options = array_options_read(ifs, m_model);
}

template <typename Solver>
double problem::compute(int rank, int world_size)
{
    (void)rank;
    (void)world_size;
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    Solver slv(m_model);

    for (std::size_t i = 0, e = m_options.options.rows(); i != e; ++i) {
        try {
            m_options.ids[i].simulated = slv.solve(m_options.options.row(i));
        } catch (const std::exception &e) {
            efyj_info(m_context, boost::format("solve failure option at row %1%: %2%") % i %
                      e.what());
        }
    }

    double ret = weighted_kappa(m_model, m_options, m_options.options.rows(), m_model.attributes[0].scale.size(), m_context);

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    efyj_info(m_context,
              boost::format("finished computation at %1% elapsed time: %2% s.\n") %
              std::ctime(&end_time) % elapsed_seconds.count());

    {
        std::ofstream ofs("obs-sim.csv");

        if (ofs) {
            ofs << "observated;simulated\n";

            for (const auto &opt : m_options.ids)
                ofs << opt.observated << ';' << opt.simulated << '\n';
        }
    }

    return ret;
}

inline
void show(const Model& model, std::size_t att, std::size_t space, std::ostream& os)
{
    os << std::string(space, ' ') << model.attributes[att].name << '\n';

    for (const auto& sc : model.attributes[att].scale.scale)
        os << std::string(space, ' ') << "| " << sc.name << '\n';

    if (model.attributes[att].is_aggregate()) {
       os << std::string(space + 1, ' ') << "\\ -> (" << model.attributes[att].functions.low << ")\n";

        for (std::size_t child : model.attributes[att].children) {
            show(model, child, space + 2, os);
        }
    }
}

inline
void show(const Model& model, std::ostream& os)
{
    show(model, 0, 0, os);
}

}

#endif
