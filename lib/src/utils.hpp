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

#include <EASTL/algorithm.h>
#include <EASTL/functional.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace efyj {

inline constexpr std::size_t
max_value(int need, size_t real) noexcept;

struct scope_exit
{
    scope_exit(eastl::function<void(void)> fct)
      : fct(fct)
    {}

    ~scope_exit()
    {
        fct();
    }

    eastl::function<void(void)> fct;
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
eastl::string
make_new_name(const eastl::string& filepath, unsigned int id) noexcept;

/**
 * Return number of available concurrency processor.
 * @return An integer greater or equal to 1.
 */
unsigned
get_hardware_concurrency() noexcept;

void
tokenize(const eastl::string& str,
         eastl::vector<eastl::string>& tokens,
         const eastl::string& delim,
         bool trimEmpty);

inline constexpr size_t
max_value(int need, size_t real) noexcept
{
    return need <= 0 ? real : eastl::min(static_cast<size_t>(need), real);
}

// inline unsigned
// get_hardware_concurrency() noexcept
// {
// #ifdef __unix__
//     long nb_procs = ::sysconf(_SC_NPROCESSORS_ONLN);
//     if (nb_procs == -1)
//         return hardware_concurrency_from_std();

//     return static_cast<unsigned>(nb_procs);
// #else
//     return hardware_concurrency_from_std();
// #endif
// }

inline eastl::string
make_new_name(const eastl::string& filepath, unsigned int id) noexcept
{
    if (filepath.empty())
        return eastl::string("worker-%u.log", id);

    auto dotposition = filepath.find_last_of('.');
    if (dotposition == 0u)
        return eastl::string("worker-%u.log", id);

    eastl::string ret = filepath.substr(0, dotposition);

    if (dotposition == filepath.size() - 1) {
        ret.append_sprintf("-%u.log");
    } else {
        ret.append_sprintf("-%u.", id);
        ret.append(filepath.substr(dotposition + 1));
    }

    return ret;
}

inline void
tokenize(const eastl::string& str,
         eastl::vector<eastl::string>& tokens,
         const eastl::string& delim,
         bool trimEmpty)
{
    tokens.clear();
    eastl::string::size_type pos, lastPos = 0, length = str.length();

    using value_type = typename eastl::vector<eastl::string>::value_type;
    using size_type = typename eastl::vector<eastl::string>::size_type;

    while (lastPos < length + 1) {
        pos = str.find_first_of(delim, lastPos);
        if (pos == eastl::string::npos) {
            pos = length;
        }
        if (pos != lastPos || !trimEmpty)
            tokens.push_back(
              value_type(str.data() + lastPos, (size_type)pos - lastPos));
        lastPos = pos + 1;
    }
}

/**
 * @brief Return true if @c Source can be casted into @c Target integer type.
 * @details Check if the integer type @c Source is castable into @c Target.
 *
 * @param arg A value.
 * @tparam Source An integer type.
 * @tparam Target An integer type.
 * @return true if @c static_cast<Target>(Source) is valid.
 */
template<typename Target, typename Source>
inline bool
is_numeric_castable(Source arg)
{
    static_assert(eastl::is_integral<Source>::value, "Integer required.");
    static_assert(eastl::is_integral<Target>::value, "Integer required.");

    using arg_traits = eastl::numeric_limits<Source>;
    using result_traits = eastl::numeric_limits<Target>;

    if (result_traits::digits == arg_traits::digits and
        result_traits::is_signed == arg_traits::is_signed)
        return true;

    if (result_traits::digits > arg_traits::digits)
        return result_traits::is_signed or arg >= 0;

    if (arg_traits::is_signed and
        arg < static_cast<Source>(result_traits::min()))
        return false;

    return arg <= static_cast<Source>(result_traits::max());
}

/**
 * @brief Tries to convert @c Source into @c Target integer.
 * @code
 * eastl::vector<int> v(1000, 5);
 *
 * // No exception.
 * int i = efyj::numeric_cast<int>(v.size());
 *
 * // Throw exception.
 * int8_t j = efyj::numeric_cast<int8_t>(v.size());
 * @endcode
 *
 * @param arg A value.
 * @tparam Source An integer type.
 * @tparam Target An integer type.
 * @return @c static_cast<Target>(Source) integer.
 */
template<typename Target, typename Source>
inline Target
numeric_cast(Source s)
{
    if (not is_numeric_castable<Target>(s))
        throw numeric_cast_error();

    return static_cast<Target>(s);
}

} // namespace efyj

#endif
