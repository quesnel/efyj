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

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <stdexcept>

#include <efyj/matrix.hpp>

#if defined _WIN32 || defined __CYGWIN__
#define EFYJ_HELPER_DLL_IMPORT __declspec(dllimport)
#define EFYJ_HELPER_DLL_EXPORT __declspec(dllexport)
#define EFYJ_HELPER_DLL_LOCAL
#else
#if __GNUC__ >= 4
#define EFYJ_HELPER_DLL_IMPORT __attribute__((visibility("default")))
#define EFYJ_HELPER_DLL_EXPORT __attribute__((visibility("default")))
#define EFYJ_HELPER_DLL_LOCAL __attribute__((visibility("hidden")))
#else
#define EFYJ_HELPER_DLL_IMPORT
#define EFYJ_HELPER_DLL_EXPORT
#define EFYJ_HELPER_DLL_LOCAL
#endif
#endif

#ifdef EFYJ_DLL
#ifdef libefyj_EXPORTS
#define EFYJ_API EFYJ_HELPER_DLL_EXPORT
#else
#define EFYJ_API EFYJ_HELPER_DLL_IMPORT
#endif
#define EFYJ_LOCAL EFYJ_HELPER_DLL_LOCAL
#define EFYJ_MODULE EFYJ_HELPER_DLL_EXPORT
#else
#define EFYJ_API
#define EFYJ_LOCAL
#define EFYJ_MODULE BARYONYX_HELPER_DLL_EXPORT
#endif

/** Comments about efyj's API.
 */
namespace efyj {

enum class status
{
    success,
    not_enough_memory,
    numeric_cast_error,
    internal_error,
    file_error,
    solver_error,

    unconsistent_input_vector,

    dexi_parser_scale_definition_error,
    dexi_parser_scale_not_found,
    dexi_parser_scale_too_big,
    dexi_parser_file_format_error,
    dexi_parser_not_enough_memory,
    dexi_parser_element_unknown,
    dexi_parser_option_conversion_error,

    csv_parser_file_error,
    csv_parser_column_number_incorrect,
    csv_parser_scale_value_unknown,
    csv_parser_column_conversion_failure,
    csv_parser_basic_attribute_unknown,

    extract_option_same_input_files,
    extract_option_fail_open_file,

    merge_option_same_inputoutput,
    merge_option_fail_open_file,

    option_input_inconsistent,

    unknown_error
};

enum class log_level
{
    emerg,   // system is unusable
    alert,   // action must be taken immediately
    crit,    // critical conditions
    err,     // error conditions
    warning, // warning conditions
    notice,  // normal but significant condition
    info,    // informational
    debug    // debug-level messages
};

using dexi_parser_callback = std::function<
  void(const status s, int line, int column, const std::string_view tag)>;

using csv_parser_callback =
  std::function<void(const status s, int line, int column)>;

using not_enough_memory_callback = std::function<void()>;
using numeric_cast_error_callback = std::function<void()>;
using internal_error_callback = std::function<void()>;
using solver_error_callback = std::function<void()>;
using file_error_callback =
  std::function<void(const std::string_view file_name)>;

struct context
{
    dexi_parser_callback dexi_cb;
    csv_parser_callback csv_cb;
    not_enough_memory_callback eov_cb;
    numeric_cast_error_callback cast_cb;
    solver_error_callback solver_cb;
    file_error_callback file_cb;

    log_level log_priority = log_level::info;
};

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

struct internal_error
{
    std::string pp_file, pp_function;
    int pp_line;

    internal_error(const std::string& file,
                   const std::string& function,
                   int line)
      : pp_file(file)
      , pp_function(function)
      , pp_line(line)
    {}
};

struct file_error
{
    std::string pp_file;

    file_error(const std::string& file)
      : pp_file(file)
    {}
};

struct solver_error
{
    std::string pp_file;

    solver_error(const std::string& file)
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

    unsigned long int m_line = 0u, m_column = 0u;
    tag m_tag = tag::done;

    std::string_view tag_name() const noexcept
    {
        static std::string_view name[] = {
            "done",
            "scale_definition_error",
            "scale_not_found",
            "scale_too_big",
            "file_format_error",
            "not_enough_memory",
            "element_unknown",
            "option_conversion_error",
        };

        return name[static_cast<int>(m_tag)];
    }

    dexi_parser_status() = default;

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

    std::string_view tag_name() const noexcept
    {
        static std::string_view name[] = { "file_error",
                                           "column_number_incorrect",
                                           "scale_value_unknown",
                                           "column_conversion_failure",
                                           "basic_attribute_unknown" };

        return name[static_cast<int>(m_tag)];
    }

    size_t m_line;
    size_t m_column;
    tag m_tag;
};

using value = int; // std::int8_t;

struct information_results
{
    std::vector<std::string> basic_attribute_names;
    std::vector<int> basic_attribute_scale_value_numbers;
};

struct evaluation_results
{
    matrix<value> options;
    matrix<value> attributes;
    std::vector<value> simulations;
    std::vector<value> observations;
    matrix<value> confusion;
    double linear_weighted_kappa;
    double squared_weighted_kappa;
};

inline bool
is_success(const status& st) noexcept
{
    return st == status::success;
}

inline bool
is_bad(const status& st) noexcept
{
    return st != status::success;
}

struct modifier
{
    modifier() noexcept = default;

    modifier(const int attribute_, const int line_, const int value_) noexcept
      : attribute(attribute_)
      , line(line_)
      , value(value_)
    {}

    int attribute;
    int line;
    int value;
};

struct result
{
    std::vector<modifier> modifiers;
    double kappa;
    double time;
    unsigned long int kappa_computed;
    unsigned long int function_computed;
};

struct data
{
    std::vector<std::string> simulations;
    std::vector<std::string> places;
    std::vector<int> departments;
    std::vector<int> years;
    std::vector<int> observed;
    std::vector<int> scale_values;

    size_t rows() const noexcept
    {
        return simulations.size();
    }

    size_t cols() const noexcept
    {
        const auto r = rows();
        return r ? scale_values.size() / r : (size_t)0;
    }

    bool is_size_valid() const noexcept
    {
        const auto r = rows();
        return r == places.size() && r == departments.size() &&
               r == years.size() && r == observed.size() &&
               scale_values.size() % r == 0;
    }
};

/**
 * @brief Use during the @c adjustment or @c prediction function call to show
 * compuation results.
 *
 * This function can return false to stop the computation of the next line
 * modifier.
 */
using result_callback = std::function<bool(const result&)>;

EFYJ_API
status
information(const context& ctx,
            const std::string& model_file_path,
            information_results& ret) noexcept;

EFYJ_API
status
evaluate(const context& ctx,
         const std::string& model_file_path,
         const data& d,
         evaluation_results& ret) noexcept;

EFYJ_API
status
evaluate(const context& ctx,
         const std::string& model_file_path,
         const std::string& options_file_path,
         evaluation_results& ret) noexcept;

EFYJ_API status
adjustment(const context& ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           const result_callback& callback,
           bool reduce,
           int limit,
           unsigned int thread) noexcept;

EFYJ_API status
adjustment(const context& ctx,
           const std::string& model_file_path,
           const data& d,
           const result_callback& callback,
           bool reduce,
           int limit,
           unsigned int thread) noexcept;

EFYJ_API status
prediction(const context& ctx,
           const std::string& model_file_path,
           const data& d,
           const result_callback& callback,
           bool reduce,
           int limit,
           unsigned int thread) noexcept;

EFYJ_API status
prediction(const context& ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           const result_callback& callback,
           bool reduce,
           int limit,
           unsigned int thread) noexcept;

EFYJ_API status
extract_options(const context& ctx,
                const std::string& model_file_path,
                const std::string& output_file_path) noexcept;

EFYJ_API status
extract_options(const context& ctx,
                const std::string& model_file_path,
                data& out) noexcept;

EFYJ_API status
extract_options(const context& ctx,
                const std::string& model_file_path,
                const data& d) noexcept;

EFYJ_API status
merge_options(const context& ctx,
              const std::string& model,
              const std::string& options,
              const std::string& output_file_path) noexcept;

EFYJ_API status
merge_options(const context& ctx,
              const std::string& model_file_path,
              const std::string& output_file_path,
              const data& d) noexcept;

} // namespace efyj

#endif
