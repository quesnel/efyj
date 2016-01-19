/* Copyright (C) 2015 INRA
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

#include "context.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <memory>

namespace {

void cout_no_deleter(std::ostream *os)
{
    (void)os;
}

std::shared_ptr<std::ostream> make_cout_stream()
{
    return std::shared_ptr <std::ostream>(&std::cout, cout_no_deleter);
}

std::shared_ptr<std::ostream> make_log_stream(const std::string &filepath)
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

void writer_run(std::shared_ptr <std::ostream> &os,
                std::list <efyj::message> &queue,
                std::mutex &queue_locker,
                std::mutex &os_locker, bool &close)
{
    std::list <efyj::message> to_write;

    while (not close) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        {
            std::lock_guard <std::mutex> lock(queue_locker);
            to_write.splice(to_write.begin(), queue);
        }
        {
            std::lock_guard <std::mutex> lock(os_locker);

            for (const auto &msg : to_write)
                (*os) << msg << '\n';

            to_write.clear();
        }
    }

    if (not queue.empty()) {
        {
            std::lock_guard <std::mutex> lock(queue_locker);
            to_write.splice(to_write.begin(), queue);
        }
        {
            std::lock_guard <std::mutex> lock(os_locker);

            for (const auto &msg : to_write)
                (*os) << msg << '\n';

            to_write.clear();
        }
    }
}

} // anonymous namespace

namespace efyj {

ContextImpl::ContextImpl(LogOption option)
    : m_os(::make_cout_stream())
    , m_priority(option)
    , m_close(false)
{
    m_writer = std::thread(
        ::writer_run,
        std::ref(m_os), std::ref(m_queue), std::ref(m_queue_locker),
        std::ref(m_os_locker), std::ref(m_close));
}

ContextImpl::ContextImpl(const std::string &filepath, LogOption option)
    : m_os(::make_log_stream(filepath))
    , m_priority(option)
    , m_close(false)
{
    m_writer = std::thread(
        ::writer_run,
        std::ref(m_os), std::ref(m_queue), std::ref(m_queue_locker),
        std::ref(m_os_locker), std::ref(m_close));
}

ContextImpl::~ContextImpl()
{
    m_close = true;

    if (m_writer.joinable())
        m_writer.join();
}

void ContextImpl::set_log_stream(const std::string &filepath)
{
    std::lock_guard <std::mutex> lock(m_os_locker);
    m_os = ::make_log_stream(filepath);
}

void ContextImpl::set_console_log_stream()
{
    std::lock_guard <std::mutex> lock(m_os_locker);
    m_os = ::make_cout_stream();
}

LogOption ContextImpl::log_priority() const
{
    return m_priority;
}

void ContextImpl::set_log_priority(LogOption priority)
{
    m_priority = priority;
}

} // namespace efyj
