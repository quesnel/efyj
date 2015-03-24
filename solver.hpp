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

#include <efyj/model.hpp>
#include <efyj/visibility.hpp>
#include <memory>
#include <set>
#include <unordered_map>

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#endif

#include  <Eigen/src/Core/util/DisableStupidWarnings.h>
#include <Eigen/Core>

namespace efyj {

    typedef std::set <scale_id> result_type;

    EFYJ_API std::vector <std::string> make_string_options(
        const std::string& options,
        const std::vector <scale_id>& max_options);

    EFYJ_API std::vector <std::vector <scale_id>> make_scale_id_options(
        const std::string& options,
        const std::vector <scale_id>& max_options);

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

        scale_id solve(const std::vector <scale_id>& options);
        result_type solve(const std::string& options);
        scale_id solve(const Eigen::MatrixXi& options);

        dexi& model;
    };

    struct EFYJ_API solver_bigmem
    {
        solver_bigmem(dexi& model);

        scale_id solve(const std::vector <scale_id>& options);
        scale_id solve(std::size_t options);
        result_type solve(const std::string& options);

        std::size_t binary_scale_value_size;
        std::vector <scale_id> binary_scales;
        std::vector <scale_id> result;
        std::vector <scale_id> basic_attribute_scale_size;
    };

    struct EFYJ_API solver_hash
    {
        solver_hash(dexi& model);

        scale_id solve(const std::vector <scale_id>& options);
        result_type solve(const std::string& options);

        std::unordered_map <std::string, scale_id> hash;
        std::vector <scale_id> basic_attribute_scale_size;
    };
}

#endif
