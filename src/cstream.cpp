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

#include <efyj/cstream.hpp>
#include <vector>
#include <cerrno>
#include <cassert>
#include <cstring>

#ifdef __unix__
#include <unistd.h>
#endif

namespace {

bool is_use_color_mode(int fd, bool try_color_mode) noexcept
{
    if (not try_color_mode)
        return false;

#ifdef __unix__
    return 1 == ::isatty(fd);
#else
    return false;
#endif
}

/// Find the length of a C array.
template <class T, std::size_t N>
std::size_t array_lenght(T (&)[N])
{
    return N;
}

const char*
color_to_str(efyj::cstream::colors c) noexcept
{
    static const char colors[][10] = {
        "\033[39m",
        "\033[30m",
        "\033[31m",
        "\033[32m",
        "\033[33m",
        "\033[34m",
        "\033[35m",
        "\033[36m",
        "\033[37m",
        "\033[90m",
        "\033[91m",
        "\033[92m",
        "\033[93m",
        "\033[94m",
        "\033[95m",
        "\033[96m",
        "\033[97m" };

    assert(static_cast<int>(c) <
           static_cast<int>(array_lenght(colors)));

    return colors[static_cast<int>(c)];
}

const char*
setter_to_str(efyj::cstream::setters s) noexcept
{
    static const char setters[][10] = {
        "\033[0m",
        "\033[1m",
        "\033[2m",
        "\033[4m" };

    assert(static_cast<int>(s) <
           static_cast<int>(array_lenght(setters)));

    return setters[static_cast<int>(s)];
}

void err_conversion_error(efyj::cstream& cs, const char *what) noexcept
{
    cs << cs.red()
       << "stream conversion error "
       << cs.def()
       << what
       << "\n";
}

} // anonymous namespace

namespace efyj {

cstream::cstream(int fd_, bool try_color_mode_, bool close_fd_) noexcept
: fd(fd_)
    , color_mode(::is_use_color_mode(fd_, try_color_mode_))
    , error_detected(false)
    , close_fd(close_fd_)
{
}

cstream::~cstream() noexcept
{
    if (fd >= 0) {
        ::fsync(fd);

        if (close_fd)
            ::close(fd);
    }
}

bool
cstream::have_color_mode() const noexcept
{
    return color_mode;
}

bool
cstream::error() const noexcept
{
    return error_detected;
}

cstream&
cstream::operator<<(char c) noexcept
{
    return write(&c, 1);
}

cstream&
cstream::operator<<(unsigned char c) noexcept
{
    return write(reinterpret_cast<const char*>(&c), 1);
}

cstream&
cstream::operator<<(signed char c) noexcept
{
    return write(reinterpret_cast<const char*>(&c), 1);
}

cstream&
cstream::operator<<(const char *str) noexcept
{
    return write(str, std::strlen(str));
}

cstream&
cstream::operator<<(const std::string& str) noexcept
{
    return write(str.data(), str.size());
}

cstream&
cstream::operator<<(unsigned int n) noexcept
{
    std::string str;

    try {
        str = std::to_string(n);
    } catch (const std::exception& e) {
        err_conversion_error(*this, e.what());
        return *this;
    }

    return write(str.data(), str.size());
}

cstream&
cstream::operator<<(signed int n) noexcept
{
    std::string str;

    try {
        str = std::to_string(n);
    } catch (const std::exception& e) {
        err_conversion_error(*this, e.what());
        return *this;
    }

    return write(str.data(), str.size());
}

cstream&
cstream::operator<<(unsigned long n) noexcept
{
    std::string str;

    try {
        str = std::to_string(n);
    } catch (const std::exception& e) {
        err_conversion_error(*this, e.what());
        return *this;
    }

    return write(str.data(), str.size());
}

cstream&
cstream::operator<<(signed long n) noexcept
{
    std::string str;

    try {
        str = std::to_string(n);
    } catch (const std::exception& e) {
        err_conversion_error(*this, e.what());
        return *this;
    }

    return write(str.data(), str.size());
}

cstream&
cstream::operator<<(unsigned long long n) noexcept
{
    std::string str;

    try {
        str = std::to_string(n);
    } catch (const std::exception& e) {
        err_conversion_error(*this, e.what());
        return *this;
    }

    return write(str.data(), str.size());
}

cstream&
cstream::operator<<(signed long long n) noexcept
{
    std::string str;

    try {
        str = std::to_string(n);
    } catch (const std::exception& e) {
        err_conversion_error(*this, e.what());
        return *this;
    }

    return write(str.data(), str.size());
}

cstream&
cstream::operator<<(long double n) noexcept
{
    std::string str;

    try {
        str = std::to_string(n);
    } catch (const std::exception& e) {
        err_conversion_error(*this, e.what());
        return *this;
    }

    return write(str.data(), str.size());
}

cstream&
cstream::operator<<(double n) noexcept
{
    std::string str;

    try {
        str = std::to_string(n);
    } catch (const std::exception& e) {
        err_conversion_error(*this, e.what());
        return *this;
    }

    return write(str.data(), str.size());
}

cstream&
cstream::operator<<(float n) noexcept
{
    std::string str;

    try {
        str = std::to_string(n);
    } catch (const std::exception& e) {
        err_conversion_error(*this, e.what());
        return *this;
    }

    return write(str.data(), str.size());
}

cstream&
cstream::operator<<(cstream::modifier m) noexcept
{
    return set_modifier(m);
}

cstream&
cstream::printf(const char *format, ...) noexcept
{
    va_list ap;

    va_start(ap, format);
    cstream::printf(format, ap);
    va_end(ap);

    return *this;
}

cstream&
cstream::printf(const char *format, va_list ap) noexcept
{
#if XOPEN_SOURCE >= 700 || _POSIX_C_SOURCE >= 200809L
    // If we use the glibc2 library, we can use the vdprintf. The function
    // vdprintf() is exact analog of vfprintf, except that they output to
    // a file descriptor fd instead of to a stdio stream.

    ::vdprintf(fd, format, ap);
#else
    std::vector<char>::size_type size {100};
    std::vector<char> buffer;

    for (;;) {
        try {
            buffer.resize(size);
        } catch (const std::bad_alloc&) {
            return *this;
        }

        int n = std::vsnprintf(buffer.data(), size, format, ap);
        if (n < 0) {
            return *this;
        }

        if (static_cast<std::string::size_type>(n) < size)
            return write(buffer.data(), n);

        size = static_cast<std::string::size_type>(n) + 1;
    }
#endif

    return *this;
}

cstream&
cstream::write(const char *buf, std::size_t count) noexcept
{
    assert(fd >= 0 && "file already closed");

    if (buf == nullptr or count == 0)
        return *this;

    do {
        ssize_t ret = ::write(fd, buf, count);

        if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;

            perror("cstream::write");
#ifndef NDEBUG
            {
                const char *buf = "cstream::write error. Turn off logging\n";
                ::write(STDERR_FILENO, buf, strlen(buf));
            }
#endif

            error_detected = true;
            break;
        }

        count -= ret;
        buf += ret;
    } while (count > 0);

    return *this;
}

cstream&
cstream::set_modifier(modifier m) noexcept
{
    if (not color_mode)
        return *this;

    std::string str;

    try {
        str.reserve(20);

        if (m.color != No_color_change)
            str = ::color_to_str(m.color);

        if (m.setter != No_setter_change)
            str += ::setter_to_str(m.setter);

    } catch (const std::bad_alloc&) {
        return *this;
    }

    return write(str);
}

cstream&
cstream::reset_modifier() noexcept
{
    if (not color_mode)
        return *this;

    std::string str;

    try {
        str.reserve(20);
        str = ::color_to_str(colors::Default);
        str += ::setter_to_str(setters::Reset);
    } catch (const std::bad_alloc&) {
        return *this;
    }

    return write(str);
}

cstream&
cstream::indent(unsigned space_number) noexcept
{
    static const char spaces[] = "                                ";

    if (space_number < ::array_lenght(spaces))
        return write(spaces, space_number);

    while (space_number) {
        unsigned to_write = std::min(
            space_number,
            static_cast<unsigned>(::array_lenght(spaces) - 1));
        write(spaces, to_write);
        space_number -= to_write;
    }

    return *this;
}

cstream& out()
{
    static cstream cs(STDOUT_FILENO, true, false);
    return cs;
}

cstream& err()
{
    static cstream cs(STDERR_FILENO, true, false);
    return cs;
}

}
