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

#ifndef INRA_EFYj_LOG_HPP
#define INRA_EFYj_LOG_HPP

#include "visibility.hpp"
#include <fstream>
#include <string>
#include <iostream>

#if defined __GNUC__
#define EFYJ_GCC_PRINTF(format__, args__)               \
    __attribute__ ((format (printf, format__, args__)))
#endif

namespace efyj {

struct EFYJ_API log
{
    log(const std::string& filepath, int id);

    ~log();

    operator bool() const
    {
        return ofs.good();
    }

private:
    std::ofstream ofs;
    std::streambuf *old_clog_rdbuf;
};

EFYJ_API void logf(const char* format, ...) EFYJ_GCC_PRINTF(1, 2);
EFYJ_API void debugf(const char* format, ...) EFYJ_GCC_PRINTF(1, 2);

/*
 * debugf do nothing when NDEBUG is actived.
 */
#ifdef NDEBUG
inline void debugf(const char* /*format*/, ...)
{}
#endif

}

#endif
