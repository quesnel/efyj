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

#include "solver-basic.hpp"
#include "solver-bigmem.hpp"
#include "solver-hash.hpp"
#include "solver-gmp.hpp"
#include "model.hpp"
#include "context.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include "options.hpp"
#include "post.hpp"
#include <fstream>
#include <chrono>

namespace efyj {

namespace problem_details {

void read_model_file(const std::string& filepath, Model& model)
{
    std::ifstream ifs(filepath);
    if (!ifs)
        throw efyj::xml_parser_error(filepath, "fail to open");

    ifs >> model;
}

void read_option_file(const std::string& filepath, const Model& model,
    Options& options)
{
    std::ifstream ifs(filepath);
    if (!ifs)
        throw efyj::csv_parser_error(filepath, "fail to open");

    options = array_options_read(ifs, model);
}

} // namespace problem_details

problem::problem(const Context& ctx,
                 const std::string& Model_filepath,
                 const std::string& option_filepath)
    : context(ctx)
{
    efyj_info(ctx, boost::format("problem: model '%1%' - option '%2%'") % Model_filepath % option_filepath);

    problem_details::read_model_file(Model_filepath, model);
    problem_details::read_option_file(option_filepath, model, options);
}

void problem::compute(int rank, int world_size)
{
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    (void)rank;
    (void)world_size;

    solver_basic basic(model);

    for (std::size_t i = 0, e = options.options.rows(); i != e; ++i) {
        try {
            options.ids[i].simulated = basic.solve(options.options.row(i));
        } catch (const std::exception& e) {
            efyj_info(context, boost::format("solve failure option at row %1%: %2%") % i % e.what());
        }
    }

    Post post;
    post.functions.emplace_back(rmsep);
    post.functions.emplace_back(weighted_kappa);
    post.apply(model, options, context);

    end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    efyj_info(context, boost::format("finished computation at %1% elapsed time: %2% s.\n") % 
        std::ctime(&end_time) % elapsed_seconds.count());
}

}

#endif
