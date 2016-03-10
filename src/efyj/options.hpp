/* Copyright (C) 2015-2016 INRA
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
#include <efyj/context.hpp>
#include <efyj/cstream.hpp>
#include <efyj/options.hpp>
#include <efyj/model.hpp>
#include <efyj/types.hpp>

namespace efyj {

/** @e The Options class stores the complete option file. (i) A lot of
 * vectors to store simulations identifiers, places, departements, years
 * and observation, (ii) the complete matrix of option and a ordered
 * structure to build link between simulations.
 */
struct EFYJ_API Options
{
    std::vector <std::string> simulations;
    std::vector <std::string> places;
    std::vector <int> departments;
    std::vector <int> years;
    std::vector <int> observated;
    Array options;

    /** @e ordered stores the link between and OptionId (id is place,
     * departement and year) and a list of another OptionId where
     * simulation, place, departement and year are different.
     */
    std::vector<std::vector<int>> ordered;

    /** Reads CSV from the input stream and ensures correspondence between
     * the readed data and the model.
     *
     * @param context use to log message if necessary.
     * @param [in] is input stream where read the CSV data.
     * @param [in] model to ensure correspondence.
     *
     * @throw std::bad_alloc or csv_parser_error.
     */
    void read(std::shared_ptr<Context> context,
              std::istream& is,
              const Model& model);

    /** Release all dynamically allocated memory. */
    void clear() noexcept;
};

EFYJ_API
cstream& operator<<(cstream& os, const Options&) noexcept;

}

#endif
