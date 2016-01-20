/* Copyright (C) 2016 INRA
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

#ifndef INRA_EFYj_SOLVER_HPP
#define INRA_EFYj_SOLVER_HPP

#include <efyj/efyj.hpp>
#include <efyj/model.hpp>
#include <efyj/options.hpp>
#include <memory>

namespace efyj {

class EFYJ_API Solver
{
    struct solver_impl;
    std::unique_ptr<solver_impl> m_impl;

public:
    Solver(const Model& model);
    ~Solver();

    Solver(const Solver&) = delete;
    Solver(Solver&&) = delete;
    Solver& operator=(const Solver&) = delete;
    Solver& operator=(Solver&&) = delete;

    scale_id solve(const Vector& opt);
};

} // namespace efyj

#endif
