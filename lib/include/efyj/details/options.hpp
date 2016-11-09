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

#include <Eigen/Core>
#include <cassert>
#include <efyj/details/model.hpp>

namespace efyj {

using Array = Eigen::ArrayXXi;

/** @e The Options class stores the complete option file. (i) A lot of
 * vectors to store simulations identifiers, places, departements, years and
 * observation, (ii) the complete matrix of option and a ordered structure to
 * build link between simulations.
 */
class Options {
public:
    std::vector<std::string> simulations;
    std::vector<std::string> places;
    std::vector<int> departments;
    std::vector<int> years;
    std::vector<int> observed;
    Array options;

    const std::vector<int> &get_subdataset(int id) const noexcept
    {
        return subdataset[id];
    }

    const std::vector<std::vector<int>> &get_subdataset() const noexcept
    {
        return subdataset;
    }

    std::size_t size() const noexcept { return simulations.size(); }

    int identifier(int id) const noexcept { return id_subdataset_reduced[id]; }

    bool empty() const noexcept
    {
        return simulations.empty() or departments.empty() or years.empty() or
               observed.empty();
    }

    void set(const options_data &options);

    /** Reads CSV from the input stream and ensures correspondence between
     * the readed data and the model.
     *
     * @param context use to log message if necessary.
     * @param [in] is input stream where read the CSV data.
     * @param [in] model to ensure correspondence.
     *
     * @throw std::bad_alloc or csv_parser_error.
     */
    void read(std::shared_ptr<context> context,
              std::istream &is,
              const Model &model);

    bool have_subdataset() const
    {
        for (const auto &elem : subdataset)
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

private:
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

namespace details {

inline std::vector<const attribute *> get_basic_attribute(const Model &model)
{
    std::vector<const attribute *> ret;
    ret.reserve(model.attributes.size());

    for (const auto &att : model.attributes)
        if (att.is_basic())
            ret.emplace_back(&att);

    return ret;
}

inline std::size_t
get_basic_attribute_id(const std::vector<const attribute *> &att,
                       const std::string &name)
{
    auto it =
        std::find_if(att.begin(), att.end(), [&name](const attribute *att) {
            return att->name == name;
        });

    if (it == att.end())
        throw csv_parser_error(
            stringf("unknown attribute `%s' in model", name.c_str()));

    return it - att.begin();
}
} // namespace details

inline void Options::read(std::shared_ptr<context> context,
                          std::istream &is,
                          const Model &model)
{
    clear();

    std::vector<const attribute *> atts = details::get_basic_attribute(model);
    std::vector<int> convertheader(atts.size(), 0);
    std::vector<std::string> columns;
    std::string line;
    int id = -1;

    if (is) {
        std::getline(is, line);
        tokenize(line, columns, ";", false);

        if (columns.size() == atts.size() + 4)
            id = 3;
        else if (columns.size() == atts.size() + 5)
            id = 4;
        else
            throw csv_parser_error(
                0,
                0,
                std::string(),
                stringf("csv have not correct number of column %ld "
                        "(expected: %ld)",
                        columns.size(),
                        (atts.size() + 5u)));
    }

    for (std::size_t i = 0, e = atts.size(); i != e; ++i)
        vInfo(context, "column %zu %s\n", i, columns[i].c_str());

    for (std::size_t i = id, e = id + atts.size(); i != e; ++i) {
        vInfo(context,
              "try to get_basic_atribute_id %zu : %s\n",
              i,
              columns[i].c_str());

        convertheader[i - id] =
            details::get_basic_attribute_id(atts, columns[i]);
    }

    options = Eigen::ArrayXXi::Zero(1, atts.size());
    int line_number = -1;

    while (true) {
        std::getline(is, line);
        line_number++;

        if (not is)
            break;

        tokenize(line, columns, ";", false);
        if (columns.size() != atts.size() + id + 1) {
            vErr(context,
                 "Options: error in csv file line %d:"
                 " not correct number of column %d"
                 " (expected: %d)\n",
                 line_number,
                 static_cast<int>(columns.size()),
                 static_cast<int>(atts.size() + id + 1));
            continue;
        }

        int obs;
        try {
            obs = model.attributes[0].scale.find_scale_value(columns.back());
        }
        catch (const std::runtime_error &e) {
            vErr(context,
                 "Options: error in csv file line %d:"
                 " convertion failure of `%s'\n",
                 line_number,
                 columns.back().c_str());
            continue;
        }

        if (obs < 0) {
            vErr(context,
                 "Options: error in csv file line %d:"
                 " fail to convert observed `%s'\n",
                 line_number,
                 columns.back().c_str());
            continue;
        }

        int department, year;
        try {
            year = std::stoi(columns[id - 1]);
            department = std::stoi(columns[id - 2]);
        }
        catch (const std::exception &e) {
            vErr(context,
                 "Options: error in csv file line %d."
                 " Malformed year or department\n",
                 line_number);
            continue;
        }

        simulations.push_back(columns[0]);
        if (id == 4)
            places.push_back(columns[1]);

        departments.push_back(department);
        years.push_back(year);
        observed.push_back(obs);

        for (std::size_t i = id, e = id + atts.size(); i != e; ++i) {
            std::size_t attid = convertheader[i - id];
            int option = atts[attid]->scale.find_scale_value(columns[i]);

            if (option < 0) {
                vErr(context,
                     "Options: error in csv file line %d: "
                     "unknown scale value `%s' for attribute `%s'",
                     line_number,
                     columns[i].c_str(),
                     atts[attid]->name.c_str());

                simulations.pop_back();
                if (id == 4)
                    places.pop_back();
                departments.pop_back();
                years.pop_back();
                observed.pop_back();

                options.conservativeResize(options.rows() - 1,
                                           Eigen::NoChange_t());
                break;
            }
            else {
                options(options.rows() - 1, attid) = option;
            }
        }

        options.conservativeResize(options.rows() + 1, Eigen::NoChange_t());
    }

    options.conservativeResize(options.rows() - 1, Eigen::NoChange_t());

    init_dataset();
    check();
}

inline void Options::init_dataset()
{
    const std::size_t size = simulations.size();

    assert(not simulations.empty());

    subdataset.resize(size);

    if (places.empty()) {
        for (std::size_t i = 0; i != size; ++i) {
            for (std::size_t j = 0; j != size; ++j) {
                if (i != j and departments[i] != departments[j] and
                    years[i] != years[j]) {
                    subdataset[i].emplace_back(j);
                }
            }
        }
    }
    else {
        for (std::size_t i = 0; i != size; ++i) {
            for (std::size_t j = 0; j != size; ++j) {
                if (i != j and departments[i] != departments[j] and
                    places[i] != places[j] and years[i] != years[j]) {
                    subdataset[i].emplace_back(j);
                }
            }
        }
    }

    // printf("init_dataset\n");
    // for (std::size_t i = 0, e = subdataset.size(); i != e; ++i) {
    //     printf("%ld [", i);
    //     for (auto elem : subdataset[i])
    //         printf("%d ", elem);
    //     printf("]\n");
    // }

    {
        std::vector<std::vector<int>> reduced;
        id_subdataset_reduced.resize(subdataset.size());

        for (std::size_t i = 0, e = subdataset.size(); i != e; ++i) {
            auto it =
                std::find(reduced.cbegin(), reduced.cend(), subdataset[i]);

            if (it == reduced.cend()) {
                id_subdataset_reduced[i] = (int)reduced.size();
                reduced.push_back(subdataset[i]);
            }
            else {
                id_subdataset_reduced[i] = std::distance(reduced.cbegin(), it);
            }
        }
    }

    // printf("id_subdataset: [");
    // for (std::size_t i = 0, e = subdataset.size(); i != e; ++i)
    //     printf("%d ", id_subdataset_reduced[i]);
    // printf("]\n");
}

inline void Options::check()
{
    if (static_cast<std::size_t>(options.rows()) != simulations.size() or
        options.cols() == 0 or simulations.size() != departments.size() or
        simulations.size() != years.size() or
        simulations.size() != observed.size() or
        not(simulations.size() == places.size() or places.empty()) or
        simulations.size() != id_subdataset_reduced.size() or
        subdataset.size() != simulations.size())
        throw internal_error("Options are inconsistent");
}

void Options::set(const options_data &opts)
{
    simulations = opts.simulations;
    places = opts.places;
    departments = opts.departments;
    years = opts.years;
    observed = opts.observed;

    const auto rows = opts.options.rows();
    const auto columns = opts.options.columns();
    options.conservativeResize(rows, columns);

    for (std::size_t r = 0; r != rows; ++r)
        for (std::size_t c = 0; c != columns; ++c)
            options(r, c) = opts.options(c, r);

    init_dataset();
    check();
}

inline void Options::clear() noexcept
{
    std::vector<std::string>().swap(simulations);
    std::vector<std::string>().swap(places);
    std::vector<int>().swap(departments);
    std::vector<int>().swap(years);
    std::vector<int>().swap(observed);

    Array().swap(options);
    std::vector<std::vector<int>>().swap(subdataset);
    std::vector<int>().swap(id_subdataset_reduced);
}
}

#endif
