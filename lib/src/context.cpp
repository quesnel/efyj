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

#include "context.hpp"
#include <chrono>
#include <fstream>
#include <memory>
#include <cassert>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef __unix__
#include <unistd.h>
#endif

namespace efyj {

Context::Context(LogOption option)
    : m_cs(std::make_shared<cstream>(STDOUT_FILENO, true, false))
      // TODO found a best solution to avoid this CPU cycle burning
      // solution. A specific null_cstream that inherits of cstream
      // with all empty function?
    , m_null_cs(::open("/dev/null", O_APPEND), false, false)
    , m_log_priority(option)
{
}

Context::~Context()
{
}

bool Context::set_log_file_stream(std::string filepath) noexcept
{
    auto fd = ::open(filepath.c_str(),
                     O_CREAT | O_WRONLY | O_TRUNC,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

    if (fd == -1) {
        err() << err().redb();
        err().printf("Error: failed to change log file stream (%s)\n",
                     filepath.c_str());
        err() << err().def();
        return false;
    }

    std::shared_ptr<cstream> tmp;
    try {
        tmp = std::make_shared <cstream>(fd, false, true);
    } catch (const std::bad_alloc&) {
        ::close(fd);
        return false;
    }

    try {
        m_log_filename.swap(filepath);
    } catch (const std::bad_alloc&) {
        ::close(fd);
        return false;
    }

    m_cs = tmp;

    return true;
}

std::string Context::get_log_filename() const noexcept
{
    return m_log_filename;
}

void Context::set_console_log_stream()
{
    std::shared_ptr<cstream> tmp;
    try {
        tmp = std::make_shared<cstream>(STDOUT_FILENO, true, false);
    } catch (const std::bad_alloc&) {
        return;
    }

    m_cs = tmp;
    m_log_filename.clear();
}

LogOption Context::log_priority() const
{
    return m_log_priority;
}

void Context::set_log_priority(LogOption priority)
{
    m_log_priority = priority;
}

cstream& Context::info() const noexcept
{
    if (static_cast<unsigned>(m_log_priority) >=
        static_cast<unsigned>(LOG_OPTION_INFO))
        return *m_cs.get();

    return m_null_cs;
}

cstream& Context::dbg() const noexcept
{
    if (static_cast<unsigned>(m_log_priority) >=
        static_cast<unsigned>(LOG_OPTION_DEBUG))
        return *m_cs.get();

    return m_null_cs;
}

cstream& Context::err() const noexcept
{
    if (static_cast<unsigned>(m_log_priority) >=
        static_cast<unsigned>(LOG_OPTION_ERROR))
        return *m_cs.get();

    return m_null_cs;
}

} // namespace efyj
