/* Copyright (C) 2014-2015 INRA
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

#ifndef INRA_EFYj_SOLVER_GMP_HPP
#define INRA_EFYj_SOLVER_GMP_HPP

#include <efyj/exception.hpp>
#include <efyj/utils.hpp>
#include <efyj/solver-stack.hpp>
#include <efyj/types.hpp>

#include <algorithm>
#include <unordered_map>
#include <gmpxx.h>

namespace efyj {

namespace gmp_details {

template <typename V>
mpz_class make_key(const V &options)
{
    mpz_class ret;

    for (int i = 0, e = options.size(); i != e; ++i) {
        ret *= 10;
        ret += options(i);
    }

    return std::move(ret);
}

} // namespace gmp_details

struct solver_gmp {
    solver_gmp(Model &model)
        : basic_attribute_scale_size(model.basic_attribute_scale_size)
    {
        std::size_t basic_scale_number = 0;

        for (const auto &att : model.attributes)
            if (att.children.empty())
                basic_scale_number++;

        std::vector <scale_id> high_level(basic_scale_number);
        int i = 0;

        for (const auto &att : model.attributes) {
            if (att.children.empty()) {
                high_level[i] = att.scale_size();
                ++i;
            }
        }

        Vector options = Vector::Zero(basic_scale_number);
        efyj::solver_stack basic(model);
        bool end = false;

        do {
            hash.emplace(gmp_details::make_key(options), basic.solve(options));
            std::size_t current = basic_scale_number - 1;

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
    scale_id solve(const V &options)
    {
        mpz_class key = gmp_details::make_key(options);
        auto it = hash.find(key);

        if (it == hash.end())
            throw solver_option_error(key.get_str());

        return it->second;
    }

    std::map <mpz_class, scale_id> hash;
    std::vector <scale_id> basic_attribute_scale_size;
};

}

#endif
