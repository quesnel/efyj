/* Copyright (C) 2015 INRA
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

#ifndef INRA_EFYj_CONTEXT_HPP
#define INRA_EFYj_CONTEXT_HPP

#include "message.hpp"
#include <fstream>
#include <deque>

namespace efyj {

class ContextImpl;

typedef std::shared_ptr <ContextImpl> Context;

} // namespace efyj

#define efyj_log_cond(ctx, prio, arg...)                                \
    do {                                                                \
        if ((ctx) && (ctx)->log_priority() >= prio) {                   \
            if (prio == LOG_OPTION_DEBUG)                               \
                (ctx)->log(prio, __FILE__, __LINE__,                    \
                           __PRETTY_FUNCTION__,  ## arg);               \
            else                                                        \
                (ctx)->log(prio, ## arg);                               \
        }                                                               \
    } while (0)

#ifdef ENABLE_LOGGING
    #ifdef ENABLE_DEBUG
        #define efyj_dbg(ctx, arg...) efyj_log_cond((ctx), LOG_OPTION_DEBUG, ## arg)
    #else
        #define efyj_dbg(ctx, arg...)
    #endif
    #define efyj_info(ctx, arg...) efyj_log_cond((ctx), LOG_OPTION_INFO, ## arg)
    #define efyj_err(ctx, arg...) efyj_log_cond((ctx), LOG_OPTION_ERR, ## arg)
#else
    #define efyj_dbg(ctx, arg...)
    #define efyj_info(ctx, arg...)
    #define efyj_err(ctx, arg...)
#endif

namespace efyj {

class ContextImpl
{
public:
    ContextImpl(const std::string &filepath, LogOption option = LOG_OPTION_DEBUG);

    ~ContextImpl();

    void set_log_stream(const std::string &filepath);

    template <typename... _Args>
    void log(_Args &&... args);

    LogOption log_priority() const;

    void set_log_priority(LogOption priority);

private:
    std::ofstream m_os;
    std::deque <message> m_queue;
    LogOption m_priority;
    std::mutex m_queue_locker;
    std::mutex m_os_locker;
    std::thread m_writer;
    bool m_close;
};

}

#include "details/context-implementation.hpp"

#endif
