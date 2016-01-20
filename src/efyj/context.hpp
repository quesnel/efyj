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

#ifndef INRA_EFYj_CONTEXT_HPP
#define INRA_EFYj_CONTEXT_HPP

#include <efyj/efyj.hpp>
#include <efyj/message.hpp>
#include <list>
#include <fstream>
#include <thread>
#include <mutex>

namespace efyj {

class ContextImpl;

typedef std::shared_ptr <ContextImpl> Context;

class EFYJ_API ContextImpl
{
public:
    ContextImpl(LogOption option = LOG_OPTION_DEBUG);
    ContextImpl(const std::string &filepath, LogOption option = LOG_OPTION_DEBUG);

    ContextImpl(const ContextImpl&) = delete;
    ContextImpl(ContextImpl&&) = delete;
    ContextImpl& operator=(const ContextImpl&) = delete;
    ContextImpl& operator=(ContextImpl&&) = delete;

    ~ContextImpl();

    void set_log_stream(const std::string &filepath);

    void set_console_log_stream();

    template <typename... _Args>
    void log(_Args &&... args)
    {
        std::lock_guard <std::mutex> lock(m_queue_locker);
        m_queue.emplace_back(std::forward <_Args>(args)...);
    }

    LogOption log_priority() const;

    void set_log_priority(LogOption priority);

private:
    std::shared_ptr <std::ostream> m_os;
    std::list <message> m_queue;
    LogOption m_priority;
    std::mutex m_queue_locker;
    std::mutex m_os_locker;
    std::thread m_writer;
    bool m_close;
};

}

#endif
