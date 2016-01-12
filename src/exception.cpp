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

#include "exception.hpp"
#include <boost/format.hpp>

namespace {

std::string csv_parser_error_format(std::size_t line,
                                    std::size_t column,
                                    const std::string &filepath,
                                    const std::string &msg)
{
    if (filepath.empty()) {
        return boost::str(
                   boost::format("CSV Error: %1%") % msg);
    } else {
        if (line == 0u) {
            return boost::str(
                       boost::format(
                           "CSV Error: file `%1%': %2%") % filepath % msg);
        } else {
            if (column == 0u) {
                return boost::str(
                           boost::format(
                               "CSV Error: file `%1%' line %2%: %3%") %
                           filepath % line % msg);
            } else {
                return boost::str(
                           boost::format(
                               "CSV Error: file `%1%' %2%:%3%: %4%") %
                           filepath % line % column % msg);
            }
        }
    }
}

std::string xml_parser_error_format(const std::string &msg)
{
    return boost::str(
               boost::format("DEXI error: %1%") % msg);
}

std::string xml_parser_error_format(const std::string &msg,
                                    const std::string &filepath)
{
    return boost::str(
               boost::format("DEXI error: %1% %2%") % filepath % msg);
}

std::string xml_parser_error_format(const std::string &msg, int line,
                                    int column, int error)
{
    return boost::str(
               boost::format("DEXI error: error %1% at %2%:%3%, %4%") % msg % line % column %
               error);
}

std::string solver_error_format(const std::string &msg)
{
    return boost::str(
               boost::format("Solver error: %1%") % msg);
}

std::string solver_option_error_format(const std::string &opt)
{
    return boost::str(
               boost::format("option `%1%' is unknown") % opt);
}

} // anonymous namespace

namespace efyj {

solver_error::solver_error(const std::string &msg)
    : efyj_error(::solver_error_format(msg))
{
}

solver_error::~solver_error()
{
}

solver_option_error::solver_option_error(const std::string &option)
    : solver_error(::solver_option_error_format(option))
{
}

solver_option_error::~solver_option_error()
{
}

xml_parser_error::xml_parser_error(const std::string &msg)
    : efyj_error(::xml_parser_error_format(msg))
    , m_line(0)
    , m_column(0)
    , m_internal_error_code(0)
    , m_message(msg)
{
}

xml_parser_error::xml_parser_error(const std::string &msg,
                                   const std::string &filepath)
    : efyj_error(::xml_parser_error_format(filepath, msg))
    , m_line(0)
    , m_column(0)
    , m_internal_error_code(0)
    , m_filepath(filepath)
    , m_message(msg)
{
}

xml_parser_error::xml_parser_error(const std::string &msg, int line, int column,
                                   int error)
    : efyj_error(::xml_parser_error_format(msg, line, column, error))
    , m_line(line)
    , m_column(column)
    , m_internal_error_code(error)
    , m_message(msg)
{
}

xml_parser_error::~xml_parser_error()
{
}

efyj_error::efyj_error(const std::string &msg)
    : std::runtime_error(msg)
{
}

efyj_error::~efyj_error()
{
}

csv_parser_error::csv_parser_error(const std::string &msg)
    : efyj_error(::csv_parser_error_format(0, 0, std::string(), msg))
    , m_line(0)
    , m_column(0)
    , m_msg(msg)
{
}

csv_parser_error::csv_parser_error(const std::string &filepath,
                                   const std::string &msg)
    : efyj_error(::csv_parser_error_format(0, 0, filepath, msg))
    , m_line(0)
    , m_column(0)
    , m_filepath(filepath)
    , m_msg(msg)
{
}

csv_parser_error::csv_parser_error(std::size_t line,
                                   const std::string &filepath,
                                   const std::string &msg)
    : efyj_error(::csv_parser_error_format(line, 0, filepath, msg))
    , m_line(line)
    , m_column(0)
    , m_filepath(filepath)
    , m_msg(msg)
{
}

csv_parser_error::csv_parser_error(std::size_t line, std::size_t column,
                                   const std::string &filepath,
                                   const std::string &msg)
    : efyj_error(::csv_parser_error_format(line, column, filepath, msg))
    , m_line(line)
    , m_column(column)
    , m_filepath(filepath)
    , m_msg(msg)
{
}

csv_parser_error::~csv_parser_error()
{
}

} // namespace efyj
