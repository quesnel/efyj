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

#ifndef ORG_VLEPROJECT_EFYJ_EFYJ_HPP
#define ORG_VLEPROJECT_EFYJ_EFYJ_HPP

#define EFYJ_MAJOR_VERSION 0
#define EFYJ_MINOR_VERSION 6
#define EFYJ_PATCH_VERSION 0

#include <EASTL/map.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include <stdexcept>

#include <efyj/matrix.hpp>

/** Comments about efyj's API.
 */
namespace efyj {

using value = int; // std::int8_t;

struct model_data
{
    eastl::map<eastl::string, eastl::vector<eastl::string>> attributes;
    eastl::vector<int> basic_attributes;
    int number;
};

struct options_data
{
    eastl::vector<eastl::string> simulations;
    eastl::vector<eastl::string> places;
    eastl::vector<int> departments;
    eastl::vector<int> years;
    eastl::vector<int> observed;
    matrix<value> options;
};

struct simulation_results
{
    matrix<value> options;
    matrix<value> attributes;
    eastl::vector<value> simulations;
};

struct evaluation_results
{
    matrix<value> options;
    matrix<value> attributes;
    eastl::vector<value> simulations;
    eastl::vector<value> observations;
    matrix<value> confusion;
    double linear_weighted_kappa;
    double squared_weighted_kappa;
};

struct modifier
{
    int attribute;
    int line;
    int value;
};

struct result
{
    eastl::vector<modifier> modifiers;
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
 * @brief An internal exception when an integer cast fail.
 * @details @c numeric_cast_error is used with the
 * @e \c efyj::numeric_cast<TargetT>(SourceT) function that cast
 * integer type to another.
 */
struct numeric_cast_error : public std::exception
{
    virtual const char* what() const noexcept
    {
        return "numeric cast error: loss of range in numeric_cast";
    }
};

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

struct internal_error
{
    eastl::string pp_file, pp_function;
    int pp_line;

    internal_error(const eastl::string& file,
                   const eastl::string& function,
                   int line)
      : pp_file(file)
      , pp_function(function)
      , pp_line(line)
    {}
};

struct file_error
{
    eastl::string pp_file;

    file_error(const eastl::string& file)
      : pp_file(file)
    {}
};

struct solver_error
{
    eastl::string pp_file;

    solver_error(const eastl::string& file)
      : pp_file(file)
    {}
};

struct dexi_parser_status
{
    enum class tag
    {
        done = 0,
        scale_definition_error,
        scale_not_found,
        scale_too_big,
        file_format_error,
        not_enough_memory,
        element_unknown,
        option_conversion_error,
    };

    unsigned long int m_line, m_column;
    tag m_tag;

    dexi_parser_status(dexi_parser_status::tag t,
                       unsigned long int line,
                       unsigned long int column)
      : m_line(line)
      , m_column(column)
      , m_tag(t)
    {}
};

struct csv_parser_status
{
    enum class tag
    {
        file_error,
        column_number_incorrect,
        scale_value_unknown,
        column_conversion_failure,
        basic_attribute_unknown
    };

    csv_parser_status(csv_parser_status::tag tag, size_t line, size_t column)
      : m_line(line)
      , m_column(column)
      , m_tag(tag)
    {}

    size_t m_line;
    size_t m_column;
    tag m_tag;
};

enum class message_type
{
    classic,
    highlight,   // underline
    observation, // green and bold
    important    // red and bold
};

class logger
{
public:
    logger() = default;
    virtual ~logger() = default;

    virtual void write(message_type m,
                       const char* format,
                       va_list args) noexcept = 0;
};

class context
{
    int m_log_priority = 7;
    eastl::unique_ptr<logger> m_logger;

public:
    context() = default;
    virtual ~context() = default;

    context(const context&) = delete;
    context& operator=(const context&) = delete;

    void set_log_priority(int priority) noexcept;

    int get_log_priority() const noexcept;

    void set_logger(eastl::unique_ptr<logger> fn) noexcept;

    void log(message_type type, const char* format, ...)
#if defined(__GNUC__)
      noexcept __attribute__((format(printf, 3, 4)));
#else
      noexcept;
#endif
};

model_data
extract_model(eastl::shared_ptr<context> ctx,
              const eastl::string& model_file_path);

simulation_results
simulate(eastl::shared_ptr<context> ctx,
         const eastl::string& model_file_path,
         const eastl::string& options_file_path);

simulation_results
simulate(eastl::shared_ptr<context> ctx,
         const eastl::string& model_file_path,
         const options_data& opts);

evaluation_results
evaluate(eastl::shared_ptr<context> ctx,
         const eastl::string& model_file_path,
         const eastl::string& options_file_path);

evaluation_results
evaluate(eastl::shared_ptr<context> ctx,
         const eastl::string& model_file_path,
         const options_data& opts);

eastl::vector<result>
adjustment(eastl::shared_ptr<context> ctx,
           const eastl::string& model_file_path,
           const eastl::string& options_file_path,
           bool reduce,
           int limit,
           unsigned int thread);

eastl::vector<result>
prediction(eastl::shared_ptr<context> ctx,
           const eastl::string& model_file_path,
           const eastl::string& options_file_path,
           bool reduce,
           int limit,
           unsigned int thread);

options_data
extract_options(eastl::shared_ptr<context> ctx,
                const eastl::string& model_file_path);

options_data
extract_options(eastl::shared_ptr<context> ctx,
                const eastl::string& model_file_path,
                const eastl::string& options_file_path);
} // namespace efyj

#endif
