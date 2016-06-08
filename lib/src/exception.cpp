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

#include "efyj.hpp"
#include "utils.hpp"

namespace
{

std::string internal_error_format_message(const std::string &msg)
{
    return efyj::stringf("Internal error: %s", msg.c_str());
}

std::string internal_error_format_message(const std::string &file,
                                          const std::string &function,
                                          int line,
                                          const std::string &msg)
{
    return efyj::stringf("Internal error at %s:%s line: %d, %s",
                         file.c_str(),
                         function.c_str(),
                         line,
                         msg.c_str());
}

std::string file_error_format_message(const std::string &file)
{
    return efyj::stringf("Access error to file `%s'", file.c_str());
}

std::string csv_parser_error_format(std::size_t line,
                                    std::size_t column,
                                    const std::string &filepath,
                                    const std::string &msg)
{
    if (filepath.empty())
        return efyj::stringf("CSV Error: %s", msg.c_str());

    if (line == 0u)
        return efyj::stringf(
            "CSV Error: file `%s': %s", filepath.c_str(), msg.c_str());
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

std::string dexi_parser_error_format(const std::string &msg)
{
    return efyj::stringf("DEXI error: %s", msg.c_str());
}

std::string dexi_parser_error_format(const std::string &msg,
                                     const std::string &filepath)
{
    return efyj::stringf("DEXI error: '%s' %s", filepath.c_str(), msg.c_str());
}

std::string dexi_parser_error_format(const std::string &msg,
                                     int line,
                                     int column,
                                     int error)
{
    return efyj::stringf("DEXI error: error %s at %d:%d, error code: %d",
                         msg.c_str(),
                         line,
                         column,
                         error);
}

std::string solver_error_format(const std::string &msg)
{
    return efyj::stringf("Solver error: %s", msg.c_str());
}

} // anonymous namespace

namespace efyj
{

internal_error::internal_error(const std::string &msg)
    : std::logic_error(::internal_error_format_message(msg))
{
}

internal_error::internal_error(const std::string &file,
                               const std::string &function,
                               int line,
                               const std::string &msg)
    : std::logic_error(
          ::internal_error_format_message(file, function, line, msg))
    , pp_file(file)
    , pp_function(function)
    , pp_line(line)
{
}

std::string internal_error::file() const { return pp_file; }

std::string internal_error::function() const { return pp_function; }

int internal_error::line() const { return pp_line; }

file_error::file_error(const std::string &file)
    : std::runtime_error(::file_error_format_message(file))
    , pp_file(file)
{
}

std::string file_error::file() const { return pp_file; }

solver_error::solver_error(const std::string &msg)
    : std::runtime_error(::solver_error_format(msg))
{
}

dexi_parser_error::dexi_parser_error(const std::string &msg)
    : std::runtime_error(::dexi_parser_error_format(msg))
    , m_line(0)
    , m_column(0)
    , m_internal_error_code(0)
    , m_message(msg)
{
}

dexi_parser_error::dexi_parser_error(const std::string &msg,
                                     const std::string &filepath)
    : std::runtime_error(::dexi_parser_error_format(filepath, msg))
    , m_line(0)
    , m_column(0)
    , m_internal_error_code(0)
    , m_filepath(filepath)
    , m_message(msg)
{
}

dexi_parser_error::dexi_parser_error(const std::string &msg,
                                     int line,
                                     int column,
                                     int error)
    : std::runtime_error(::dexi_parser_error_format(msg, line, column, error))
    , m_line(line)
    , m_column(column)
    , m_internal_error_code(error)
    , m_message(msg)
{
}

csv_parser_error::csv_parser_error(const std::string &msg)
    : std::runtime_error(::csv_parser_error_format(0, 0, std::string(), msg))
    , m_line(0)
    , m_column(0)
    , m_msg(msg)
{
}

csv_parser_error::csv_parser_error(const std::string &filepath,
                                   const std::string &msg)
    : std::runtime_error(::csv_parser_error_format(0, 0, filepath, msg))
    , m_line(0)
    , m_column(0)
    , m_filepath(filepath)
    , m_msg(msg)
{
}

csv_parser_error::csv_parser_error(std::size_t line,
                                   const std::string &filepath,
                                   const std::string &msg)
    : std::runtime_error(::csv_parser_error_format(line, 0, filepath, msg))
    , m_line(line)
    , m_column(0)
    , m_filepath(filepath)
    , m_msg(msg)
{
}

csv_parser_error::csv_parser_error(std::size_t line,
                                   std::size_t column,
                                   const std::string &filepath,
                                   const std::string &msg)
    : std::runtime_error(
          ::csv_parser_error_format(line, column, filepath, msg))
    , m_line(line)
    , m_column(column)
    , m_filepath(filepath)
    , m_msg(msg)
{
}

} // namespace efyj
