/* Copyright (C) 2014 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef INRA_EFYj_PARSER_HPP
#define INRA_EFYj_PARSER_HPP

#include "model.hpp"
#include "visibility.hpp"
#include <string>
#include <stdexcept>

namespace efyj {

struct EFYJ_API xml_parse_error : std::logic_error
{
    xml_parse_error(const std::string& msg, int line, int column, int error)
        : std::logic_error(msg), line(line), column(column),
        internal_error_code(error)
        {}

    int line, column;
    int internal_error_code;
};

/**
 * Parse an XML DEXi file and fill the @e dexi_data structure.
 *
 * @exception @e std::bad_alloc
 * @exception @e std::invalid_argument
 * @exception @e efyj::xml_parse_error
 *
 * @param filepath
 * @param dexi_data
 */
EFYJ_API void read(std::istream& is, dexi& dexi_data);

/**
 * Write an XML DEXi file fril the @e dexi_data object.
 *
 * @exception @e std::bad_alloc
 * @exception @e std::invalid_argument
 * @exception @e efyj::xml_parse_error
 *
 * @param filepath
 * @param dexi_data
 */
EFYJ_API void write(std::ostream& os, const dexi& dexi_data);

}

#endif
