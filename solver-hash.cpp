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
#include <unordered_map>
#include <iostream>

namespace {

    std::string make_key(const std::vector <std::uint8_t>& options)
    {
        std::string ret;

        for (auto opt : options) {
            if (opt > 9)
                throw efyj::solver_option_error("options must be [0-9]");

            ret += std::to_string(opt);
        }

        return std::move(ret);
    }

}

namespace efyj {

    struct solver_hash::solver_hash_impl
    {
        solver_hash_impl(dexi& model)
        {
            std::vector <std::size_t> high_level(model.basic_scale_number);
            int i = 0;

            for (const auto& att : model.attributes) {
                if (att.children.empty()) {
                    high_level[i] = att.scale_size();
                    ++i;
                }
            }

            std::vector <std::uint_fast8_t> options(model.basic_scale_number, 0);
            efyj::solver_basic basic(model);
            bool end = false;

            do {
                hash.emplace(make_key(options), basic.solve(options));

                std::size_t current = model.basic_scale_number - 1;
                do {
                    options[current]++;
                    if (options[current] >= high_level[current]) {
                        options[current] = 0;

                        if (current == 0) {
                            end = true;
                            break;
                        } else {
                            --current;
                        }
                    } else
                        break;
                } while (not end);
            } while (not end);
        }

        std::unordered_map <std::string, std::uint_fast8_t> hash;
    };

    solver_hash::solver_hash(dexi& model)
        : impl(new solver_hash::solver_hash_impl(model))
    {
    }

    solver_hash::~solver_hash()
    {
    }

    std::int8_t solver_hash::solve(const std::vector <std::uint8_t>& options)
    {
        std::string key = ::make_key(options);

        auto it = impl->hash.find(key);
        if (it == impl->hash.end())
            throw solver_error(
                stringf("Unknown result for key: %s", key.c_str()));

        return it->second;
    }

    std::int8_t solver_hash::solve(const std::string& options)
    {
        auto it = impl->hash.find(options);
        if (it == impl->hash.end())
            throw solver_error(
                stringf("Unknown result for key: %s", options.c_str()));

        return it->second;
    }
}
