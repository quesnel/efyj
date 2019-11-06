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

namespace efyj {

enum class log_level
{
    emerg,   // system is unusable
    alert,   // action must be taken immediately
    crit,    // critical conditions
    err,     // error conditions
    warning, // warning conditions
    notice,  // normal but significant condition
    info,    // informational
    debug    // debug-level messages
};

class context
{
public:
    std::function<void(int, const std::string&)> log_cb;

    log_level log_priority;

    bool log_console = true;
};

inline bool
is_loggable(log_level current_level, log_level level) noexcept
{
    return static_cast<int>(current_level) >= static_cast<int>(level);
}

template<typename... Args>
void
log(const std::shared_ptr<context>& ctx,
    FILE* stream,
    log_level level,
    const char* fmt,
    const Args&... args)
{
    if (!is_loggable(ctx->log_priority, level))
        return;

    if (ctx->log_console)
        fmt::print(stream, fmt, args...);

    if (ctx->log_cb)
        ctx->log_cb(static_cast<int>(level),
                    fmt::format(fmt, args...).c_str());
}

template<typename... Args>
void
log(const std::shared_ptr<context>& ctx,
    FILE* stream,
    log_level level,
    const char* msg)
{
    if (!is_loggable(ctx->log_priority, level))
        return;

    if (ctx->log_console)
        fmt::print(stream, msg);

    if (ctx->log_cb)
        ctx->log_cb(static_cast<int>(level), fmt::format(msg).c_str());
}

template<typename... Args>
void
log(context* ctx,
    FILE* stream,
    log_level level,
    const char* fmt,
    const Args&... args)
{
    if (not is_loggable(ctx->log_priority, level))
        return;

    if (ctx->log_console)
        fmt::print(stream, fmt, args...);

    if (ctx->log_cb)
        ctx->log_cb(static_cast<int>(level),
                    fmt::format(fmt, args...).c_str());
}

template<typename... Args>
void
log(context* ctx, FILE* stream, log_level level, const char* msg)
{
    if (not is_loggable(ctx->log_priority, level))
        return;

    if (ctx->log_console)
        fmt::print(stream, msg);

    if (ctx->log_cb)
        ctx->log_cb(static_cast<int>(level), fmt::format(msg).c_str());
}

struct sink_arguments
{
    template<typename... Args>
    sink_arguments(const Args&...)
    {}
};

template<typename... Args>
void
emerg(const std::shared_ptr<context>& ctx,
      const char* fmt,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::emerg, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
alert(const std::shared_ptr<context>& ctx,
      const char* fmt,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::alert, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
crit(const std::shared_ptr<context>& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::crit, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
error(const std::shared_ptr<context>& ctx,
      const char* fmt,
      const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::err, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
warning(const std::shared_ptr<context>& ctx,
        const char* fmt,
        const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::warning, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
notice(const std::shared_ptr<context>& ctx,
       const char* fmt,
       const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::notice, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
info(const std::shared_ptr<context>& ctx, const char* fmt, const Args&... args)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stdout, log_level::info, fmt, args...);
#else
    sink_arguments(ctx, fmt, args...);
#endif
}

template<typename... Args>
void
debug(const std::shared_ptr<context>& ctx,
      const char* fmt,
      const Args&... args)
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
emerg(const std::shared_ptr<context>& ctx,
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
alert(const std::shared_ptr<context>& ctx,
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
crit(const std::shared_ptr<context>& ctx,
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
error(const std::shared_ptr<context>& ctx,
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
warning(const std::shared_ptr<context>& ctx,
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
notice(const std::shared_ptr<context>& ctx,
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
info(const std::shared_ptr<context>& ctx,
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
debug(const std::shared_ptr<context>& ctx,
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

template<typename T>
void
log(const std::shared_ptr<context>& ctx,
    FILE* stream,
    log_level level,
    const T& msg)
{
    if (not is_loggable(ctx->log_priority, level))
        return;

    if (ctx->log_console)
        fmt::print(stream, "{}", msg);

    if (ctx->log_cb)
        ctx->log_cb(static_cast<int>(level), fmt::format("{}", msg).c_str());
}

template<typename T>
void
log(context* ctx, FILE* stream, log_level level, const T& msg)
{
    if (not is_loggable(ctx->log_priority, level))
        return;

    if (ctx->log_console)
        fmt::print(stream, "{}", msg);

    if (ctx->log_cb)
        ctx->log_cb(static_cast<int>(level), fmt::format("{}", msg).c_str());
}

////////////////////////////////////////////////

template<typename T>
void
emerg(const std::shared_ptr<context>& ctx, const T& msg)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::emerg, msg);
#else
    sink_arguments(ctx, msg);
#endif
}

template<typename T>
void
alert(const std::shared_ptr<context>& ctx, const T& msg)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::alert, msg);
#else
    sink_arguments(ctx, msg);
#endif
}

template<typename T>
void
crit(const std::shared_ptr<context>& ctx, const T& msg)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::crit, msg);
#else
    sink_arguments(ctx, msg);
#endif
}

template<typename T>
void
error(const std::shared_ptr<context>& ctx, const T& msg)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::err, msg);
#else
    sink_arguments(ctx, msg);
#endif
}

template<typename T>
void
warning(const std::shared_ptr<context>& ctx, const T& msg)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stderr, log_level::warning, msg);
#else
    sink_arguments(ctx, msg);
#endif
}

template<typename T>
void
notice(const std::shared_ptr<context>& ctx, const T& msg)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stdout, log_level::notice, msg);
#else
    sink_arguments(ctx, msg);
#endif
}

template<typename T>
void
info(const std::shared_ptr<context>& ctx, const T& msg)
{
#ifdef EFYJ_ENABLE_LOG
    log(ctx, stdout, log_level::info, msg);
#else
    sink_arguments(ctx, msg);
#endif
}

template<typename T>
void
debug(const std::shared_ptr<context>& ctx, const T& msg)
{
#ifdef EFYJ_ENABLE_LOG
#ifndef EFYJ_ENABLE_DEBUG
    log(ctx, stdout, log_level::debug, msg);
#else
    sink_arguments(ctx, msg);
#endif
#endif
}

inline std::shared_ptr<context>
copy_context(const std::shared_ptr<context>& ctx)
{
    auto ret = make_context();

    ret->log_priority = ctx->log_priority;
    ret->log_console = ctx->log_console;

    return ret;
}

inline void
set_logger_callback(std::shared_ptr<context> ctx,
                    std::function<void(int, const std::string& message)> cb)
{
    debug(ctx, "efyj: change logger callback function.\n");

    ctx->log_cb = cb;
}
}

#endif
