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

#ifndef ORG_VLEPROJECT_EFYJ_EFYJ_HPP
#define ORG_VLEPROJECT_EFYJ_EFYJ_HPP

#define EFYJ_MAJOR_VERSION 0
#define EFYJ_MINOR_VERSION 6
#define EFYJ_PATCH_VERSION 0

#include <efyj/matrix.hpp>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

/** Comments about efyj's API.
*/
namespace efyj {

using value = int; // std::int8_t;

struct model_data {
    std::map<std::string, std::vector<std::string>> attributes;
    std::vector<int> basic_attributes;
    int number;
};

struct options_data {
    std::vector<std::string> simulations;
    std::vector<std::string> places;
    std::vector<int> departments;
    std::vector<int> years;
    std::vector<int> observed;
    matrix<value> options;
};

struct simulation_results {
    matrix<value> options;
    matrix<value> attributes;
    std::vector<value> simulations;
};

struct evaluation_results {
    matrix<value> options;
    matrix<value> attributes;
    std::vector<value> simulations;
    std::vector<value> observations;
    matrix<value> confusion;
    double linear_weighted_kappa;
    double squared_weighted_kappa;
};

struct modifier {
    int attribute;
    int line;
    int value;
};

struct result {
    std::vector<modifier> modifiers;
    double kappa;
    double time;
    unsigned long int kappa_computed;
    unsigned long int function_computed;
};

/**
 * @brief Return true if @c Source can be casted into @c Target integer
 * type.
 * @details Check if the integer type @c Source is castable into @c Target.
 *
 * @param arg A value.
 * @tparam Source An integer type.
 * @tparam Target An integer type.
 * @return true if @c static_cast<Target>(Source) is valid.
 */
template <typename Target, typename Source>
inline bool is_numeric_castable(Source arg)
{
    static_assert(std::is_integral<Source>::value, "Integer required.");
    static_assert(std::is_integral<Target>::value, "Integer required.");

    using arg_traits = std::numeric_limits<Source>;
    using result_traits = std::numeric_limits<Target>;

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
 * @brief An internal exception when an integer cast fail.
 * @details @c numeric_cast_error is used with the
 * @e \c efyj::numeric_cast<TargetT>(SourceT) function that cast
 * integer type to another.
 */
struct numeric_cast_error : public std::exception {
    virtual const char *what() const noexcept
    {
        return "numeric cast error: loss of range in numeric_cast";
    }
};

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
template <typename Target, typename Source>
inline Target numeric_cast(Source s)
{
    if (not is_numeric_castable<Target>(s))
        throw numeric_cast_error();

    return static_cast<Target>(s);
}

class internal_error : std::logic_error {
    std::string pp_file, pp_function;
    int pp_line;

public:
    internal_error(const std::string &msg);
    internal_error(const std::string &file,
                   const std::string &function,
                   int line,
                   const std::string &msg);
    virtual ~internal_error() noexcept = default;

    std::string file() const;
    std::string function() const;
    int line() const;
};

class file_error : public std::runtime_error {
    std::string pp_file;

public:
    file_error(const std::string &file);

    virtual ~file_error() noexcept = default;

    std::string file() const;
};

class solver_error : public std::runtime_error {
public:
    solver_error(const std::string &msg);

    virtual ~solver_error() noexcept = default;
};

class dexi_parser_error : public std::runtime_error {
public:
    dexi_parser_error(const std::string &msg);

    dexi_parser_error(const std::string &msg, const std::string &filepath);

    dexi_parser_error(const std::string &msg, int line, int column, int error);

    virtual ~dexi_parser_error() noexcept = default;

    int line() const { return m_line; }
    int column() const { return m_column; }
    int internal_error_code() const { return m_internal_error_code; }
    std::string filepath() const { return m_filepath; }
    std::string message() const { return m_message; }

private:
    int m_line, m_column;
    int m_internal_error_code;
    std::string m_filepath;
    std::string m_message;
};

class csv_parser_error : public std::runtime_error {
public:
    csv_parser_error(const std::string &msg);

    csv_parser_error(const std::string &filepath, const std::string &msg);

    csv_parser_error(std::size_t line,
                     const std::string &filepath,
                     const std::string &msg);

    csv_parser_error(std::size_t line,
                     std::size_t column,
                     const std::string &filepath,
                     const std::string &msg);

    virtual ~csv_parser_error() noexcept = default;

    std::size_t line() const { return m_line; }
    std::size_t column() const { return m_column; }
    std::string filepath() const { return m_filepath; }
    std::string message() const { return m_msg; }

private:
    std::size_t m_line;
    std::size_t m_column;
    std::string m_filepath;
    std::string m_msg;
};

enum class message_type {
    classic,
    highlight,   // underline
    observation, // green and bold
    important    // red and bold
};

class logger {
public:
    logger() = default;
    virtual ~logger() = default;
    virtual void write(int priority,
                       const char *file,
                       int line,
                       const char *fn,
                       const char *format,
                       va_list args) noexcept = 0;

    virtual void
    write(message_type m, const char *format, va_list args) noexcept = 0;
};

class context {
    int m_log_priority = 7;
    std::unique_ptr<logger> m_logger;

public:
    context() = default;
    virtual ~context() = default;

    context(const context &) = delete;
    context &operator=(const context &) = delete;

    void set_log_priority(int priority) noexcept;

    int get_log_priority() const noexcept;

    void set_logger(std::unique_ptr<logger> fn) noexcept;

    void log(message_type type, const char *format, ...)
#if defined(__GNUC__)
        noexcept __attribute__((format(printf, 3, 4)));
#else
        noexcept;
#endif

    void log(int priority,
             const char *file,
             int line,
             const char *fn,
             const char *format,
             ...)
#if defined(__GNUC__)
        noexcept __attribute__((format(printf, 6, 7)));
#else
        noexcept;
#endif
};

model_data extract_model(std::shared_ptr<context> ctx,
                         const std::string &model_file_path);

simulation_results simulate(std::shared_ptr<context> context,
                            const std::string &model_file_path,
                            const std::string &options_file_path);

simulation_results simulate(std::shared_ptr<context> context,
                            const std::string &model_file_path,
                            const options_data &options);

evaluation_results evaluate(std::shared_ptr<context> context,
                            const std::string &model_file_path,
                            const std::string &options_file_path);

evaluation_results evaluate(std::shared_ptr<context> context,
                            const std::string &model_file_path,
                            const options_data &options);

options_data extract_options(std::shared_ptr<context> context,
                             const std::string &model_file_path);

options_data extract_options(std::shared_ptr<context> context,
                             const std::string &model_file_path,
                             const std::string &options_file_path);

} // namespace efyj

#include <efyj/details/efyj.hpp>

#endif
