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

#ifndef INRA_EFYj_PROBLEM_HPP
#define INRA_EFYj_PROBLEM_HPP

#include <efyj/context.hpp>
#include <efyj/options.hpp>
#include <efyj/model.hpp>

namespace efyj {

struct problem
{
    problem(const efyj::Context& context,
            const std::string& dexi_filepath,
            const std::string& option_filepath);

    template <typename Solver>
    void compute(int rank, int world_size);

    Context context;
    Options options;
    Model model;
};

void show(const Model& model, std::ostream& os);

}

#include "details/problem-implementation.hpp"

#endif
