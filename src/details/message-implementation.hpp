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

#ifndef INRA_EFYj_MESSAGE_IMPLEMENTATION_HPP
#define INRA_EFYj_MESSAGE_IMPLEMENTATION_HPP

#include <sstream>

#include <sys/types.h>
#include <unistd.h>

namespace efyj {

template <typename T>
inline message::message(LogOption priority, const T &t)
    : thread_id(std::this_thread::get_id())
    , priority(priority)
    , pid(::getpid())
{
    std::stringstream os;
    os << t;
    msg = os.str();
}

template <>
inline message::message(LogOption priority, const std::string &msg)
    : msg(msg)
    , thread_id(std::this_thread::get_id())
    , priority(priority)
    , pid(::getpid())
{
}

template <typename T>
inline message::message(LogOption priority, const char *file, int line,
                        const char *fn, const T &t)
    : file(file)
    , fn(fn)
    , thread_id(std::this_thread::get_id())
    , priority(priority)
    , pid(::getpid())
    , line(line)
{
    std::stringstream os;
    os << t;
    msg = os.str();
}

template <>
inline message::message(LogOption priority, const char *file,
                        int line,
                        const char *fn, const std::string &msg)
    : msg(msg)
    , file(file)
    , fn(fn)
    , thread_id(std::this_thread::get_id())
    , priority(priority)
    , pid(::getpid())
    , line(line)
{
}

inline std::ostream &operator<<(std::ostream &os, const message &msg)
{
#ifdef NDEBUG
    os << msg.pid << '@' << msg.thread_id << ": " << msg.priority << "\n";
#endif

    if (not msg.file.empty())
        os << "DEBUG: " << msg.file << ' ' << msg.fn << ' ' << msg.line << "\n    ";

    return os << msg.msg;
}

}

#endif
