/* Copyright (C) 2015 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef INRA_EFYj_OPTIONS_HPP
#define INRA_EFYj_OPTIONS_HPP

#include <efyj/efyj.hpp>
#include <efyj/types.hpp>
#include <boost/container/flat_map.hpp>

namespace efyj {

struct EFYJ_API OptionId {
    OptionId(const std::string &simulation, const std::string &place,
             int deparment, int year, int observated);

    std::string simulation;
    std::string place;
    int department;
    int year;
    int observated;
};

/** @e The Options class stores the complete option file: a vector of
 * @e OptionId and an array of integet that correspond to option.
 */
struct EFYJ_API Options {
    std::vector <OptionId> ids;
    Array options;

    /** @e ordered stores the link between and OptionId (id is place,
     * departement and year) and a list of another OptionId where simulation,
     * place, departement and year are different.
     */
    boost::container::flat_multimap <int, int> ordered;
};

EFYJ_API
Options array_options_read(Context ctx, std::istream &is,
                           const efyj::Model &model);

EFYJ_API
std::ostream& operator<<(std::ostream& os, const OptionId&);

EFYJ_API
std::ostream& operator<<(std::ostream& os, const Options&);

}

#endif
