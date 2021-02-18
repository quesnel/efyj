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

#ifndef ORG_VLEPROJECT_EFYj_OPTIONS_HPP
#define ORG_VLEPROJECT_EFYj_OPTIONS_HPP

#include <optional>

#include <efyj/efyj.hpp>
#include <efyj/matrix.hpp>

#include "dynarray.hpp"
#include "model.hpp"

namespace efyj {

/** @e The Options class stores the complete option file. (i) A lot of
 * vectors to store simulations identifiers, places, departements, years and
 * observation, (ii) the complete matrix of option and a ordered structure to
 * build link between simulations.
 */
class Options
{
public:
    std::vector<std::string> simulations;
    std::vector<std::string> places;
    std::vector<int> departments;
    std::vector<int> years;
    std::vector<int> observed;
    DynArray options;

    Options() = default;

    Options(const data& d);

    const std::vector<int>& get_subdataset(int id) const noexcept
    {
        return subdataset[id];
    }

    const std::vector<std::vector<int>>& get_subdataset() const noexcept
    {
        return subdataset;
    }

    size_t size() const noexcept
    {
        return simulations.size();
    }

    int identifier(int id) const noexcept
    {
        return id_subdataset_reduced[id];
    }

    bool empty() const noexcept
    {
        return simulations.empty() || departments.empty() || years.empty() ||
               observed.empty();
    }

    void set(const options_data& options);

    /** Reads CSV from the input stream and ensures correspondence between
     * the readed data and the model.
     *
     * @param context use to log message if necessary.
     * @param [in] is input stream where read the CSV data.
     * @param [in] model to ensure correspondence.
     *
     * @throw std::bad_alloc or csv_parser_error.
     */
    void read(const context& ctx, const input_file& is, const Model& model);

    bool have_subdataset() const
    {
        for (const auto& elem : subdataset)
            if (elem.empty())
                return false;

        return true;
    }

    /** Release all dynamically allocated memory. */
    void clear() noexcept;

    /// check consistency of all data into the object. This function is
    /// automatically used after \e read(...) or \e set(...) functions. If
    /// data are not consistency, \e clear() is called.
    ///
    /// \execption \e internal_error or \e options_error.
    void check();

    /// \e init_dataset is called after \e read(...) or \e set(...)
    /// functions to initialize the \e subdataset and \e
    /// id_subdataset_reduced variables.
    void init_dataset();

    /// \e subdataset stores a list of line that defines the learning
    /// options for each options. \e subdataset.size() equals \e
    /// simulations.size()
    std::vector<std::vector<int>> subdataset;

    /// \e id_subdataset_reduced stores indices for each options. Index
    /// may appear several times if the learning options are equals.
    std::vector<int> id_subdataset_reduced;
};
}

#endif
