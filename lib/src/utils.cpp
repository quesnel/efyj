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

#include <thread>
#include <cstdio>
#include <cstdarg>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

namespace {

inline unsigned
hardware_concurrency_from_std() noexcept
{
    unsigned ret = std::thread::hardware_concurrency();

    return ret == 0 ? 1 : ret;
}

inline std::string vstringf(const char* format, va_list ap)
{
    std::string buffer(256, '\0');
    int sz;

    for (;;) {
        sz = std::vsnprintf(&buffer[0], buffer.size(), format, ap);

        if (sz < 0)
            return {};

        if (static_cast <std::size_t>(sz) < buffer.size())
            return buffer;

        buffer.resize(sz + 1);
    }
}

} // anonymous namespace

namespace efyj {

std::string stringf(const char* format, ...)
{
    va_list ap;

    va_start(ap, format);
    std::string ret = ::vstringf(format, ap);
    va_end(ap);

    return ret;
}

unsigned get_hardware_concurrency() noexcept
{
#ifdef HAVE_UNISTD_H
    long nb_procs = ::sysconf(_SC_NPROCESSORS_ONLN);
    if (nb_procs == -1)
        return ::hardware_concurrency_from_std();

    return static_cast<unsigned>(nb_procs);
#else
    return ::hardware_concurrency_from_std();
#endif
}

} // namespace efyj
