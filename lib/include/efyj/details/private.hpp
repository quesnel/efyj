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

#define EFYJ_LOG_EMERG 0   // system is unusable
#define EFYJ_LOG_ALERT 1   // action must be taken immediately
#define EFYJ_LOG_CRIT 2    // critical conditions
#define EFYJ_LOG_ERR 3     // error conditions
#define EFYJ_LOG_WARNING 4 // warning conditions
#define EFYJ_LOG_NOTICE 5  // normal but significant condition
#define EFYJ_LOG_INFO 6    // informational
#define EFYJ_LOG_DEBUG 7   // debug-level messages

static inline void
#if defined(__GNUC__)
    __attribute__((always_inline, format(printf, 2, 3)))
#endif
    efyj_log_null(const std::shared_ptr<efyj::context> &, const char *, ...)
{
}

static inline void
#if defined(__GNUC__)
    __attribute__((always_inline, format(printf, 2, 3)))
#endif
    efyj_log_null(const efyj::context *, const char *, ...)
{
}

#define efyj_log_cond(ctx, prio, arg...)                                      \
    do {                                                                      \
        if (ctx->get_log_priority() >= prio) {                                \
            ctx->log(prio, __FILE__, __LINE__, __FUNCTION__, ##arg);          \
        }                                                                     \
    } while (0)

//
// Default, logging system is active and the @c vDbg() macro checks log
// priority argument. Define DISABLE_LOGGING to hide all logging message.
// Define NDEBUG to remove @c vDbg() macro.
//

#ifndef DISABLE_LOGGING
#ifndef NDEBUG
#define vDbg(ctx, arg...) efyj_log_cond(ctx, EFYJ_LOG_DEBUG, ##arg)
#else
#define vDbg(ctx, arg...) efyj_log_null(ctx, ##arg)
#endif
#define vInfo(ctx, arg...) efyj_log_cond(ctx, EFYJ_LOG_INFO, ##arg)
#define vErr(ctx, arg...) efyj_log_cond(ctx, EFYJ_LOG_ERR, ##arg)
#else
#define vDbg(ctx, arg...) efyj_log_null(ctx, ##arg)
#define vInfo(ctx, arg...) efyj_log_null(ctx, ##arg)
#define vErr(ctx, arg...) efyj_log_null(ctx, ##arg)
#endif

#endif
