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
#include <boost/dynamic_bitset.hpp>
#include <numeric>
#include <vector>
#include <string>
#include <cmath>

namespace {

    unsigned long make_key(const std::vector <efyj::scale_id>& options,
                           const std::vector <efyj::scale_id>& bits)
    {
        unsigned long ret = 0;
        std::size_t indice = 0;

        for (std::size_t i = 0, e = bits.size(); i != e; ++i) {
            ret += (options[i] << indice);
            indice += std::floor(std::log2(bits[i]) + 1);
        }

        return ret;
    }

    unsigned long make_key(const std::string& options,
                           const std::vector <efyj::scale_id>& bits)
    {
        unsigned long ret = 0;
        std::size_t indice = 0;

        for (std::size_t i = 0, e = bits.size(); i != e; ++i) {
            ret += ((options[i] - '0') << indice);
            indice += std::floor(std::log2(bits[i]) + 1);
        }

        return ret;
    }

}

namespace efyj {

    solver_bigmem::solver_bigmem(dexi& model)
        : binary_scale_value_size(0)
          , basic_attribute_scale_size(model.basic_attribute_scale_size)
    {
        {
            for (const auto& att : model.attributes) {
                if (att.children.empty()) {
                    binary_scales.push_back(
                        std::floor(std::log2(att.scale.scale.size())) + 1);
                    binary_scale_value_size += binary_scales.back();
                }
            }

            try {
                result.resize(std::pow(2, binary_scale_value_size),
                              scale_id_unknown());
            } catch (const std::bad_alloc&) {
                throw std::logic_error(
                    efyj::stringf("bigmem: failed to allocate %f GB",
                                  std::pow(2, binary_scale_value_size) / 1024 /
                                  1024) + "GB");
            }

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
                unsigned long key = ::make_key(options, binary_scales);
                result[key] = basic.solve(options);

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
        };
    }

    efyj::scale_id solver_bigmem::solve(const std::vector <efyj::scale_id>& options)
    {
        unsigned long key = ::make_key(options, binary_scales);

        return result[key];
    }

    efyj::scale_id solver_bigmem::solve(std::size_t options)
    {
        return result[options];
    }

    result_type solver_bigmem::solve(const std::string& options)
    {
        std::vector <std::string> todo =
            efyj::make_string_options(options, basic_attribute_scale_size);

        result_type ret;

        std::transform(todo.cbegin(), todo.cend(),
                       std::inserter(ret, ret.end()),
                       [this](const std::string& opt)
                       {
                           unsigned long key = ::make_key(opt, binary_scales);
                           scale_id r = result[key];

                           if (not is_valid_scale_id(r))
                               throw solver_error(
                                   stringf("bigmem: Unknown result for key: %"
                                           PRIuMAX, static_cast
                                           <std::uintmax_t>(key)));

                           return r;
                       });

        return std::move(ret);
    }
}
