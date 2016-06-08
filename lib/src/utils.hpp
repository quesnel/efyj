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

#ifndef INRA_EFYj_UTILS_HPP
#define INRA_EFYj_UTILS_HPP

#include <functional>
#include <sstream>
#include <cstdint>
#include <cinttypes>

#if defined __GNUC__
#define EFYJ_GCC_PRINTF(format__, args__)                                     \
    __attribute__((format(printf, format__, args__)))
#endif

namespace efyj
{

std::string stringf(const char *format, ...) EFYJ_GCC_PRINTF(1, 2);

inline void Expects(bool condition)
{
    if (not condition)
        throw std::invalid_argument("");
}

inline void Expects(bool condition, const char *msg)
{
    if (not condition)
        throw std::invalid_argument(msg);
}

struct scope_exit {
    scope_exit(std::function<void(void)> fct)
        : fct(fct)
    {
    }

    ~scope_exit() { fct(); }

    std::function<void(void)> fct;
};

/** \e make_new_name is used to create new file path with a suffix composed
 *with
 * an identifier.
 * \param filepath Original filepath to be updated.  \e filepath can be empty
 *or
 * have and extension.
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
inline std::string make_new_name(const std::string &filepath,
                                 unsigned int id) noexcept
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
    } catch (const std::bad_alloc &) {
        return std::string();
    }
}

/**
 * Return number of available concurrency processor.
 * @return An integer greater or equal to 1.
 */
unsigned get_hardware_concurrency() noexcept;
}

#endif
