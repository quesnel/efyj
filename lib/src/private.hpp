/* Copyright (C) 2016 INRA
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

#ifndef ORG_VLEPROJECT_EFYJ_PRIVATE_HPP
#define ORG_VLEPROJECT_EFYJ_PRIVATE_HPP

#include <cstdarg>
#include <cstdio>

namespace efyj {

enum class log_level
{
    LOG_EMERG,   // system is unusable
    LOG_ALERT,   // action must be taken immediately
    LOG_CRIT,    // critical conditions
    LOG_ERR,     // error conditions
    LOG_WARNING, // warning conditions
    LOG_NOTICE,  // normal but significant condition
    LOG_INFO,    // informational
    LOG_DEBUG    // debug-level messages
};

#define EFYJ_LOG_EMERG 0   // system is unusable
#define EFYJ_LOG_ALERT 1   // action must be taken immediately
#define EFYJ_LOG_CRIT 2    // critical conditions
#define EFYJ_LOG_ERR 3     // error conditions
#define EFYJ_LOG_WARNING 4 // warning conditions
#define EFYJ_LOG_NOTICE 5  // normal but significant condition
#define EFYJ_LOG_INFO 6    // informational
#define EFYJ_LOG_DEBUG 7   // debug-level messages

inline void
vInfo(const eastl::shared_ptr<context>& ctx, const char* format, ...)
{
    (void)ctx;

    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}

inline void
vInfo(const context& ctx, const char* format, ...)
{
    (void)ctx;

    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}

inline void
vErr(const eastl::shared_ptr<context>& ctx, const char* format, ...)
{
    (void)ctx;

    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

inline void
vErr(const context& ctx, const char* format, ...)
{
    (void)ctx;

    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}
}

#endif
