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

#include <algorithm>
#include <string>

#include "options.hpp"
#include "private.hpp"
#include "utils.hpp"

#include <cassert>
#include <cstdio>

namespace efyj {

static std::vector<const attribute*>
get_basic_attribute(const Model& model)
{
    std::vector<const attribute*> ret;
    ret.reserve(model.attributes.size());

    for (const auto& att : model.attributes)
        if (att.is_basic())
            ret.emplace_back(&att);

    return ret;
}

static std::optional<int>
get_basic_attribute_id(const std::vector<const attribute*>& att,
                       const std::string& name)
{
    for (size_t i = 0, e = att.size(); i != e; ++i)
        if (att[i]->name == name)
            return std::make_optional(static_cast<int>(i));

    return std::nullopt;
}

struct line_reader
{
    line_reader(FILE* is_)
      : is(is_)
    {}

    bool is_end() const
    {
        return feof(is) || ferror(is);
    }

    std::optional<std::string> getline()
    {
        // If the next line is already available in the m_buffer, we return a
        // substring of the m_buffer and update the m_buffer.

        auto newline_pos = m_buffer.find_first_of('\n');
        if (newline_pos != std::string::npos) {
            auto ret = m_buffer.substr(0, newline_pos);

            if (newline_pos >= m_buffer.size())
                m_buffer.clear();
            else
                m_buffer = m_buffer.substr(newline_pos + 1, std::string::npos);

            return std::make_optional(ret);
        }

        // We need to append data to the buffer.

        char buffer[BUFSIZ];

        do {
            if (is_end()) {
                m_buffer.clear();
                return {};
            }

            auto len = fread(buffer, 1, BUFSIZ - 1, is);
            buffer[len] = '\0';

            if (len == 0) {
                auto ret = std::move(m_buffer);
                ret += '\n';

                return std::make_optional(ret);
            } else {
                auto* newline = strchr(buffer, '\n');
                if (newline == nullptr) {
                    m_buffer.append(buffer);
                } else {
                    m_buffer.append(buffer, newline);
                    auto ret = std::move(m_buffer);
                    m_buffer.assign(newline + 1);

                    return std::make_optional(ret);
                }
            }
        } while (!is_end());

        auto ret = std::move(m_buffer);
        ret += '\n';

        return std::make_optional(ret);
    }

    FILE* is;
    std::string m_buffer;
};

#if 0
Options::Options(const data& d)
  : simulations(d.simulations)
  , places(d.places)
  , departments(d.departments)
  , years(d.years)
  , observed(d.observed)
{
    fmt::print("Options::Options(const data& d)\n");
    const auto rows = simulations.size();
    const auto nb_basic_attributes = d.scale_values.size() / rows;

    fmt::print("{} {}\n", rows, nb_basic_attributes);
    options.init(rows, nb_basic_attributes);

    for (size_t r = 0, i = 0; r < rows; ++r) {
        for (size_t c = 0; c < nb_basic_attributes; ++c, ++i) {
            fmt::print("set {},{} = {}\n", r, c, d.scale_values[i]);
            options(r, c) = d.scale_values[i];
        }
    }

    fmt::print("init_dataset()\n");
    init_dataset();
    fmt::print("check\n");
    check();
    fmt::print("Options::Options finished\n");
}
#endif

void
Options::save(const char* filename) noexcept
{
    if (std::FILE* os = std::fopen(filename, "w"); os) {
        fmt::print(os, "simulations {}\n", simulations.size());
        for (const auto& elem : simulations)
            fmt::print(os, "[{}]", elem);

        fmt::print(os, "\nplaces {}\n", places.size());
        for (const auto& elem : places)
            fmt::print(os, "[{}]", elem);

        fmt::print(os, "\ndepartments {}\n", departments.size());
        for (const auto& elem : departments)
            fmt::print(os, "[{}]", elem);

        fmt::print(os, "\nyears {}\n", years.size());
        for (const auto& elem : years)
            fmt::print(os, "[{}]", elem);

        fmt::print(os, "\nobserved {}\n", observed.size());
        for (const auto& elem : observed)
            fmt::print(os, "[{}]", elem);

        fmt::print(os, "\noptions {}*{}\n", options.rows(), options.cols());
        const auto rows = options.rows();
        const auto cols = options.cols();

        for (size_t r = 0; r < rows; ++r) {
            for (size_t c = 0; c < cols; ++c)
                fmt::print(os, "{}", options(r, c));
            fmt::print(os, "\n");
        }

        fmt::print(os, "\n");
        std::fclose(os);
    }
}

csv_parser_status::tag
Options::read(const context& ctx, const input_file& is, const Model& model)
{
    clear();

    std::vector<const attribute*> atts = get_basic_attribute(model);
    std::vector<int> convertheader(atts.size(), 0);
    std::vector<std::string> columns;
    std::string line;
    size_t id;

    line_reader ls(is.get());

    {
        auto opt_line = ls.getline();
        if (!opt_line) {
            info(ctx, "Fail to read header\n");
            error_at_line = 0;
            error_at_column = columns.size();
            return csv_parser_status::tag::file_error;
        }

        line = *opt_line;

        tokenize(line, columns, ";", false);

        if (columns.size() == atts.size() + 4) {
            id = 3;
        } else if (columns.size() == atts.size() + 5) {
            id = 4;
        } else {
            error_at_line = 0;
            error_at_column = columns.size();
            return csv_parser_status::tag::column_number_incorrect;
        }
    }

    for (size_t i = 0, e = atts.size(); i != e; ++i)
        info(ctx, "column {} {}\n", i, columns[i].c_str());

    for (size_t i = id, e = id + atts.size(); i != e; ++i) {
        info(
          ctx, "try to get_basic_atribute_id {} : {}\n", i, columns[i].c_str());

        auto opt_att_id = get_basic_attribute_id(atts, columns[i]);
        if (!opt_att_id) {
            error(ctx, "Fail to found attribute for `{}'\n", columns[i]);
            error_at_line = 0;
            error_at_column = columns.size();
            return csv_parser_status::tag::basic_attribute_unknown;
        }

        convertheader[i - id] = *opt_att_id;
    }

    info(ctx, "Starts to read data (atts.size() = {}\n", atts.size());

    options.init(atts.size());
    options.push_line();
    int line_number = -1;

    while (true) {
        auto opt_line = ls.getline();
        if (!opt_line)
            break;

        line = *opt_line;
        line_number++;

        tokenize(line, columns, ";", false);
        if (columns.size() != atts.size() + id + 1) {
            error(ctx,
                  "Options: error in csv file line {}:"
                  " not correct number of column {}"
                  " (expected: {})\n",
                  line_number,
                  columns.size(),
                  atts.size() + id + 1);
            continue;
        }

        auto opt_obs =
          model.attributes[0].scale.find_scale_value(columns.back());

        if (!opt_obs) {
            error(
              ctx,
              "Options: error in csv file line {}: unknown scale value `{}'\n",
              line_number,
              columns.back());

            error_at_line = line_number;
            error_at_column = columns.size();
            return csv_parser_status::tag::scale_value_unknown;
        }

        int obs = *opt_obs;
        int department, year;

        {
            auto len1 = sscanf(columns[id - 1].c_str(), "%d", &year);
            auto len2 = sscanf(columns[id - 1].c_str(), "%d", &department);

            if (len1 != 1 || len2 != 1) {
                error(ctx,
                      "Options: error in csv file line {}."
                      " Malformed year or department\n",
                      line_number);
                continue;
            }
        }

        simulations.push_back(columns[0]);
        if (id == 4)
            places.push_back(columns[1]);

        departments.push_back(department);
        years.push_back(year);
        observed.push_back(obs);

        for (size_t i = id, e = id + atts.size(); i != e; ++i) {
            size_t attid = convertheader[i - id];

            auto opt_option = atts[attid]->scale.find_scale_value(columns[i]);
            if (!opt_option) {
                error(ctx,
                      "Options: error in csv file line {}: "
                      "unknown scale value `{}' for attribute `{}'\n",
                      line_number,
                      columns[i].c_str(),
                      atts[attid]->name.c_str());

                error_at_line = line_number;
                error_at_column = columns.size();
                return csv_parser_status::tag::scale_value_unknown;
            }

            options(options.rows() - 1, attid) = *opt_option;
        }

        options.push_line();
    }

    options.pop_line();

    init_dataset();
    check();

    return csv_parser_status::tag::done;
}

void
Options::init_dataset()
{
    const size_t size = simulations.size();

    assert(!simulations.empty());

    subdataset.resize(size);
    for (auto& elem : subdataset)
        elem.clear();

    if (places.empty()) {
        for (size_t i = 0; i != size; ++i) {
            for (size_t j = 0; j != size; ++j) {
                if (i != j && departments[i] != departments[j] &&
                    years[i] != years[j]) {
                    subdataset[i].emplace_back(static_cast<int>(j));
                }
            }
        }
    } else {
        for (size_t i = 0; i != size; ++i) {
            for (size_t j = 0; j != size; ++j) {
                if (i != j && departments[i] != departments[j] &&
                    places[i] != places[j] && years[i] != years[j]) {
                    subdataset[i].emplace_back(static_cast<int>(j));
                }
            }
        }
    }

    fmt::print("init_dataset\n");
    for (size_t i = 0, e = subdataset.size(); i != e; ++i) {
        fmt::print("{} [", i);
        for (auto elem : subdataset[i])
            fmt::print("{} ", elem);
        fmt::print("]\n");
    }

    {
        std::vector<std::vector<int>> reduced;
        id_subdataset_reduced.resize(subdataset.size(), 0);

        for (size_t i = 0, e = subdataset.size(); i != e; ++i) {
            auto it =
              std::find(reduced.cbegin(), reduced.cend(), subdataset[i]);

            if (it == reduced.cend()) {
                id_subdataset_reduced[i] = (int)reduced.size();
                reduced.push_back(subdataset[i]);
            } else {
                id_subdataset_reduced[i] =
                  numeric_cast<int>(std::distance(reduced.cbegin(), it));
            }
        }
    }

    fmt::print("id_subdataset: [");
    for (size_t i = 0, e = subdataset.size(); i != e; ++i)
        fmt::print("{} ", id_subdataset_reduced[i]);
    fmt::print("]\n");
}

bool
Options::check()
{
    return !(static_cast<size_t>(options.rows()) != simulations.size() ||
             options.cols() == 0 || simulations.size() != departments.size() ||
             simulations.size() != years.size() ||
             simulations.size() != observed.size() ||
             !(simulations.size() == places.size() || places.empty()) ||
             simulations.size() != id_subdataset_reduced.size() ||
             subdataset.size() != simulations.size());
}

void
Options::clear() noexcept
{
    std::vector<std::string>().swap(simulations);
    std::vector<std::string>().swap(places);
    std::vector<int>().swap(departments);
    std::vector<int>().swap(years);
    std::vector<int>().swap(observed);

    DynArray().swap(options);
    std::vector<std::vector<int>>().swap(subdataset);
    std::vector<int>().swap(id_subdataset_reduced);
}
} // namespace efyj
