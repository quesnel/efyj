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

#ifndef INRA_EFYj_SOLVER_HPP
#define INRA_EFYj_SOLVER_HPP

#include "model.hpp"
#include "visibility.hpp"
#include <memory>

namespace efyj {

    struct solver_error : std::logic_error
    {
        solver_error(const std::string& msg)
            : std::logic_error(msg)
        {}
    };

    struct solver_option_error : std::logic_error
    {
        solver_option_error(const std::string& msg)
            : std::logic_error(msg)
        {}
    };

    struct EFYJ_API solver_basic
    {
        solver_basic(dexi& model);

        scale_id solve(const std::vector <efyj::scale_id>& options);

        dexi& model;
    };

    struct EFYJ_API solver_bigmem
    {
        solver_bigmem(dexi& model);

        std::int8_t solve(const std::vector <efyj::scale_id>& options);
        std::int8_t solve(std::size_t options);

        std::size_t binary_scale_value_size;
        std::vector <efyj::scale_id> binary_scales;
        std::vector <std::int8_t> result;
    };

    struct EFYJ_API solver_hash
    {
        solver_hash(dexi& model);
        ~solver_hash();

        std::int8_t solve(const std::vector <efyj::scale_id>& options);
        std::int8_t solve(const std::string& options);

        struct solver_hash_impl;
        std::unique_ptr <solver_hash_impl> impl;
    };
}

#endif
