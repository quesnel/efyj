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

#ifndef INRA_EFYj_EXCEPTION_HPP
#define INRA_EFYj_EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace efyj {

class efyj_error : public std::runtime_error
{
public:
    efyj_error(const std::string &msg);

    virtual ~efyj_error();
};

class solver_error : public efyj_error
{
public:
    solver_error(const std::string &msg);

    virtual ~solver_error();
};

class solver_option_error : solver_error
{
public:
    solver_option_error(const std::string &msg);

    virtual ~solver_option_error();
};

class xml_parser_error : public efyj_error
{
public:
    xml_parser_error(const std::string &msg);

    xml_parser_error(const std::string &msg, const std::string &filepath);

    xml_parser_error(const std::string &msg, int line, int column, int error);

    virtual ~xml_parser_error();

    int line() const { return m_line; }
    int column() const { return m_column; }
    int internal_error_code() const { return m_internal_error_code; }
    std::string filepath() const { return m_filepath; }
    std::string message() const { return m_message; }

private:
    int m_line, m_column;
    int m_internal_error_code;
    std::string m_filepath;
    std::string m_message;
};

class csv_parser_error : public efyj_error
{
public:
    csv_parser_error(const std::string &msg);

    csv_parser_error(const std::string &filepath, const std::string &msg);

    csv_parser_error(std::size_t line, const std::string &filepath,
                     const std::string &msg);

    csv_parser_error(std::size_t line, std::size_t column,
                     const std::string &filepath,
                     const std::string &msg);

    virtual ~csv_parser_error();

    std::size_t line() const { return m_line; }
    std::size_t column() const { return m_column; }
    std::string filepath() const { return m_filepath; }
    std::string message() const { return m_msg; }

private:
    std::size_t m_line;
    std::size_t m_column;
    std::string m_filepath;
    std::string m_msg;
};

}

#endif
