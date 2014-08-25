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

#include "solver.hpp"
#include "print.hpp"
#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>
#include <string>
#include <cmath>
#include <map>
#include <stack>

namespace {

std::string format_error_msg(const efyj::attribute& att, std::uint32_t key)
{
    return std::move(
        efyj::stringf("Unknown solution %" PRIu32 " for attribute %s",
                      key, att.name.c_str()));
}

std::string format_error_msg(const std::vector <efyj::scale_id>& options)
{
    return std::move(
        efyj::stringf("Too many option in vector (%" PRIuMAX ")",
                      static_cast <std::uintmax_t>(options.size())));
}

efyj::scale_id utility_function_get_value(efyj::attribute& att)
{
    std::uint32_t key = std::accumulate(
        att.children.begin(),
        att.children.end(),
        0u,
        [](std::uint32_t init, const efyj::attribute* a)
        {
            return init * 10 + a->value;
        });

    auto it = att.utility_function.find(key);
    if (it == att.utility_function.end())
        throw efyj::solver_error(format_error_msg(att, key));

    return it->second;
}
}

namespace efyj {

solver_basic::solver_basic(dexi& model)
    : model(model)
{
    model.init();
}

scale_id solver_basic::solve(const std::vector <efyj::scale_id>& options)
{
    std::size_t i = 0;
    for (auto& att : model.attributes) {
        if (not att.is_basic()) {
            att.parent_is_solvable = 0;
            att.value = std::numeric_limits <std::size_t>::max();
        }
    }

    for (auto& att : model.attributes) {
        if (att.is_basic()) {
            att.value = options[i++];

            if (i >= model.attributes.size())
                throw solver_option_error(format_error_msg(options));

            if (att.parent != nullptr)
                att.parent->parent_is_solvable++;
        }
    }

    for (;;) {
        for (auto& att : model.attributes) {
            if (not att.is_basic()) {
                if (att.value != std::numeric_limits <std::size_t>::max())
                    break;

                attribute *cur= &att;
                while (cur && cur->parent_is_solvable == cur->children.size()) {
                    scale_id value = ::utility_function_get_value(*cur);
                    if (cur->parent == nullptr)
                        return value;

                    cur->value = value;
                    cur->parent->parent_is_solvable++;
                    cur = cur->parent;
                }
            }
        }
    }
}

scale_id solver_basic::solve(const Eigen::MatrixXi& options)
{
    std::size_t i = 0;
    for (auto& att : model.attributes) {
        if (not att.is_basic()) {
            att.parent_is_solvable = 0;
            att.value = std::numeric_limits <std::size_t>::max();
        }
    }

    for (auto& att : model.attributes) {
        if (att.is_basic()) {
            att.value = options(i++);

            if (i >= model.attributes.size())
                throw std::invalid_argument("i > model.attributes.size()");

            if (att.parent != nullptr)
                att.parent->parent_is_solvable++;
        }
    }

    for (;;) {
        for (auto& att : model.attributes) {
            if (not att.is_basic()) {
                if (att.value != std::numeric_limits <std::size_t>::max())
                    break;

                attribute *cur= &att;
                while (cur && cur->parent_is_solvable == cur->children.size()) {
                    scale_id value = ::utility_function_get_value(*cur);
                    if (cur->parent == nullptr)
                        return value;

                    cur->value = value;
                    cur->parent->parent_is_solvable++;
                    cur = cur->parent;
                }
            }
        }
    }

}


efyj::result_type solver_basic::solve(const std::string& options)
{
    std::vector <std::vector <scale_id>> todo =
        efyj::make_scale_id_options(options, model.basic_attribute_scale_size);

    result_type ret;

    std::transform(todo.cbegin(), todo.cend(),
                   std::inserter(ret, ret.end()),
                   [this](const std::vector <scale_id>& opt)
                   {
                       return solve(opt);
                   });

    return std::move(ret);
}

}
