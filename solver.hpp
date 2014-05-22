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

    struct solver_basic
    {
        solver_basic(dexi& model);

        std::uint_fast8_t solve(const std::vector <std::uint8_t>& options);

        dexi& model;
    };

    struct solver_bigmem
    {
        solver_bigmem(dexi& model);
        ~solver_bigmem();

        std::int8_t solve(const std::vector <std::uint8_t>& options);

        struct solver_bigmem_impl;
        std::unique_ptr <solver_bigmem_impl> impl;
    };

    struct solver_hash
    {
        solver_hash(dexi& model);
        ~solver_hash();

        std::int8_t solve(const std::vector <std::uint8_t>& options);

        struct solver_hash_impl;
        std::unique_ptr <solver_hash_impl> impl;
    };
}

#endif
