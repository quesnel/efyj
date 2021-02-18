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

#include <algorithm>
#include <functional>
#include <limits>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <efyj/efyj.hpp>

namespace efyj {

/** Casts nonnegative integer to unsigned.
 */
template<typename Integer>
constexpr typename std::make_unsigned<Integer>::type
to_unsigned(Integer value)
{
    assert(value >= 0);

    return static_cast<typename std::make_unsigned<Integer>::type>(value);
}

/**
 * @brief Get a sub-string without any @c std::isspace characters at left.
 *
 * @param s The string-view to clean up.
 *
 * @return An update sub-string or the same string-view.
 */
inline std::string_view
left_trim(std::string_view s) noexcept
{
    auto found = s.find_first_not_of(" \t\n\v\f\r");

    if (found == std::string::npos)
        return {};

    return s.substr(found, std::string::npos);
}

/**
 * @brief Get a sub-string without any @c std::isspace characters at right.
 *
 * @param s The string-view to clean up.
 *
 * @return An update sub-string or the same string-view.
 */
inline std::string_view
right_trim(std::string_view s) noexcept
{
    auto found = s.find_last_not_of(" \t\n\v\f\r");

    if (found == std::string::npos)
        return {};

    return s.substr(0, found + 1);
}

/**
 * @brief Get a sub-string without any @c std::isspace characters at left and
 * right
 *
 * @param s The string-view to clean up.
 *
 * @return An update sub-string or the same string-view.
 */
inline std::string_view
trim(std::string_view s) noexcept
{
    return left_trim(right_trim(s));
}

/**
 * @brief Compute the length of the @c container.
 * @details Return the @c size provided by the @c C::size() but cast it into a
 *     @c int. This is a specific baryonyx function, we know that number of
 *         variables and constraints are lower than the @c INT_MAX value.
 *
 * @code
 * std::vector<int> v(z);
 *
 * for (int i = 0, e = length(v); i != e; ++i)
 *     ...
 * @endcode
 *
 * @param c The container to request to size.
 * @tparam T The of container value (must provide a @c size() member).
 */
template<class C>
constexpr int
length(const C& c) noexcept
{
    return static_cast<int>(c.size());
}

/**
 * @brief Compute the length of the C array.
 * @details Return the size of the C array but cast it into a @c int. This
 * is a specific baryonyx function, we know that number of variables and
 *     constraints are lower than the  @c int max value (INT_MAX).
 *
 * @code
 * int v[150];
 * for (int i = 0, e = length(v); i != e; ++i)
 *     ...
 * @endcode
 *
 * @param v The container to return size.
 * @tparam T The type of the C array.
 * @tparam N The size of the C array.
 */
template<class T, size_t N>
constexpr int
length(const T (&array)[N]) noexcept
{
    (void)array;

    return static_cast<int>(N);
}

inline constexpr std::size_t
max_value(int need, size_t real) noexcept;

struct scope_exit
{
    scope_exit(std::function<void(void)> fct)
      : fct(fct)
    {}

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
std::string
make_new_name(const std::string& filepath, unsigned int id) noexcept;

/**
 * Return number of available concurrency processor.
 * @return An integer greater or equal to 1.
 */
unsigned
get_hardware_concurrency() noexcept;

void
tokenize(const std::string& str,
         std::vector<std::string>& tokens,
         const std::string& delim,
         bool trimEmpty);

inline constexpr size_t
max_value(int need, size_t real) noexcept
{
    return need <= 0 ? real : std::min(static_cast<size_t>(need), real);
}

inline std::string
make_new_name(const std::string& filepath, unsigned int id) noexcept
{
    if (filepath.empty())
        return fmt::format("worker-{}.log", id);

    auto dotposition = filepath.find_last_of('.');
    if (dotposition == 0u)
        return fmt::format("worker-{}.log", id);

    if (dotposition == filepath.size() - 1)
        return fmt::format("{}-{}.log", filepath.substr(0, dotposition), id);

    return fmt::format("{}-{}{}.log",
                       filepath.substr(0, dotposition),
                       id,
                       filepath.substr(dotposition + 1));
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
    static_assert(std::is_integral<Source>::value, "Integer required.");
    static_assert(std::is_integral<Target>::value, "Integer required.");

    using arg_traits = std::numeric_limits<Source>;
    using result_traits = std::numeric_limits<Target>;

    if (result_traits::digits == arg_traits::digits &&
        result_traits::is_signed == arg_traits::is_signed)
        return true;

    if (result_traits::digits > arg_traits::digits)
        return result_traits::is_signed || arg >= 0;

    if (arg_traits::is_signed &&
        arg < static_cast<Source>(result_traits::min()))
        return false;

    return arg <= static_cast<Source>(result_traits::max());
}

/**
 * @brief Tries to convert @c Source into @c Target integer.
 * @code
 * std::vector<int> v(1000, 5);
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
    if (!is_numeric_castable<Target>(s))
        throw numeric_cast_error();

    return static_cast<Target>(s);
}

inline std::string_view
get_error_message(const efyj::status s) noexcept
{
    const static std::string_view ret[] = {
        "success",
        "not enough memory",
        "internal integer cast error",
        "internal error",
        "file access error",
        "internal solver error",
        "unconsistent input vector",
        "dexi file parser scale definition error",
        "dexi parser scale not found",
        "dexi parser scale too big",
        "dexi parser file format error",
        "dexi parser not enough memory",
        "dexi parser element unknown",
        "dexi parser option conversion error",
        "csv parser file error",
        "csv parser column number incorrect",
        "csv_parser scale value unknown",
        "csv parser column conversion failure",
        "csv parser basic attribute unknown",
        "extract option same input files",
        "extract option fail open file",
        "merge option same input/output dexi file",
        "merge option fail to open file",
        "option input vector inconsistent",
        "unknown error"
    };

    const auto elem = static_cast<size_t>(s);
    const auto max_elem = std::size(ret);

    assert(elem < max_elem);

    return ret[elem];
}

class output_file
{
private:
    std::FILE* file = nullptr;

public:
    output_file() noexcept = default;

    output_file(const char* file_path) noexcept
    {
#ifdef _WIN32
        if (fopen_s(&file, file_path, "w"))
            file = nullptr;
#else
            file = std::fopen(file_path, "w"));
#endif
    }

    output_file(output_file&& other) noexcept
      : file(other.file)
    {
        other.file = nullptr;
    }

    output_file& operator=(output_file&& other) noexcept
    {
        if (file) {
            std::fclose(file);
            file = nullptr;
        }

        file = other.file;
        other.file = nullptr;
        return *this;
    }

    output_file(const output_file&) = delete;
    output_file& operator=(const output_file&) = delete;

    std::FILE* get() const noexcept
    {
        return file;
    }

    bool is_open() const noexcept
    {
        return file != nullptr;
    }

    ~output_file()
    {
        if (file)
            std::fclose(file);
    }

    void vprint(const std::string_view format,
                const fmt::format_args args) const
    {
        fmt::vprint(file, format, args);
    }

    template<typename... Args>
    void print(const std::string_view format, const Args&... args) const
    {
        vprint(format, fmt::make_format_args(args...));
    }
};

class input_file
{
private:
    std::FILE* file = nullptr;

public:
    input_file() noexcept = default;

    input_file(const char* file_path) noexcept
    {
#ifdef _WIN32
        if (fopen_s(&file, file_path, "r"))
            file = nullptr;
#else
            file = std::fopen(file_path, "r"));
#endif
    }

    input_file(input_file&& other) noexcept
      : file(other.file)
    {
        other.file = nullptr;
    }

    input_file& operator=(input_file&& other) noexcept
    {
        if (file) {
            std::fclose(file);
            file = nullptr;
        }

        file = other.file;
        other.file = nullptr;
        return *this;
    }

    input_file(const input_file&) = delete;
    input_file& operator=(const input_file&) = delete;

    std::FILE* get() const noexcept
    {
        return file;
    }

    bool is_open() const noexcept
    {
        return file != nullptr;
    }

    ~input_file()
    {
        if (file)
            std::fclose(file);
    }
};

} // namespace efyj

#endif
