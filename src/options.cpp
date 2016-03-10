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
#include "exception.hpp"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <iterator>
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
    assert(options.simulations.size() == options.departments.size() &&
           options.simulations.size() == options.years.size() &&
           options.simulations.size() == options.observated.size() &&
           (options.simulations.size() == options.places.size() ||
            options.places.empty()) &&
           options.simulations.size() ==
           static_cast<std::size_t>(options.options.rows()));

    const std::size_t size = options.simulations.size();

    if (not options.ordered.empty())
        std::vector<std::vector<int>>().swap(options.ordered);

    options.ordered.resize(size);

    for (std::size_t i = 0; i != size; ++i)
        for (std::size_t j = 0; j != size; ++j)
            if (i != j
                and options.departments[i] != options.departments[j]
                and options.years[i] != options.years[j])
                options.ordered[i].emplace_back(j);
}

} // anonymous namespace

namespace efyj {

cstream&
operator<<(cstream &os,
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

cstream&
operator<<(cstream &os, const Options &options) noexcept
{
    os << "option identifiers\n------------------\n";

    if (not options.places.empty()) {
        for (std::size_t i = 0, e = options.simulations.size(); i != e; ++i) {
            os << i << options.simulations[i]
               << options.places[i] << '.'
               << options.departments[i] << '.'
               << options.years[i] << '.'
               << options.observated[i] << "\n";
        }
    } else {
        for (std::size_t i = 0, e = options.simulations.size(); i != e; ++i) {
            os << i << options.simulations[i]
               << options.departments[i] << '.'
               << options.years[i] << '.'
               << options.observated[i] << "\n";
        }
    }

    std::ostringstream ss;
    ss << options.options;

    return os << "\noption matrix\n-------------\n" << ss.str()
        << "\nordered option\n--------------\n" << options.ordered
        << "\n";
}

void Options::read(std::shared_ptr<Context> context,
                   std::istream& is,
                   const efyj::Model& model)
{
    std::vector <const efyj::attribute *> atts = ::get_basic_attribute(model);
    std::vector <int> convertheader(atts.size(), 0);
    std::vector <std::string> columns;
    std::string line;
    int id = -1;

    if (is) {
        std::getline(is, line);
        boost::algorithm::split(columns, line, boost::algorithm::is_any_of(";"));

        if (columns.size() == atts.size() + 4)
            id = 3;
        else if (columns.size() == atts.size() + 5)
            id = 4;
        else
            throw efyj::csv_parser_error(
                0, 0, std::string(),
                (boost::format(
                    "csv have not correct number of column %1% "
                    "(expected: %2%)") % columns.size() %
                 (atts.size() + 5u)).str());
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

        boost::algorithm::split(columns, line, boost::algorithm::is_any_of(";"));
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
        } catch (const efyj_error &e) {
            context->err().printf("Options: error in csv file line %d:"
                                  " convertion failure of `%s'\n",
                                  line_number,
                                  columns.back().c_str());
            continue;
        }

        if (obs < 0) {
            context->err().printf("Options: error in csv file line %d:"
                                  " fail to convert observated `%s'\n",
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
        observated.push_back(obs);

        for (std::size_t i = id, e = id + atts.size(); i != e; ++i) {
            std::size_t attid = convertheader[i - id];
            int option = atts[attid]->scale.find_scale_value(columns[i]);

            if (option < 0) {
                context->err().printf("Options: error in csv file line %d: "
                                      "unknown scale value `%s' for attribute `%s'",
                                      line_number,
                                      columns[i].c_str(),
                                      atts[attid]->name.c_str());

                simulations.pop_back();
                if (id == 4)
                    places.pop_back();
                departments.pop_back();
                years.pop_back();
                observated.pop_back();

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

    assert(static_cast<std::size_t>(options.rows()) == simulations.size());

    ::build_ordered_options(*this);
}

void Options::clear() noexcept
{
    std::vector <std::string>().swap(simulations);
    std::vector <std::string>().swap(places);
    std::vector <int>().swap(departments);
    std::vector <int>().swap(years);
    std::vector <int>().swap(observated);

    Array().swap(options);
    std::vector<std::vector<int>>().swap(ordered);
}

} // namespace efyj
