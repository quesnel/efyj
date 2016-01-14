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

#include "options.hpp"
#include "exception.hpp"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <istream>

namespace {

std::vector <const efyj::attribute *>
get_basic_attribute(const efyj::Model &model)
{
    std::vector <const efyj::attribute *> ret;
    ret.reserve(model.attributes.size());

    for (const auto &att : model.attributes)
        if (att.is_basic())
            ret.emplace_back(&att);

    return std::move(ret);
}

std::size_t
get_basic_attribute_id(
    const std::vector <const efyj::attribute *>
    &att,
    const std::string &name)
{
    auto it = std::find_if(att.begin(),
    att.end(), [&name](const efyj::attribute * att) {
        return att->name == name;
    });

    if (it == att.end())
        throw efyj::csv_parser_error(
            (boost::format("unknown attribute `%1%' in model") % name).str());

    return it - att.begin();
}

void
build_ordered_options(efyj::Options &options) noexcept
{
    assert(options.ids.size() == options.options.rows());

    for (std::size_t i = 0, end_i = options.ids.size(); i != end_i; ++i) {
        auto it = options.ordered.end();

        for (std::size_t j = 0, end_j = options.ids.size(); j != end_j; ++j) {
            if (i != j and
                options.ids[i].department != options.ids[j].department and
                options.ids[i].year != options.ids[j].year) {
                if (it == options.ordered.end())
                    it = options.ordered.emplace(i, j);
                else
                    options.ordered.emplace_hint(it, i, j);
            }
        }
    }
}

} // anonymous namespace

namespace efyj {

OptionId::OptionId(const std::string &simulation_,
                   const std::string &place_,
                   int department_, int year_, int observated_)
    : simulation(simulation_)
    , place(place_)
    , department(department_)
    , year(year_)
    , observated(observated_)
{
}

cstream &
operator<<(cstream &os, const OptionId &optionid) noexcept
{
    return os << optionid.simulation << ' '
        << optionid.place << ' '
        << optionid.department << ' '
        << optionid.year;
}

cstream &
operator<<(cstream &os,
           const boost::container::flat_multimap <int, int> &map) noexcept
{
    if (not map.empty()) {
        auto previous = map.cbegin();
        os << previous->first << ' ' << previous->second;

        if (map.size() > 2) {
            auto it = previous + 1;

            while (it != map.cend()) {
                if (it->first != previous->first) {
                    os << '\n' << it->first << ' ' << it->second;
                } else {
                    os << ' ' << it->second;
                }

                previous = it;
            }
        }
    }

    return os;
}

cstream &
operator<<(cstream &os, const Options &options) noexcept
{
    os << "option identifiers\n------------------\n";

    for (std::size_t i = 0, e = options.ids.size(); i != e;  ++i)
        os << i << " " << options.ids[i] << "\n";

    std::ostringstream ss;
    ss << options.options;

    return os << "\noption matrix\n-------------\n" << ss.str()
        << "\nordered option\n--------------\n" << options.ordered
        << "\n";
}

void Options::read(std::istream& is, const efyj::Model& model)
{
    std::vector <const efyj::attribute *> atts =
        ::get_basic_attribute(model);
    std::vector <int> convertheader(atts.size(), 0);
    std::vector <std::string> columns;
    std::string line;

    if (is) {
        std::getline(is, line);
        boost::algorithm::split(columns, line, boost::algorithm::is_any_of(";"));

        if (columns.size() != atts.size() + 5u)
            throw efyj::csv_parser_error(
                0, 0, std::string(),
                (boost::format(
                        "csv file have not correct number of column %1% "
                        "(expected: %2%)") % columns.size() %
                    (atts.size() + 5u)).str());

        for (std::size_t i = 4, e = 4 + atts.size(); i != e; ++i)
	    convertheader[i - 4] = ::get_basic_attribute_id(atts, columns[i]);
    }

    options = Eigen::ArrayXXi::Zero(1, atts.size());
    int line_number = -1;

    while (true) {
        std::getline(is, line);
        line_number++;

        if (not is)
            break;

        boost::algorithm::split(columns, line, boost::algorithm::is_any_of(";"));
        if (columns.size() != atts.size() + 5u) {
            err().printf("Options: error in csv file line %d:"
                         " not correct number of column %d"
                         " (expected: %d)",
                         line_number,
                         static_cast<int>(columns.size()),
                         static_cast<int>(atts.size() + 5u));
            continue;
        }

        int obs;
        try {
            obs = model.attributes[0].scale.find_scale_value(
                columns[columns.size() - 1]);
        } catch (const efyj_error &e) {
            err().printf("Options: error in csv file line %d:"
                         " convertion failure of `%s'",
                         line_number,
                         columns[columns.size() - 1].c_str());
            continue;
        }

        if (obs == -1) {
            err().printf("Options: error in csv file line %d:"
                         " fail to convert observated `%s'",
                         line_number,
                         columns[columns.size() - 1].c_str());
            continue;
        }

        int department, year;
        try {
            department = std::stoi(columns[2]);
            year = std::stoi(columns[3]);
        } catch (const std::exception &e) {
            err().printf("Options: error in csv file line %d:"
                         " unknown id (`%s' `%s' `%s' `%5%')",
                         line_number,
                         columns[0].c_str(),
                         columns[1].c_str(),
                         columns[2].c_str(),
                         columns[3].c_str());
            continue;
        }

        ids.emplace_back(columns[0], columns[1], department, year, obs);

        for (std::size_t i = 4, e = 4 + atts.size(); i != e; ++i) {
            std::size_t attid = convertheader[i - 4];
            int option = atts[attid]->scale.find_scale_value(columns[i]);

            if (option < 0) {
                err().printf("Options: error in csv file line %d: "
                             "unknown scale value `%s' for attribute `%s'",
                             line_number,
                             columns[i].c_str(),
                             atts[attid]->name.c_str());
                ids.pop_back();
                options.conservativeResize(options.rows() - 1,
                                           Eigen::NoChange_t());
                break;
            } else {
                options(options.rows() - 1, attid) = option;
            }
        }

        options.conservativeResize(options.rows() + 1,
                                   Eigen::NoChange_t());
    }

    options.conservativeResize(options.rows() - 1,
                               Eigen::NoChange_t());

    assert(options.rows() == ids.size());

    ::build_ordered_options(*this);
}

void Options::clear() noexcept
{
    std::vector <OptionId>().swap(ids);
    Array().swap(options);
    boost::container::flat_multimap <int, int>().swap(ordered);
}

} // namespace efyj
