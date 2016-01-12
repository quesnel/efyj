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

#ifndef INRA_EFYj_PROBLEM_HPP
#define INRA_EFYj_PROBLEM_HPP

#include <efyj/efyj.hpp>
#include <efyj/model.hpp>
#include <efyj/options.hpp>
#include <efyj/context.hpp>

namespace efyj {

EFYJ_API
Model
model_read(Context ctx, const std::string& filename);

EFYJ_API
Options
option_read(Context ctx, const Model& model, const std::string &filename);

EFYJ_API
void
option_extract(Context ctx, const Model& model, const std::string& filename);

EFYJ_API
double
compute0(Context ctx, const Model& model, const Options& options,
         int rank, int world_size);

EFYJ_API
double
computen(Context ctx, const Model& model, const Options& options,
         int rank, int world_size, int walker_number);

EFYJ_API
double
compute_for_ever(Context ctx, const Model& model, const Options& options,
                 int rank, int world_size);

EFYJ_API
double
prediction(Context ctx, const Model& model, const Options&
                options, int rank, int world_size);

EFYJ_API
void
model_show(const Model &model, std::ostream &os);

}

#endif
