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

#include <efyj/context.hpp>
#include <chrono>
#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>

namespace {

inline
void cout_no_deleter(std::ostream *os) noexcept
{
    (void)os;
}

inline
std::shared_ptr<std::ostream> make_cout_stream()
{
    return std::shared_ptr <std::ostream>(&std::cout, cout_no_deleter);
}

} // anonymous namespace

namespace efyj {

Context::Context(LogOption option)
    : m_os(::make_cout_stream())
    , m_log_priority(option)
{
    m_null_os.setstate(std::ios_base::badbit);
}

Context::~Context()
{
}

void Context::set_log_file_stream(const std::string &filepath) noexcept
{
    auto tmp = std::make_shared <std::ofstream>(filepath);

    if (not tmp->is_open()) {
        err() << "Failed to change log file stream ("
              << filepath << ")\n";
        return;
    }

    m_os = tmp;
    m_log_filename = filepath;
}

std::string Context::get_log_filename() const noexcept
{
    return m_log_filename;
}

void Context::set_console_log_stream()
{
    m_os = ::make_cout_stream();
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

std::ostream& Context::info() const noexcept
{
    if (static_cast<unsigned>(m_log_priority) >=
        static_cast<unsigned>(LOG_OPTION_INFO))
        return *m_os.get();

    return m_null_os;
}

std::ostream& Context::dbg() const noexcept
{
    if (static_cast<unsigned>(m_log_priority) >=
        static_cast<unsigned>(LOG_OPTION_DEBUG))
        return *m_os.get();

    return m_null_os;
}

std::ostream& Context::err() const noexcept
{
    if (static_cast<unsigned>(m_log_priority) >=
        static_cast<unsigned>(LOG_OPTION_ERROR))
        return *m_os.get();

    return m_null_os;
}

} // namespace efyj
