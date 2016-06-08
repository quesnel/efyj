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

#include "options.hpp"
#include "utils.hpp"
#include <boost/algorithm/string.hpp>
#include <iterator>
#include <fstream>
#include <string>
#include <istream>

namespace
{

std::vector<const efyj::attribute *>
get_basic_attribute(const efyj::Model &model)
{
    std::vector<const efyj::attribute *> ret;
    ret.reserve(model.attributes.size());

    for (const auto &att : model.attributes)
        if (att.is_basic())
            ret.emplace_back(&att);

    return ret;
}

std::size_t
get_basic_attribute_id(const std::vector<const efyj::attribute *> &att,
                       const std::string &name)
{
    auto it = std::find_if(
        att.begin(),
        att.end(),
        [&name](const efyj::attribute *att) { return att->name == name; });

    if (it == att.end())
        throw efyj::csv_parser_error(
            efyj::stringf("unknown attribute `%s' in model", name.c_str()));

    return it - att.begin();
}

} // anonymous namespace

namespace efyj
{

void Options::read(std::shared_ptr<Context> context,
                   std::istream &is,
                   const Model &model)
{
    clear();

    std::vector<const attribute *> atts = ::get_basic_attribute(model);
    std::vector<int> convertheader(atts.size(), 0);
    std::vector<std::string> columns;
    std::string line;
    int id = -1;

    if (is) {
        std::getline(is, line);
        boost::algorithm::split(
            columns, line, boost::algorithm::is_any_of(";"));

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
        context->info() << "column " << i << ' ' << columns[i] << '\n';

    for (std::size_t i = id, e = id + atts.size(); i != e; ++i) {
        context->info() << "try to get_basic_atribute_id " << i << " : "
                        << columns[i] << '\n';
        convertheader[i - id] = ::get_basic_attribute_id(atts, columns[i]);
    }

    options = Eigen::ArrayXXi::Zero(1, atts.size());
    int line_number = -1;

    while (true) {
        std::getline(is, line);
        line_number++;

        if (not is)
            break;

        boost::algorithm::split(
            columns, line, boost::algorithm::is_any_of(";"));
        if (columns.size() != atts.size() + id + 1) {
            context->err().printf("Options: error in csv file line %d:"
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
        } catch (const std::runtime_error &e) {
            context->err().printf("Options: error in csv file line %d:"
                                  " convertion failure of `%s'\n",
                                  line_number,
                                  columns.back().c_str());
            continue;
        }

        if (obs < 0) {
            context->err().printf("Options: error in csv file line %d:"
                                  " fail to convert observed `%s'\n",
                                  line_number,
                                  columns.back().c_str());
            continue;
        }

        int department, year;
        try {
            year = std::stoi(columns[id - 1]);
            department = std::stoi(columns[id - 2]);
        } catch (const std::exception &e) {
            context->err().printf("Options: error in csv file line %d."
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
                context->err().printf(
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
            } else {
                options(options.rows() - 1, attid) = option;
            }
        }

        options.conservativeResize(options.rows() + 1, Eigen::NoChange_t());
    }

    options.conservativeResize(options.rows() - 1, Eigen::NoChange_t());

    init_dataset();
    check();
}

void Options::init_dataset()
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
    } else {
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
            } else {
                id_subdataset_reduced[i] = std::distance(reduced.cbegin(), it);
            }
        }
    }

    // printf("id_subdataset: [");
    // for (std::size_t i = 0, e = subdataset.size(); i != e; ++i)
    //     printf("%d ", id_subdataset_reduced[i]);
    // printf("]\n");
}

void Options::check()
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

void Options::set(const std::vector<std::string> &simulations_,
                  const std::vector<std::string> &places_,
                  const std::vector<int> &departments_,
                  const std::vector<int> &years_,
                  const std::vector<int> &observed_,
                  const std::vector<int> &options_)
{
    simulations = simulations_;
    places = places_;
    departments = departments_;
    years = years_;
    observed = observed_;

    const auto rows = simulations.size();
    const auto columns = options_.size() / rows;

    if (columns == 0)
        throw internal_error("Inconsistent array");

    options.conservativeResize(rows, columns);

    for (auto row = decltype(rows){0}; row != rows; ++row)
        for (auto col = decltype(columns){0}; col != columns; ++col)
            options(row, col) = options_[row * columns + col];

    init_dataset();
    check();
}

void Options::clear() noexcept
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

cstream &operator<<(cstream &os,
                    const std::vector<std::vector<int>> &ordered) noexcept
{
    for (std::size_t i = 0, e = ordered.size(); i != e; ++i) {
        os << i << ' ';

        for (auto j : ordered[i])
            os << j << ' ';

        os << '\n';
    }

    return os;
}

cstream &operator<<(cstream &os, const Options &options) noexcept
{
    os << "option identifiers\n------------------\n";

    if (not options.places.empty()) {
        for (std::size_t i = 0, e = options.simulations.size(); i != e; ++i) {
            os << i << options.simulations[i] << options.places[i] << '.'
               << options.departments[i] << '.' << options.years[i] << '.'
               << options.observed[i] << "\n";
        }
    } else {
        for (std::size_t i = 0, e = options.simulations.size(); i != e; ++i) {
            os << i << options.simulations[i] << options.departments[i] << '.'
               << options.years[i] << '.' << options.observed[i] << "\n";
        }
    }

    std::ostringstream ss;
    ss << options.options;

    os << "\noption matrix\n-------------\n" << ss.str();

    os << "\nordered option\n--------------\n";

    for (std::size_t i = 0, e = options.size(); i != e; ++i) {
        os << i << ' ';

        for (auto v : options.get_subdataset(i))
            os << v << ' ';

        os << '\n';
    }

    return os;
}

} // namespace efyj
