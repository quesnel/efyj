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

#include <string>
#include <string_view>
#include <vector>

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

enum class status : int
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

    dexi_writer_error,

    csv_parser_file_error,
    csv_parser_column_number_incorrect,
    csv_parser_scale_value_unknown,
    csv_parser_column_conversion_failure,
    csv_parser_basic_attribute_unknown,
    csv_parser_init_dataset_simulation_empty,
    csv_parser_init_dataset_cast_error,

    extract_option_same_input_files,
    extract_option_fail_open_file,

    merge_option_same_inputoutput,
    merge_option_fail_open_file,

    option_input_inconsistent,
    scale_value_inconsistent,
    option_too_many,

    unknown_error
};

template<typename T, typename... Args>
constexpr bool
match(const T& s, Args... args) noexcept
{
    return ((s == args) || ... || false);
}

inline bool
is_bad(const status t) noexcept
{
    return t != status::success;
}

inline bool
is_success(const status t) noexcept
{
    return t == status::success;
}

inline const char*
get_error_message(const efyj::status s) noexcept
{
    const static char* ret[] = { "success",
                                 "not enough memory",
                                 "numeric cast error",
                                 "internal error",
                                 "file error",
                                 "solver error",
                                 "unconsistent input vector",
                                 "dexi parser scale definition error",
                                 "dexi parser scale not found",
                                 "dexi parser scale too big",
                                 "dexi parser file format error",
                                 "dexi parser not enough memory",
                                 "dexi parser element unknown",
                                 "dexi parser option conversion error",
                                 "dexi writer error",
                                 "csv parser file error",
                                 "csv parser column number incorrect",
                                 "csv parser scale value unknown",
                                 "csv parser column conversion failure",
                                 "csv parser basic attribute unknown",
                                 "csv parser init dataset simulation empty",
                                 "csv parser init dataset cast error",
                                 "extract option same input files",
                                 "extract option fail open file",
                                 "merge option same inputoutput",
                                 "merge option fail open file",
                                 "option input inconsistent",
                                 "scale value inconsistent",
                                 "option too any",
                                 "unknown error" };

    const auto elem = static_cast<int>(s);
    const auto max_elem = std::size(ret);

    assert(static_cast<size_t>(elem) < max_elem);

    return ret[elem];
}

enum class log_level : int
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

struct context
{
    std::ostream* out = nullptr;
    std::ostream* err = nullptr;
    int line = 0;
    int column = 0;
    size_t size = 0;
    std::string data_1;
    efyj::status status;
    log_level log_priority = log_level::info;
};

using value = int; // std::int8_t;

struct information_results
{
    std::vector<std::string> basic_attribute_names;
    std::vector<int> basic_attribute_scale_value_numbers;

    void clear()
    {
        basic_attribute_names.clear();
        basic_attribute_scale_value_numbers.clear();
    }
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

    void clear()
    {
        options.clear();
        attributes.clear();
        simulations.clear();
        observations.clear();
        confusion.clear();
        linear_weighted_kappa = 0.0;
        squared_weighted_kappa = 0.0;
    }
};

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

    void clear()
    {
        modifiers.clear();
        kappa = 0.0;
        time = 0.0;
        kappa_computed = 0;
        function_computed = 0;
    }
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
 *
 * @todo Replace with a function_ref or function_view.
 */
using result_callback = bool (*)(const result&, void* user_data_result);

using check_user_interrupt_callback = void (*)(void* user_data_interrupt);

EFYJ_API
status
information(context& ctx,
            const std::string& model_file_path,
            information_results& ret) noexcept;

EFYJ_API
status
evaluate(context& ctx,
         const std::string& model_file_path,
         const data& d,
         evaluation_results& ret) noexcept;

EFYJ_API
status
evaluate(context& ctx,
         const std::string& model_file_path,
         const std::string& options_file_path,
         evaluation_results& ret) noexcept;

EFYJ_API status
adjustment(context& ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           result_callback callback,
           void* user_data_callback,
           check_user_interrupt_callback interrupt,
           void* user_data_interrupt,
           bool reduce,
           int limit,
           unsigned int thread) noexcept;

EFYJ_API status
adjustment(context& ctx,
           const std::string& model_file_path,
           const data& d,
           result_callback callback,
           void* user_data_callback,
           check_user_interrupt_callback interrupt,
           void* user_data_interrupt,
           bool reduce,
           int limit,
           unsigned int thread) noexcept;

EFYJ_API status
prediction(context& ctx,
           const std::string& model_file_path,
           const data& d,
           result_callback callback,
           void* user_data_callback,
           check_user_interrupt_callback interrupt,
           void* user_data_interrupt,
           bool reduce,
           int limit,
           unsigned int thread) noexcept;

EFYJ_API status
prediction(context& ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           result_callback callback,
           void* user_data_callback,
           check_user_interrupt_callback interrupt,
           void* user_data_interrupt,
           bool reduce,
           int limit,
           unsigned int thread) noexcept;

EFYJ_API status
extract_options_to_file(context& ctx,
                        const std::string& model_file_path,
                        const std::string& output_file_path) noexcept;

EFYJ_API status
extract_options(context& ctx,
                const std::string& model_file_path,
                data& out) noexcept;

EFYJ_API status
extract_options(context& ctx,
                const std::string& model_file_path,
                const data& d) noexcept;

EFYJ_API status
extract_options(context& ctx,
                const std::string& model_file_path,
                const std::string& options_file_path,
                data& out) noexcept;

EFYJ_API status
merge_options_to_file(context& ctx,
                      const std::string& model,
                      const std::string& options,
                      const std::string& output_file_path) noexcept;

EFYJ_API status
merge_options(context& ctx,
              const std::string& model_file_path,
              const std::string& output_file_path,
              const data& d) noexcept;

} // namespace efyj

#endif
