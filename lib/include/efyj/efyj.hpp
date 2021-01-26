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
#include <memory>
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

struct model_data
{
    std::map<std::string, std::vector<std::string>> attributes;
    std::vector<int> basic_attributes;
    int number;
};

struct options_data
{
    std::vector<std::string> simulations;
    std::vector<std::string> places;
    std::vector<int> departments;
    std::vector<int> years;
    std::vector<int> observed;
    std::vector<std::string> basic_attribute_names;
    matrix<value> options;
};

struct information_results
{
    information_results() = default;

    information_results(const dexi_parser_status status_) noexcept
      : status(status_)
    {}

    std::vector<std::string> basic_attribute_names;
    std::vector<int> basic_attribute_scale_value_numbers;

    dexi_parser_status status;
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

struct static_evaluation_results
{
    static_evaluation_results() = default;

    static_evaluation_results(const dexi_parser_status& status)
      : parser_status{ status }
    {}

    static_evaluation_results(evaluation_results&& results_)
      : results(std::move(results_))
    {}

    evaluation_results results;
    std::optional<dexi_parser_status> parser_status;
};

struct modifier
{
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

class context;

EFYJ_API
std::shared_ptr<context>
make_context(int log_priority = 6);

EFYJ_API
void
set_logger_callback(std::shared_ptr<context> ctx,
                    std::function<void(int, const std::string&)> cb);

EFYJ_API
model_data
extract_model(std::shared_ptr<context> ctx,
              const std::string& model_file_path);

EFYJ_API
evaluation_results
evaluate(std::shared_ptr<context> ctx,
         const std::string& model_file_path,
         const std::string& options_file_path);

EFYJ_API
evaluation_results
evaluate(std::shared_ptr<context> ctx,
         const std::string& model_file_path,
         const options_data& opts);

EFYJ_API
information_results
static_information(const std::string& model_file_path) noexcept;

EFYJ_API
static_evaluation_results
static_evaluate(const std::string& model_file_path,
                const std::vector<std::string>& simulations,
                const std::vector<std::string>& places,
                const std::vector<int> departments,
                const std::vector<int> years,
                const std::vector<int> observed,
                const std::vector<int>& scale_values) noexcept;

EFYJ_API
std::vector<result>
adjustment(std::shared_ptr<context> ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           bool reduce,
           int limit,
           unsigned int thread);

EFYJ_API
std::vector<result>
prediction(std::shared_ptr<context> ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           bool reduce,
           int limit,
           unsigned int thread);

EFYJ_API
void
extract_options_to_file(std::shared_ptr<context> ctx,
                        const std::string& model_file_path,
                        const std::string& output_file_path);

EFYJ_API
options_data
extract_options(std::shared_ptr<context> ctx,
                const std::string& model_file_path);

EFYJ_API
options_data
extract_options(std::shared_ptr<context> ctx,
                const std::string& model_file_path,
                const std::string& options_file_path);

EFYJ_API
void
merge_options(std::shared_ptr<context> ctx,
              const std::string& model,
              const std::string& options,
              const std::string& output_file_path);

} // namespace efyj

#endif
