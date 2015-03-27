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

#ifndef INRA_EFYj_SOLVER_HASH_HPP
#define INRA_EFYj_SOLVER_HASH_HPP

#include "solver.hpp"
#include "utils.hpp"
#include <algorithm>
#include <unordered_map>

namespace efyj {

namespace hash_details {

std::string make_key(const Eigen::VectorXi& options)
{
    std::string ret;

    for (int i = 0, e = options.size(); i != e; ++i)
        ret += ('0' + options(i));

    return std::move(ret);
}

}

struct solver_hash
{
    solver_hash(dexi& model)
        : basic_attribute_scale_size(model.basic_attribute_scale_size)
    {
        std::vector <scale_id> high_level(model.basic_scale_number);
        int i = 0;

        for (const auto& att : model.attributes) {
            if (att.children.empty()) {
                high_level[i] = att.scale_size();
                ++i;
            }
        }

        Vector options(model.basic_scale_number);
        efyj::solver_basic basic(model);
        bool end = false;

        do {
            hash.emplace(hash_details::make_key(options), basic.solve(options));

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
    template <typename V>
    scale_id solve(const V& options)
    {
        std::string key = hash_details::make_key(options);

        auto it = hash.find(key);
        if (it == hash.end())
            throw solver_error(
                (fmt("hash: Unknown result for key: %1%") % key).str());

        return it->second;
    }

    std::unordered_map <std::string, scale_id> hash;
    std::vector <scale_id> basic_attribute_scale_size;
};

}

#endif
