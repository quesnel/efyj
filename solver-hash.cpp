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
#include "utils.hpp"
#include <algorithm>
#include <unordered_map>

namespace {

    std::string make_key(const std::vector <efyj::scale_id>& options)
    {
        std::string ret;

        for (auto opt : options) {
            if (not efyj::is_valid_scale_id(opt))
                throw efyj::solver_option_error(
                    efyj::stringf("Hash: invalid scale id: %" PRIuMAX,
                                  (static_cast <std::uintmax_t>(opt))));

            ret += ('0' + opt);
        }

        return std::move(ret);
    }

}

namespace efyj {

    solver_hash::solver_hash(dexi& model)
        : basic_attribute_scale_size(model.basic_attribute_scale_size)
    {
        std::vector <std::size_t> high_level(model.basic_scale_number);
        int i = 0;

        for (const auto& att : model.attributes) {
            if (att.children.empty()) {
                high_level[i] = att.scale_size();
                ++i;
            }
        }

        std::vector <scale_id> options(model.basic_scale_number, 0);
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

    efyj::scale_id solver_hash::solve(const std::vector <efyj::scale_id>& options)
    {
        std::string key = ::make_key(options);

        auto it = hash.find(key);
        if (it == hash.end())
            throw solver_error(
                stringf("hash: Unknown result for key: %s", key.c_str()));

        return it->second;
    }

    result_type solver_hash::solve(const std::string& options)
    {
        std::vector <std::string> todo =
            efyj::make_string_options(options, basic_attribute_scale_size);

        result_type ret;

        std::transform(todo.begin(), todo.end(),
                       std::inserter(ret, ret.end()),
                       [this](const std::string& opt)
                       {
                           auto it = hash.find(opt);

                           if (it == hash.end())
                               throw solver_error(
                                   stringf("hash: Unknown result for key: %s",
                                           opt.c_str()));

                           return it->second;
                       });

        return std::move(ret);
    }
}
