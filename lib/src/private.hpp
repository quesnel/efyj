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

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <cassert>

namespace efyj {

inline bool
is_loggable(log_level level, log_level current_level) noexcept
{
    return static_cast<int>(current_level) >= static_cast<int>(level);
}

template<typename... Args>
void
log(context& ctx, log_level level, const char* fmt, const Args&... args)
{
    if (!is_loggable(ctx.log_priority, level))
        return;

    switch (level) {
    case log_level::emerg:
    case log_level::alert:
    case log_level::crit:
    case log_level::err:
    case log_level::warning:
        if (ctx.err)
            fmt::print(*ctx.err, fmt, args...);
        break;
    case log_level::notice:
    case log_level::info:
    case log_level::debug:
        if (ctx.out)
            fmt::print(*ctx.out, fmt, args...);
        break;
    }
}

template<typename... Args>
void
log(context& ctx, log_level level, const char* msg)
{
    if (!is_loggable(ctx.log_priority, level))
        return;

    switch (level) {
    case log_level::emerg:
    case log_level::alert:
    case log_level::crit:
    case log_level::err:
    case log_level::warning:
        if (ctx.err)
            fmt::print(*ctx.err, "{}", msg);
        break;
    case log_level::notice:
    case log_level::info:
    case log_level::debug:
        if (ctx.out)
            fmt::print(*ctx.out, "{}", msg);
        break;
    }
}

template<typename... Args>
void
log_indent(context& ctx,
           unsigned indent,
           log_level level,
           const char* fmt,
           const Args&... args)
{
    if (!is_loggable(ctx.log_priority, level))
        return;

    switch (level) {
    case log_level::emerg:
    case log_level::alert:
    case log_level::crit:
    case log_level::err:
    case log_level::warning:
        if (ctx.err) {
            fmt::print(*ctx.err, "{:{}}", "", indent);
            fmt::print(*ctx.err, fmt, args...);
        }
        break;
    case log_level::notice:
    case log_level::info:
    case log_level::debug:
        if (ctx.out) {
            fmt::print(*ctx.out, "{:{}}", "", indent);
            fmt::print(*ctx.out, fmt, args...);
        }
        break;
    }
}

template<typename... Args>
void
log_indent(context& ctx, unsigned indent, log_level level, const char* msg)
{
    if (!is_loggable(ctx.log_priority, level))
        return;

    switch (level) {
    case log_level::emerg:
    case log_level::alert:
    case log_level::crit:
    case log_level::err:
    case log_level::warning:
        if (ctx.err) {
            fmt::print(*ctx.err, "{:{}}", "", indent);
            fmt::print(*ctx.err, "{}", msg);
        }
        break;
    case log_level::notice:
    case log_level::info:
    case log_level::debug:
        if (ctx.out) {
            fmt::print(*ctx.out, "{:{}}", "", indent);
            fmt::print(*ctx.out, "{}", msg);
        }
        break;
    }
}

struct sink_arguments
{
    template<typename... Args>
    sink_arguments(const Args&...)
    {}
};

template<typename... Args>
void
emerg(context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::emerg, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
alert(context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::alert, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
crit(context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::crit, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
error(context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::err, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
warning(context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::warning, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
notice(context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::notice, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
info(context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::info, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
debug(context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
#ifdef EFYJ_ENABLE_DEBUG
    log(ctx, log_level::debug, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
#endif
}

template<typename Arg1, typename... Args>
void
emerg(context& ctx, const char* fmt, const Arg1& arg1, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::emerg, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
alert(context& ctx, const char* fmt, const Arg1& arg1, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::alert, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
crit(context& ctx, const char* fmt, const Arg1& arg1, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::crit, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
error(context& ctx, const char* fmt, const Arg1& arg1, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::err, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
warning(context& ctx, const char* fmt, const Arg1& arg1, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::warning, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
notice(context& ctx, const char* fmt, const Arg1& arg1, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::notice, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
info(context& ctx, const char* fmt, const Arg1& arg1, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, log_level::info, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
debug(context& ctx, const char* fmt, const Arg1& arg1, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
#ifdef EFYJ_ENABLE_DEBUG
    log(ctx, log_level::debug, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
#endif
}

//
// The same with indent parameters
//

template<typename... Args>
void
emerg(context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::emerg, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
alert(context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::alert, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
crit(context& ctx,
     [[maybe_unused]] unsigned indent,
     const char* fmt,
     const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::crit, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
error(context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::err, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
warning(context& ctx,
        [[maybe_unused]] unsigned indent,
        const char* fmt,
        const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::warning, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
notice(context& ctx,
       [[maybe_unused]] unsigned indent,
       const char* fmt,
       const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::notice, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
info(context& ctx,
     [[maybe_unused]] unsigned indent,
     const char* fmt,
     const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::info, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
debug(context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
#ifdef EFYJ_ENABLE_DEBUG
    log_indent(ctx, indent, log_level::debug, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
#endif
}

template<typename Arg1, typename... Args>
void
emerg(context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::emerg, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
alert(context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::alert, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
crit(context& ctx,
     [[maybe_unused]] unsigned indent,
     const char* fmt,
     const Arg1& arg1,
     const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::crit, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
error(context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::err, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
warning(context& ctx,
        [[maybe_unused]] unsigned indent,
        const char* fmt,
        const Arg1& arg1,
        const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::warning, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
notice(context& ctx,
       [[maybe_unused]] unsigned indent,
       const char* fmt,
       const Arg1& arg1,
       const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::notice, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
info(context& ctx,
     [[maybe_unused]] unsigned indent,
     const char* fmt,
     const Arg1& arg1,
     const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, indent, log_level::info, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
debug(context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
#ifdef EFYJ_ENABLE_DEBUG
    log_indent(ctx, indent, log_level::debug, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
#endif
}

}

#endif
