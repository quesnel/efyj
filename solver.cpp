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

namespace efyj {

    std::vector <std::string> make_string_options(
        const std::string& options,
        const std::vector <scale_id>& max_options)
    {
        if (options.size() != max_options.size())
            throw solver_option_error("solver: options and max_options have"
                                      " different size");

        std::string cnv(options.size(), '\0');
        std::vector <efyj::scale_id> id_in_cnv; /* id of '*' in options. */
        std::vector <efyj::scale_id> value_in_cnv; /* current value of '*'. */
        std::vector <std::string> ret;

        {
            std::size_t ret_size = 1;

            for (std::size_t i = 0, e = options.size(); i != e; ++i) {
                if (options[i] == '*') {
                    cnv[i] = '0';
                    id_in_cnv.emplace_back(i);
                    value_in_cnv.emplace_back(0);

                    ret_size *= max_options[i];
                } else {
                    cnv[i] = options[i];
                }
            }
            ret.reserve(ret_size);
        }

        if (id_in_cnv.empty()) {
            ret.emplace_back(cnv);
        } else {
            bool end = false;

            do {
                for (std::size_t i = 0, e = id_in_cnv.size(); i != e; ++i)
                    cnv[id_in_cnv[i]] = '0' + value_in_cnv[i];

                ret.emplace_back(cnv);

                std::size_t current = value_in_cnv.size() - 1;
                do {
                    value_in_cnv[current]++;
                    if (value_in_cnv[current] >= max_options[current]) {
                        value_in_cnv[current] = 0;

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

        return std::move(ret);
    }

    std::vector <std::vector <scale_id>> make_scale_id_options(
        const std::string& options,
        const std::vector <scale_id>& max_options)
    {
        if (options.size() != max_options.size())
            throw solver_option_error("solver: options and max_options have"
                                      " different size");

        std::vector <efyj::scale_id> cnv(options.size(), 0);
        std::vector <efyj::scale_id> id_in_cnv; /* id of '*' in options. */
        std::vector <efyj::scale_id> value_in_cnv; /* current value of '*'. */
        std::vector <std::vector <scale_id>> ret;

        {
            std::size_t ret_size = 1;

            for (std::size_t i = 0, e = options.size(); i != e; ++i) {
                if (options[i] == '*') {
                    cnv[i] = 0;
                    id_in_cnv.emplace_back(i);
                    value_in_cnv.emplace_back(0);

                    ret_size *= max_options[i];
                } else {
                    cnv[i] = options[i] - '0';
                }
            }
            ret.reserve(ret_size);
        }

        if (id_in_cnv.empty()) {
            ret.emplace_back(cnv);
        } else {
            bool end = false;

            do {
                for (std::size_t i = 0, e = id_in_cnv.size(); i != e; ++i)
                    cnv[id_in_cnv[i]] = value_in_cnv[i];

                ret.emplace_back(cnv);

                std::size_t current = value_in_cnv.size() - 1;
                do {
                    value_in_cnv[current]++;
                    if (value_in_cnv[current] >= max_options[current]) {
                        value_in_cnv[current] = 0;

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

        return std::move(ret);
    }
}

