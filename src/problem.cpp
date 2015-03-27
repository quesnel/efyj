/* Copyright (C) 2014 INRA
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

#include "problem.hpp"
#include "solver-basic.hpp"
#include "solver-bigmem.hpp"
#include "solver-hash.hpp"
#include "model.hpp"
#include "context.hpp"
#include "exception.hpp"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include "utils.hpp"
#include "options.hpp"
#include "post.hpp"
#include "solver.hpp"
#include <iostream>
#include <fstream>
#include <gmpxx.h>
#include <numeric>
#include <cinttypes>

namespace efyj {

struct problem::pimpl
{
    pimpl(const Context& ctx, const std::string& dexi_filepath,
          const std::string& option_filepath)
        : context(ctx)
    {
        ctx->log(fmt("problem: model '%1%' - option '%2%'\n")
                 % dexi_filepath % option_filepath);

        read_dexi_file(dexi_filepath);
        read_option_file(option_filepath);
    }

    void read_dexi_file(const std::string& filepath)
    {
        std::ifstream ifs(filepath);
        if (!ifs)
            throw efyj::xml_parser_error(
                (efyj::fmt("fail to load DEXi file %1%") % filepath).str());

        ifs >> model;
    }

    void read_option_file(const std::string& filepath)
    {
        std::ifstream ifs(filepath);
        if (!ifs)
            throw efyj::csv_parser_error(
                (efyj::fmt("fail to load option file %1%") % filepath).str());

        options = array_options_read(ifs, model);
    }

    void compute(int rank, int world_size)
    {
        (void)rank;
        (void)world_size;

        solver_basic basic(model);

        for (std::size_t i = 0, e = options.options.rows(); i != e; ++i) {
            try {
                options.ids[i].simulated = basic.solve(options.options.row(i));
                // std::cout << options.ids[i].observated << " "
                //           << options.ids[i].simulated << " "
                //           << std::abs(options.ids[i].observated -
                //                       options.ids[i].simulated) << '\n';
            } catch (const std::exception& e) {
                std::cout << "failure option at line " << i
                          << ": " << e.what() << '\n';
            }
        }
    }

    Context context;
    Options options;
    dexi model;
};

problem::problem(const Context& ctx,
                 const std::string& dexi_filepath,
                 const std::string& option_filepath)
    : m(new problem::pimpl(ctx, dexi_filepath, option_filepath))
{
    std::cout << boost::format("problem instantiate: %1% line(s) to solve with model")
        % m->options.ids.size()
              << '\n';

    std::ofstream ofs("matrix.dat");
    ofs << m->options.options << '\n';
}

problem::~problem()
{
}

void problem::solve(int rank, int world_size)
{
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    m->compute(rank, world_size);

    Post post;
    post.functions.emplace_back(rmsep);
    post.functions.emplace_back(weighted_kappa);
    post.apply(m->model, m->options, std::cout);

    end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s\n";

}

}
