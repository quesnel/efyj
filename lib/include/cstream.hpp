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

#ifndef FR_INRA_EFYJ_CSTREAM_HPP
#define FR_INRA_EFYJ_CSTREAM_HPP

#include "efyj.hpp"
#include <string>
#include <cstdio>
#include <cstdarg>
#include <cinttypes>

namespace efyj {

/** efyj::cstream is an inspired class from the std::ostream. This class
 * implements an extremely fast and simple output stream that can only
 * output to a stream. It does not support seeking, reopening, rewinding,
 * etc.
 *
 * @code
 * #include "cstream.hpp"
 * int main()
 * {
 *     out() << out().red() << "In red " << out().def() << "\n";
 *
 *     int fd = ::open("output.log", O_CREATE);
 *     if (fd >= 0) {
 *         efyj::cstream cs(fd);
 *         cs << "Hello world!\n";
 *         cs.printf("Hello world %d\n", fd);
 *     }
 * }
 * @endcode
 */
class EFYJ_API cstream
{
public:
    enum colors { Default = 0, Black, Red, Green, Yellow, Blue, Magenta,
                  Cyan, Light_gray, Dark_gray, Light_red, Light_green, Light_yellow,
                  Light_blue, Light_magenta, Light_cyan, White, No_color_change };

    enum setters { Reset = 0, Bold, Dim, Underlined, No_setter_change };

    struct modifier {
        constexpr modifier(colors color_, setters setter_) noexcept
            : color(color_)
            , setter(setter_)
        {}

        constexpr modifier(colors color_) noexcept
            : color(color_)
            , setter(No_setter_change)
        {}

        constexpr modifier(setters setter_) noexcept
            : color(No_color_change)
            , setter(setter_)
        {}

        constexpr modifier() noexcept
            : color(No_color_change)
            , setter(No_setter_change)
        {}

        colors color;
        setters setter;
    };

    cstream(int fd, bool try_color_mode, bool close_fd) noexcept;
    ~cstream() noexcept;

    bool have_color_mode() const noexcept;

    /// \e error returns true if an internal write operation failed in
    /// a previous write.
    bool error() const noexcept;

    cstream(const cstream&) = delete;
    cstream& operator=(const cstream&) = delete;
    cstream(cstream&&) = delete;
    cstream& operator=(cstream&&) = delete;

    cstream& operator<<(char c) noexcept;
    cstream& operator<<(unsigned char c) noexcept;
    cstream& operator<<(signed char c) noexcept;
    cstream& operator<<(const char *str) noexcept;
    cstream& operator<<(const std::string& str) noexcept;
    cstream& operator<<(unsigned int n) noexcept;
    cstream& operator<<(signed int n) noexcept;
    cstream& operator<<(unsigned long n) noexcept;
    cstream& operator<<(signed long n) noexcept;
    cstream& operator<<(unsigned long long n) noexcept;
    cstream& operator<<(signed long long n) noexcept;
    cstream& operator<<(long double n) noexcept;
    cstream& operator<<(double n) noexcept;
    cstream& operator<<(float n) noexcept;
    cstream& operator<<(modifier m) noexcept;

    cstream& printf(const char *format, ...) noexcept;
    cstream& printf(const char *format, va_list ap) noexcept;

    cstream& write(const char *buf, std::size_t count) noexcept;
    cstream& write(const std::string &str) noexcept;
    cstream& write(unsigned char c) noexcept;

    cstream& set_modifier(modifier m) noexcept;
    cstream& reset_modifier() noexcept;

    cstream& indent(unsigned space_number) noexcept;

    modifier def() const noexcept { return { Default, Reset }; }
    modifier defb() const noexcept { return { Default, Bold }; }
    modifier defd() const noexcept { return { Default, Dim }; }
    modifier defu() const noexcept { return { Default, Underlined }; }

    modifier red() const noexcept { return { Red, Reset }; }
    modifier redb() const noexcept { return { Red, Bold }; }
    modifier redd() const noexcept { return { Red, Dim }; }
    modifier redu() const noexcept { return { Red, Underlined }; }

    modifier green() const noexcept { return { Green, Reset }; }
    modifier greenb() const noexcept { return { Green, Bold }; }
    modifier greend() const noexcept { return { Green, Dim }; }
    modifier greenu() const noexcept { return { Green, Underlined }; }

    modifier yellow() const noexcept { return { Yellow, Reset }; }
    modifier yellowb() const noexcept { return { Yellow, Bold }; }
    modifier yellowd() const noexcept { return { Yellow, Dim }; }
    modifier yellowu() const noexcept { return { Yellow, Underlined }; }

    modifier magenta() const noexcept { return { Magenta, Reset }; }
    modifier magentab() const noexcept { return { Magenta, Bold }; }
    modifier magentad() const noexcept { return { Magenta, Dim }; }
    modifier magentau() const noexcept { return { Magenta, Underlined }; }

    modifier cyan() const noexcept { return { Cyan, Reset }; }
    modifier cyanb() const noexcept { return { Cyan, Bold }; }
    modifier cyand() const noexcept { return { Cyan, Dim }; }
    modifier cyanu() const noexcept { return { Cyan, Underlined }; }

    modifier reset() const noexcept { return { Default, Reset }; }

private:
    /// \e fd the file descriptor.
    int fd;

    /// \e tty is true if the file descriptor allows colors (windows
    /// console or tty on unix).
    bool color_mode;

    /// \e error_detected reports fatal error in a write operation.
    bool error_detected;

    /// \e close_fd is true if the file descriptor must be close in
    /// destructor. \e close_fd is true for regular file, false for
    /// standard output and error stream.
    bool close_fd;
};

// /** Give an access to the standard output stream (stdout).
//  */
EFYJ_API cstream& out();

// /** Give an access to the standard output error stream (stderr).
//  */
EFYJ_API cstream& err();

//
// implementation part.
//

inline cstream&
cstream::write(const std::string &str) noexcept
{
    return write(str.c_str(), str.size());
}

inline cstream&
cstream::write(unsigned char c) noexcept
{
    return write(reinterpret_cast<char*>(c), 1);
}

}

#endif