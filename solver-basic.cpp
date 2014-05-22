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
#include "dbg.hpp"
#include <algorithm>
#include <numeric>
#include <vector>
#include <string>
#include <cmath>
#include <map>
#include <cinttypes>
#include <cstdio>
#include <cstdarg>
#include <stack>

namespace {

    std::string print_c_str(const char* format, ...)
    {
        std::vector <char> buffer(1024, '\0');
        int sz;
        va_list ap;

        for (;;) {
            va_start(ap, format);
            sz = std::vsnprintf(buffer.data(), buffer.size(), format, ap);
            va_end(ap);

            if (sz < 0)
                return std::move(std::string());
            else if (static_cast <std::size_t>(sz) < buffer.size())
                return std::move(std::string(buffer.data(), buffer.size()));
            else
                buffer.resize(sz + 1);
        }
    }

    std::string format_error_msg(const efyj::attribute& att, std::uint32_t key)
    {
        return std::move(
            print_c_str("Unknown solution " PRIu32 " for attribute %s",
                        key, att.name.c_str()));
    }

    std::string format_error_msg(const std::vector <std::uint8_t>& options)
    {
        return std::move(
            print_c_str("Too many option in vector (" PRIuMAX ")",
                        options.size()));
    }

    std::uint_fast8_t utility_function_get_value(efyj::attribute& att)
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

    std::uint_fast8_t solver_basic::solve(const std::vector <std::uint8_t>& options)
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
                        std::uint_fast8_t value = ::utility_function_get_value(*cur);
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
}
