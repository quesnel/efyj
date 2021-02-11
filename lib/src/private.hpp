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

#include <cassert>

namespace efyj {

inline bool
is_loggable(log_level current_level, log_level level) noexcept
{
    return static_cast<int>(current_level) >= static_cast<int>(level);
}

template<typename... Args>
void
log(const context& ctx,
    FILE* stream,
    log_level level,
    const char* fmt,
    const Args&... args)
{
    if (!is_loggable(ctx.log_priority, level))
        return;

    assert(stream);

    fmt::print(stream, fmt, args...);
}

template<typename... Args>
void
log(const context& ctx, FILE* stream, log_level level, const char* msg)
{
    if (!is_loggable(ctx.log_priority, level))
        return;

    assert(stream);

    fmt::print(stream, msg);
}

template<typename... Args>
void
log_indent(const context& ctx,
           FILE* stream,
           unsigned indent,
           log_level level,
           const char* fmt,
           const Args&... args)
{
    if (!is_loggable(ctx.log_priority, level))
        return;

    assert(stream);

    fmt::print(stream, "{:{}}", "", indent);
    fmt::print(stream, fmt, args...);
}

template<typename... Args>
void
log_indent(const context& ctx,
           FILE* stream,
           unsigned indent,
           log_level level,
           const char* msg)
{
    if (!is_loggable(ctx.log_priority, level))
        return;

    assert(stream);

    fmt::print(stream, "{:{}}", "", indent);
    fmt::print(stream ? stream : stdout, msg);
}

struct sink_arguments
{
    template<typename... Args>
    sink_arguments(const Args&...)
    {}
};

template<typename... Args>
void
emerg(const context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::emerg, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
alert(const context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::alert, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
crit(const context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::crit, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
error(const context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::err, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
warning(const context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::warning, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
notice(const context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::notice, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
info(const context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stdout, log_level::info, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
debug(const context& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
#ifdef EFYJ_ENABLE_DEBUG
    log(ctx, stdout, log_level::debug, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
#endif
}

template<typename Arg1, typename... Args>
void
emerg(const context& ctx,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::emerg, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
alert(const context& ctx,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::alert, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
crit(const context& ctx,
     const char* fmt,
     const Arg1& arg1,
     const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::crit, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
error(const context& ctx,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::err, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
warning(const context& ctx,
        const char* fmt,
        const Arg1& arg1,
        const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::warning, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
notice(const context& ctx,
       const char* fmt,
       const Arg1& arg1,
       const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stdout, log_level::notice, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
info(const context& ctx,
     const char* fmt,
     const Arg1& arg1,
     const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stdout, log_level::info, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
debug(const context& ctx,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
#ifdef EFYJ_ENABLE_DEBUG
    log(ctx, stdout, log_level::debug, fmt, arg1, args...);
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
emerg(const context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stderr, indent, log_level::emerg, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
alert(const context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stderr, indent, log_level::alert, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
crit(const context& ctx,
     [[maybe_unused]] unsigned indent,
     const char* fmt,
     const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stderr, indent, log_level::crit, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
error(const context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stderr, indent, log_level::err, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
warning(const context& ctx,
        [[maybe_unused]] unsigned indent,
        const char* fmt,
        const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stderr, indent, log_level::warning, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
notice(const context& ctx,
       [[maybe_unused]] unsigned indent,
       const char* fmt,
       const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stderr, indent, log_level::notice, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
info(const context& ctx,
     [[maybe_unused]] unsigned indent,
     const char* fmt,
     const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stdout, indent, log_level::info, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
debug(const context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
#ifdef EFYJ_ENABLE_DEBUG
    log_indent(ctx, stdout, indent, log_level::debug, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
#endif
}

template<typename Arg1, typename... Args>
void
emerg(const context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stderr, indent, log_level::emerg, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
alert(const context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stderr, indent, log_level::alert, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
crit(const context& ctx,
     [[maybe_unused]] unsigned indent,
     const char* fmt,
     const Arg1& arg1,
     const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stderr, indent, log_level::crit, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
error(const context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stderr, indent, log_level::err, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
warning(const context& ctx,
        [[maybe_unused]] unsigned indent,
        const char* fmt,
        const Arg1& arg1,
        const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stderr, indent, log_level::warning, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
notice(const context& ctx,
       [[maybe_unused]] unsigned indent,
       const char* fmt,
       const Arg1& arg1,
       const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stdout, indent, log_level::notice, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
info(const context& ctx,
     [[maybe_unused]] unsigned indent,
     const char* fmt,
     const Arg1& arg1,
     const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log_indent(ctx, stdout, indent, log_level::info, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
}

template<typename Arg1, typename... Args>
void
debug(const context& ctx,
      [[maybe_unused]] unsigned indent,
      const char* fmt,
      const Arg1& arg1,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
#ifdef EFYJ_ENABLE_DEBUG
    log_indent(ctx, stdout, indent, log_level::debug, fmt, arg1, args...);
#else
    sink_arguments(ctx, fmt, arg1, args...);
#endif
#endif
}

}

#endif
