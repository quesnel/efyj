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

void err_conversion_error(const char *what) noexcept
{
    efyj::err() << efyj::err().red()
                << "stream conversion error "
                << efyj::err().def()
                << what
                << "\n";
}

void err_write_error(int error_code) noexcept
{
    std::vector<char>::size_type size {512};
    std::vector<char> buf;

    for (;;) {
        try {
            buf.resize(size);
        } catch (const std::bad_alloc&) {
            efyj::err() << efyj::err().red()
                << "write error"
                << efyj::err().def()
                << "\n";
            return;
        }

#if ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE)
        errno = 0;
        if (::strerror_r(error_code, buf.data(), buf.size()) != 0) {
            if (errno == ERANGE) {
                size *= 2;
                continue;
            } else {
                efyj::err() << efyj::err().red()
                    << "write error: unknown errno code"
                    << efyj::err().def()
                    << "\n";
                return;
            }
        }
#else
        char *msg = ::strerror_r(error_code, buf.data(), buf.size());
        if (msg != buf.data()) {
            efyj::err() << efyj::err().red()
                << "write error: "
                << efyj::err().def ()
                << msg
                << "\n";
            return;
        }
#endif
        else {
            efyj::err() << efyj::err().red()
                << "write error: "
                << efyj::err().def()
                << buf.data()
                << "\n";
            return;
        }
    }
}

} // anonymous namespace

namespace efyj {

cstream::cstream(int fd_, bool try_color_mode) noexcept
    : fd(fd_)
    , color_mode(::is_use_color_mode(fd, try_color_mode))
{
}

cstream::~cstream() noexcept
{
    ::fsync(fd);
}

cstream&
cstream::operator<<(char c) noexcept
{
    assert(fd >= 0 && "file already closed");

    if (::write(fd, &c, 1) != static_cast<ssize_t>(1))
	    err_write_error(errno);

    return *this;
}

cstream&
cstream::operator<<(unsigned char c) noexcept
{
    assert(fd >= 0 && "file already closed");

    if (::write(fd, &c, 1) != static_cast<ssize_t>(1))
	    err_write_error(errno);

    return *this;
}

cstream&
cstream::operator<<(signed char c) noexcept
{
    assert(fd >= 0 && "file already closed");

    if (::write(fd, &c, 1) != static_cast<ssize_t>(1))
	    err_write_error(errno);

    return *this;
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
    try {
        return write(std::to_string(n));
    } catch (const std::exception& e) {
        err_conversion_error(e.what());
        return *this;
    }
}

cstream&
cstream::operator<<(signed int n) noexcept
{
    try {
        return write(std::to_string(n));
    } catch (const std::exception& e) {
        err_conversion_error(e.what());
        return *this;
    }
}

cstream&
cstream::operator<<(unsigned long n) noexcept
{
    try {
        return write(std::to_string(n));
    } catch (const std::exception& e) {
        err_conversion_error(e.what());
        return *this;
    }
}

cstream&
cstream::operator<<(signed long n) noexcept
{
    try {
        return write(std::to_string(n));
    } catch (const std::exception& e) {
        err_conversion_error(e.what());
        return *this;
    }
}

cstream&
cstream::operator<<(unsigned long long n) noexcept
{
    try {
        return write(std::to_string(n));
    } catch (const std::exception& e) {
        err_conversion_error(e.what());
        return *this;
    }
}

cstream&
cstream::operator<<(signed long long n) noexcept
{
    try {
        return write(std::to_string(n));
    } catch (const std::exception& e) {
        err_conversion_error(e.what());
        return *this;
    }
}

cstream&
cstream::operator<<(double n) noexcept
{
    try {
        return write(std::to_string(n));
    } catch (const std::exception& e) {
        err_conversion_error(e.what());
        return *this;
    }
}

cstream&
cstream::operator<<(float n) noexcept
{
    try {
        return write(std::to_string(n));
    } catch (const std::exception& e) {
        err_conversion_error(e.what());
        return *this;
    }
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
    std::vector<char>::size_type size {100};
    std::vector<char> buffer;

    for (;;) {
        try {
            buffer.resize(size);
        } catch (const std::bad_alloc&) {
            err() << red() << "stream printf error: bad alloc\n";
            return *this;
        }

        int n = std::vsnprintf(buffer.data(), size, format, ap);
        if (n < 0) {
            err() << red() <<  "stream printf error" << def() << "\n";
            return *this;
        }

        if (static_cast<std::string::size_type>(n) < size)
            return write(buffer.data(), n);

        size = static_cast<std::string::size_type>(n) + 1;
    }
}

cstream&
cstream::write(const char *buf, std::size_t count) noexcept
{
    assert(fd >= 0 && "file already closed");

    if (buf != nullptr or count > 0)
        if (::write(fd, buf, count) != static_cast<ssize_t>(count))
            err_write_error(errno);

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
    static cstream cs(STDOUT_FILENO);
    return cs;
}

cstream& err()
{
    static cstream cs(STDERR_FILENO);
    return cs;
}

}
