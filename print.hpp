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

#ifndef INRA_EFYj_PRINT_HPP
#define INRA_EFYj_PRINT_HPP

#include "visibility.hpp"
#include <string>
#include <cstdint>
#include <cinttypes>

#define dWHITE "\x1b[37;1m"
#define dRED "\x1b[31;1m"
#define dYELLOW "\x1b[33;1m"
#define dCYAN "\x1b[36;1m"
#define dNORMAL "\x1b[0;m"

#if defined __GNUC__
#define EFYJ_GCC_PRINTF(format__, args__) __attribute__ ((format (printf, format__, args__)))
#endif

namespace efyj {

    EFYJ_API std::string stringf(const char* format, ...) EFYJ_GCC_PRINTF(1, 2);

}

#endif

