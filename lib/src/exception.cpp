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

#include "exception.hpp"
#include "utils.hpp"

namespace {

std::string csv_parser_error_format(std::size_t line,
                                    std::size_t column,
                                    const std::string &filepath,
                                    const std::string &msg)
{
    if (filepath.empty())
        return efyj::stringf("CSV Error: %s", msg.c_str());

    if (line == 0u)
        return efyj::stringf("CSV Error: file `%s': %s",
                             filepath.c_str(),
                             msg.c_str());
    if (column == 0u)
        return efyj::stringf("CSV Error: file `%s' line %ld: %s",
                             filepath.c_str(),
                             line,
                             msg.c_str());

    return efyj::stringf("CSV Error: file `%s' %ld:%ld: %s",
                         filepath.c_str(),
                         line,
                         column,
                         msg.c_str());
}

std::string xml_parser_error_format(const std::string &msg)
{
    return efyj::stringf("DEXI error: %s", msg.c_str());
}

std::string xml_parser_error_format(const std::string &msg,
                                    const std::string &filepath)
{
    return efyj::stringf("DEXI error: '%s' %s", filepath.c_str(), msg.c_str());
}

std::string xml_parser_error_format(const std::string &msg, int line,
                                    int column, int error)
{
    return efyj::stringf("DEXI error: error %s at %d:%d, error code: %d",
                         msg.c_str(), line, column, error);
}

std::string solver_error_format(const std::string &msg)
{
    return efyj::stringf("Solver error: %s", msg.c_str());
}

std::string solver_option_error_format(const std::string &opt)
{
    return efyj::stringf("Solver error: option `%s' is unknown", opt.c_str());
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
