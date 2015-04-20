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

#ifndef INRA_EFYj_DETAILS_OPTIONS_IMPLEMENTATION_HPP
#define INRA_EFYj_DETAILS_OPTIONS_IMPLEMENTATION_HPP

#include <efyj/exception.hpp>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <istream>

namespace efyj {
namespace options_details {

inline std::vector <const efyj::attribute *> get_basic_attribute(
    const efyj::Model &model)
{
    std::vector <const efyj::attribute *> ret;
    ret.reserve(model.attributes.size());

    for (const auto &att : model.attributes)
        if (att.is_basic())
            ret.emplace_back(&att);

    return std::move(ret);
}

inline std::size_t get_basic_attribute_id(const std::vector
        <const efyj::attribute *>
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

} // options_details namespace

inline OptionId::OptionId(const std::string &simulation_,
                          const std::string &place_,
                          int department_, int year_, int observated_)
    : simulation(simulation_)
    , place(place_)
    , department(department_)
    , year(year_)
    , observated(observated_)
{
}

inline Options array_options_read(Context ctx, std::istream &is, const efyj::Model &model)
{
    std::vector <const efyj::attribute *> atts =
        options_details::get_basic_attribute(model);
    std::vector <int> convertheader(atts.size(), 0);
    std::vector <std::string> columns;
    std::string line;
    Options ret;

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
            convertheader[i - 4] = options_details::get_basic_attribute_id(atts,
                                   columns[i]);
    }

    ret.options = Eigen::ArrayXXi::Zero(1, atts.size());
    int line_number = -1;

    while (true) {
        std::getline(is, line);
        line_number++;

        if (not is)
            break;

        boost::algorithm::split(columns, line, boost::algorithm::is_any_of(";"));

        if (columns.size() != atts.size() + 5u) {
            efyj_info(ctx,
                      boost::format(
                          "error in csv file line %1%: not correct number of column %2%"
                          " (expected: %3%)") % line_number % columns.size()
                      % (atts.size() + 5u));
            continue;
        }

        int obs;
        try {
            obs = model.attributes[0].scale.find_scale_value(
                columns[columns.size() - 1]);
        } catch (const efyj_error& e) {
            efyj_info(ctx,
                      boost::format(
                          "error in csv file line %1%: convertion failure of `%2%'")
                      % line_number % columns[columns.size() - 1]);
            continue;
        }

        if (obs == -1) {
            efyj_info(ctx,
                      boost::format(
                          "error in csv file line %1%: fail to convert observated `%2%'")
                      % line_number % columns[columns.size() - 1]);
            continue;
        }

        int department, year;

        try {
            department = std::stoi(columns[2]);
            year = std::stoi(columns[3]);
        } catch (const std::exception &e) {
            efyj_info(ctx,
                      boost::format(
                          "error in csv file line %1%: unknown id (`%2%' `%3%' `%4%'"
                          "`%5%')") % line_number % columns[0] % columns[1]
                      % columns[2] % columns[3]);;
            continue;
        }

        ret.ids.emplace_back(columns[0], columns[1], department, year, obs);

        for (std::size_t i = 4, e = 4 + atts.size(); i != e; ++i) {
            std::size_t attid = convertheader[i - 4];
            int option = atts[attid]->scale.find_scale_value(columns[i]);

            if (option < 0) {
                efyj_info(ctx,
                          boost::format(
                              "error in csv file line %1%: "
                              "unknown scale value `%2%' for attribute `%3%'")
                          % line_number % columns[i] % atts[attid]->name);
                ret.ids.pop_back();
                ret.options.conservativeResize(ret.options.rows() - 1,
                                               Eigen::NoChange_t());
                break;
            } else {
                ret.options(ret.options.rows() - 1, attid) = option;
            }
        }

        ret.options.conservativeResize(ret.options.rows() + 1, Eigen::NoChange_t());
    }

    ret.options.conservativeResize(ret.options.rows() - 1, Eigen::NoChange_t());

    efyj_info(ctx, "Options matrix:\n");
    efyj_info(ctx, ret.options);

    return std::move(ret);
}

}

#endif
