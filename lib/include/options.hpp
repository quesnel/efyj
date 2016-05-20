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

#include "efyj.hpp"
#include "context.hpp"
#include "cstream.hpp"
#include "options.hpp"
#include "model.hpp"
#include "types.hpp"

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
    std::vector <int> observed;
    Array options;

    const std::vector<int>& get_subdataset(int id) const noexcept
    {
        return subdataset[id];
    }

    const std::vector<std::vector<int>>& get_subdataset() const noexcept
    {
        return subdataset;
    }

    std::size_t size() const noexcept
    {
        return simulations.size();
    }

    int identifier(int id) const noexcept
    {
        return id_subdataset_reduced[id];
    }

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

private:
    /// \e subdataset stores a list of line that defines the learning
    /// options for each options. \e subdataset.size() equals \e
    /// simulations.size()
    std::vector<std::vector<int>> subdataset;

    /// \e id_subdataset_reduced stores indices for each options. Index
    /// may appear several times if the learning options are equals.
    std::vector<int> id_subdataset_reduced;
};

EFYJ_API
cstream& operator<<(cstream& os, const Options&) noexcept;

}

#endif
