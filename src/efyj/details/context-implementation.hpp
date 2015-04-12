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

#ifndef INRA_EFYj_DETAILS_CONTEXT_IMPLEMENTATION_HPP
#define INRA_EFYj_DETAILS_CONTEXT_IMPLEMENTATION_HPP

#include <chrono>
#include <iostream>
#include <fstream>
#include <memory>

namespace efyj {

namespace context_details {

inline void cout_no_deleter(std::ostream *os)
{
    (void)os;
}

inline std::shared_ptr<std::ostream> make_cout_stream()
{
    return std::shared_ptr <std::ostream>(&std::cout, cout_no_deleter);
}

inline std::shared_ptr<std::ostream> make_log_stream(const std::string &filepath)
{
    {
        std::shared_ptr <std::ofstream> tmp(new std::ofstream(filepath));

        if (tmp->is_open())
            return tmp;
    }
    {
        std::shared_ptr <std::ofstream> tmp(new std::ofstream("efyj.log"));

        if (tmp->is_open())
            return tmp;
    }

    return make_cout_stream();
}

inline void writer_run(std::shared_ptr <std::ostream> &os, std::deque <message> &queue,
                       std::mutex &queue_locker, std::mutex &os_locker, bool &close)
{
    std::deque <message> copy;

    while (not close) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        {
            std::lock_guard <std::mutex> lock(queue_locker);
            copy = queue;
            queue.clear();
        }
        {
            std::lock_guard <std::mutex> lock(os_locker);

            for (const auto &msg : copy)
                (*os) << msg << '\n';
        }
    }

    if (not queue.empty()) {
        {
            std::lock_guard <std::mutex> lock(queue_locker);
            copy = std::move(queue);
            queue.clear();
        }
        {
            std::lock_guard <std::mutex> lock(os_locker);

            for (const auto &msg : copy) {
                (*os) << msg << '\n';
            }
        }
    }
}

} // namespace context_details

inline ContextImpl::ContextImpl(LogOption option)
    : m_os(context_details::make_cout_stream())
    , m_priority(option)
    , m_close(false)
{
    m_writer = std::thread(context_details::writer_run,
                           std::ref(m_os), std::ref(m_queue), std::ref(m_queue_locker),
                           std::ref(m_os_locker), std::ref(m_close));
}

inline ContextImpl::ContextImpl(const std::string &filepath, LogOption option)
    : m_os(context_details::make_log_stream(filepath))
    , m_priority(option)
    , m_close(false)
{
    m_writer = std::thread(context_details::writer_run,
                           std::ref(m_os), std::ref(m_queue), std::ref(m_queue_locker),
                           std::ref(m_os_locker), std::ref(m_close));
}

inline ContextImpl::~ContextImpl()
{
    m_close = true;

    if (m_writer.joinable())
        m_writer.join();
}

inline void ContextImpl::set_log_stream(const std::string &filepath)
{
    std::lock_guard <std::mutex> lock(m_os_locker);
    m_os = context_details::make_log_stream(filepath);
}

inline void ContextImpl::set_console_log_stream()
{
    std::lock_guard <std::mutex> lock(m_os_locker);
    m_os = context_details::make_cout_stream();
}

template <typename... _Args>
void ContextImpl::log(_Args &&... args)
{
    std::lock_guard <std::mutex> lock(m_queue_locker);
    m_queue.emplace_back(std::forward <_Args>(args)...);
}

inline LogOption ContextImpl::log_priority() const
{
    return m_priority;
}

inline void ContextImpl::set_log_priority(LogOption priority)
{
    m_priority = priority;
}

}

#endif
