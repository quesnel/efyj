/* Copyright (C) 2016-2017 INRA
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

#ifndef INRA_EFYj_UTILS_HPP
#define INRA_EFYj_UTILS_HPP

#include <cstdarg>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#if defined __GNUC__
#define EFYJ_GCC_PRINTF(format__, args__)                                     \
    __attribute__((format(printf, format__, args__)))
#endif

namespace efyj {

inline constexpr std::size_t max_value(int need, std::size_t real) noexcept;

std::string stringf(const char* format, ...) noexcept EFYJ_GCC_PRINTF(1, 2);

std::string vstringf(const char* format, va_list) noexcept;

struct scope_exit
{
    scope_exit(std::function<void(void)> fct)
      : fct(fct)
    {
    }

    ~scope_exit()
    {
        fct();
    }

    std::function<void(void)> fct;
};

/** \e make_new_name is used to create new file path with a suffix composed
 * with an identifier.
 * \param filepath Original filepath to be updated.  \e filepath can be empty *
 * or have and extension.
 * \param id Identifier to be attached to the origin filepath.
 * \return A new string represents modified \e filepath with the \e identifier.
 *
 * \example
 * \code
 * assert(make_new_name("example.dat", 0) == "example-0.dat");
 * assert(make_new_name("", 0) == "worker-0.dat");
 * assert(make_new_name("x.y.example.dat", 0) == "x.y.example-0.dat");
 * assert(make_new_name(".zozo", 0) == "worker-0.dat");
 * \endcode
 * \endexample
 */
inline std::string make_new_name(const std::string& filepath,
                                 unsigned int id) noexcept;

/**
 * Return number of available concurrency processor.
 * @return An integer greater or equal to 1.
 */
unsigned get_hardware_concurrency() noexcept;

void tokenize(const std::string& str,
              std::vector<std::string>& tokens,
              const std::string& delim,
              bool trimEmpty);

inline constexpr std::size_t
max_value(int need, std::size_t real) noexcept
{
    return need <= 0 ? real : std::min(static_cast<std::size_t>(need), real);
}

inline unsigned
hardware_concurrency_from_std() noexcept
{
    unsigned ret = std::thread::hardware_concurrency();

    return ret == 0 ? 1 : ret;
}

inline std::string
vstringf(const char* format, va_list ap) noexcept
{
    try {
        std::string buffer(256, '\0');
        int sz;

        for (;;) {
            sz = std::vsnprintf(&buffer[0], buffer.size(), format, ap);

            if (sz < 0)
                return {};

            if (static_cast<std::size_t>(sz) < buffer.size())
                return buffer;

            buffer.resize(sz + 1);
        }
    } catch (const std::exception& /*e*/) {
    }

    return std::string();
}

inline std::string
stringf(const char* format, ...) noexcept
{
    va_list ap;

    va_start(ap, format);
    std::string ret = vstringf(format, ap);
    va_end(ap);

    return ret;
}

inline unsigned
get_hardware_concurrency() noexcept
{
#ifdef __unix__
    long nb_procs = ::sysconf(_SC_NPROCESSORS_ONLN);
    if (nb_procs == -1)
        return hardware_concurrency_from_std();

    return static_cast<unsigned>(nb_procs);
#else
    return hardware_concurrency_from_std();
#endif
}

inline std::string
make_new_name(const std::string& filepath, unsigned int id) noexcept
{
    try {
        std::ostringstream os;

        if (filepath.empty()) {
            os << "worker-" << id << ".log";
            return os.str();
        }

        auto dotposition = filepath.find_last_of('.');
        if (dotposition == 0u) {
            os << "worker-" << id << ".log";
            return os.str();
        }

        if (dotposition == filepath.size() - 1) {
            os << filepath.substr(0, dotposition) << '-' << id;
            return os.str();
        }

        os << filepath.substr(0, dotposition) << '-' << id
           << filepath.substr(dotposition + 1);
        return os.str();
    } catch (const std::bad_alloc&) {
        return std::string();
    }
}

inline void
tokenize(const std::string& str,
         std::vector<std::string>& tokens,
         const std::string& delim,
         bool trimEmpty)
{
    tokens.clear();
    std::string::size_type pos, lastPos = 0, length = str.length();

    using value_type = typename std::vector<std::string>::value_type;
    using size_type = typename std::vector<std::string>::size_type;

    while (lastPos < length + 1) {
        pos = str.find_first_of(delim, lastPos);
        if (pos == std::string::npos) {
            pos = length;
        }
        if (pos != lastPos || !trimEmpty)
            tokens.push_back(
              value_type(str.data() + lastPos, (size_type)pos - lastPos));
        lastPos = pos + 1;
    }
}

} // namespace efyj

#endif
