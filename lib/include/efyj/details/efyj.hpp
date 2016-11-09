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

#ifndef ORG_VLEPROJECT_EFYJ_DETAILS_EFYJ_HPP
#define ORG_VLEPROJECT_EFYJ_DETAILS_EFYJ_HPP

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#endif

#include <efyj/details/adjustment.hpp>
#include <efyj/details/efyj.hpp>
#include <efyj/details/model.hpp>
#include <efyj/details/options.hpp>
#include <efyj/details/post.hpp>
#include <efyj/details/prediction.hpp>
#include <efyj/details/private.hpp>
#include <efyj/details/solver-stack.hpp>
#include <efyj/details/utils.hpp>
#include <fstream>

namespace efyj {

namespace details {

inline std::string internal_error_format_message(const std::string &msg)
{
    return stringf("Internal error: %s", msg.c_str());
}

inline std::string internal_error_format_message(const std::string &file,
                                                 const std::string &function,
                                                 int line,
                                                 const std::string &msg)
{
    return stringf("Internal error at %s:%s line: %d, %s",
                   file.c_str(),
                   function.c_str(),
                   line,
                   msg.c_str());
}

inline std::string file_error_format_message(const std::string &file)
{
    return stringf("Access error to file `%s'", file.c_str());
}

inline std::string csv_parser_error_format(std::size_t line,
                                           std::size_t column,
                                           const std::string &filepath,
                                           const std::string &msg)
{
    if (filepath.empty())
        return stringf("CSV Error: %s", msg.c_str());

    if (line == 0u)
        return stringf(
            "CSV Error: file `%s': %s", filepath.c_str(), msg.c_str());
    if (column == 0u)
        return stringf("CSV Error: file `%s' line %ld: %s",
                       filepath.c_str(),
                       line,
                       msg.c_str());

    return stringf("CSV Error: file `%s' %ld:%ld: %s",
                   filepath.c_str(),
                   line,
                   column,
                   msg.c_str());
}

inline std::string dexi_parser_error_format(const std::string &msg)
{
    return stringf("DEXI error: %s", msg.c_str());
}

inline std::string dexi_parser_error_format(const std::string &msg,
                                            const std::string &filepath)
{
    return stringf("DEXI error: '%s' %s", filepath.c_str(), msg.c_str());
}

inline std::string dexi_parser_error_format(const std::string &msg,
                                            int line,
                                            int column,
                                            int error)
{
    return stringf("DEXI error: error %s at %d:%d, error code: %d",
                   msg.c_str(),
                   line,
                   column,
                   error);
}

inline std::string solver_error_format(const std::string &msg)
{
    return stringf("Solver error: %s", msg.c_str());
}

} // namespace details

inline internal_error::internal_error(const std::string &msg)
    : std::logic_error(details::internal_error_format_message(msg))
{
}

inline internal_error::internal_error(const std::string &file,
                                      const std::string &function,
                                      int line,
                                      const std::string &msg)
    : std::logic_error(
          details::internal_error_format_message(file, function, line, msg))
    , pp_file(file)
    , pp_function(function)
    , pp_line(line)
{
}

inline std::string internal_error::file() const
{
    return pp_file;
}

inline std::string internal_error::function() const
{
    return pp_function;
}

inline int internal_error::line() const
{
    return pp_line;
}

inline file_error::file_error(const std::string &file)
    : std::runtime_error(details::file_error_format_message(file))
    , pp_file(file)
{
}

inline std::string file_error::file() const
{
    return pp_file;
}

inline solver_error::solver_error(const std::string &msg)
    : std::runtime_error(details::solver_error_format(msg))
{
}

inline dexi_parser_error::dexi_parser_error(const std::string &msg)
    : std::runtime_error(details::dexi_parser_error_format(msg))
    , m_line(0)
    , m_column(0)
    , m_internal_error_code(0)
    , m_message(msg)
{
}

inline dexi_parser_error::dexi_parser_error(const std::string &msg,
                                            const std::string &filepath)
    : std::runtime_error(details::dexi_parser_error_format(filepath, msg))
    , m_line(0)
    , m_column(0)
    , m_internal_error_code(0)
    , m_filepath(filepath)
    , m_message(msg)
{
}
inline dexi_parser_error::dexi_parser_error(const std::string &msg,
                                            int line,
                                            int column,
                                            int error)
    : std::runtime_error(
          details::dexi_parser_error_format(msg, line, column, error))
    , m_line(line)
    , m_column(column)
    , m_internal_error_code(error)
    , m_message(msg)
{
}

inline csv_parser_error::csv_parser_error(const std::string &msg)
    : std::runtime_error(
          details::csv_parser_error_format(0, 0, std::string(), msg))
    , m_line(0)
    , m_column(0)
    , m_msg(msg)
{
}

inline csv_parser_error::csv_parser_error(const std::string &filepath,
                                          const std::string &msg)
    : std::runtime_error(details::csv_parser_error_format(0, 0, filepath, msg))
    , m_line(0)
    , m_column(0)
    , m_filepath(filepath)
    , m_msg(msg)
{
}

inline csv_parser_error::csv_parser_error(std::size_t line,
                                          const std::string &filepath,
                                          const std::string &msg)
    : std::runtime_error(
          details::csv_parser_error_format(line, 0, filepath, msg))
    , m_line(line)
    , m_column(0)
    , m_filepath(filepath)
    , m_msg(msg)
{
}

inline csv_parser_error::csv_parser_error(std::size_t line,
                                          std::size_t column,
                                          const std::string &filepath,
                                          const std::string &msg)
    : std::runtime_error(
          details::csv_parser_error_format(line, column, filepath, msg))
    , m_line(line)
    , m_column(column)
    , m_filepath(filepath)
    , m_msg(msg)
{
}

//     void set_options(const std::vector<std::string> &simulations,
//                      const std::vector<std::string> &places,
//                      const std::vector<int> &departments,
//                      const std::vector<int> &years,
//                      const std::vector<int> &observed,
//                      const std::vector<int> &options)
//     {
//         // TODO add test to ensure simulation.size() == departments.size()
//         // and etc.

//         pp_options.set(
//             simulations, places, departments, years, observed, options);
//     }

//     std::vector<result> compute_prediction(int line_limit,
//                                            double time_limit,
//                                            int reduce_mode) const
//     {
//         if (pp_options.empty() or pp_model.empty())
//             throw std::logic_error(
//                 "load options and model before trying prediction
//                 function.");

//         prediction_evaluator computer(pp_context, pp_model, pp_options);
//         return computer.run(line_limit, time_limit, reduce_mode);
//     }

//     std::vector<result> compute_adjustment(int line_limit,
//                                            double time_limit,
//                                            int reduce_mode) const
//     {
//         if (pp_options.empty() or pp_model.empty())
//             throw std::logic_error(
//                 "load options and model before trying adjustment
//                 function.");

//         adjustment_evaluator computer(pp_context, pp_model, pp_options);
//         return computer.run(line_limit, time_limit, reduce_mode);
//     }

//     void clear()
//     {
//         pp_options.clear();
//         pp_model.clear();
//     }
// };

inline void context::set_log_priority(int priority) noexcept
{
    m_log_priority = priority;
}

inline int context::get_log_priority() const noexcept
{
    return m_log_priority;
}

inline void context::set_logger(std::unique_ptr<logger> fn) noexcept
{
    m_logger = std::move(fn);
}

inline void context::log(message_type type, const char *format, ...) noexcept
{
    if (m_logger) {
        va_list args;

        va_start(args, format);
        m_logger->write(type, format, args);
        va_end(args);
    }
}

inline void context::log(int priority,
                         const char *file,
                         int line,
                         const char *fn,
                         const char *format,
                         ...) noexcept
{
    if (m_logger) {
        va_list args;

        va_start(args, format);
        m_logger->write(priority, file, line, fn, format, args);
        va_end(args);
    }
}

Model make_model(std::shared_ptr<context> ctx,
                 const std::string &model_file_path)
{
    Model model;

    std::ifstream ifs(model_file_path);
    if (not ifs) {
        vErr(ctx, "fail to open `%s'\n", model_file_path.c_str());
        throw file_error(model_file_path);
    }

    model.read(ifs);

    return model;
}

Options make_options(std::shared_ptr<context> ctx,
                     Model &model,
                     const std::string &options_file_path)
{
    Options options;

    std::ifstream ifs(options_file_path);
    if (not ifs) {
        vErr(ctx, "fail to open `%s'", options_file_path.c_str());
        throw file_error(options_file_path);
    }

    options.read(ctx, ifs, model);

    return options;
}

inline model_data extract_model(std::shared_ptr<context> ctx,
                                const std::string &model_file_path)
{
    auto model = make_model(ctx, model_file_path);
    model_data ret;

    ret.number = numeric_cast<int>(model.attributes.size());

    for (std::size_t i = 0, e = model.attributes.size(); i != e; ++i) {
        auto key = ret.attributes.emplace(model.attributes[i].name,
                                          std::vector<std::string>());

        for (auto &scale : model.attributes[i].scale.scale)
            key.first->second.emplace_back(scale.name);

        if (model.attributes[i].is_basic())
            ret.basic_attributes.emplace_back(i);
    }

    return ret;
}

inline simulation_results simulate(std::shared_ptr<context> ctx,
                                   const std::string &model_file_path,
                                   const std::string &options_file_path)
{
    auto model = make_model(ctx, model_file_path);
    auto options = make_options(ctx, model, options_file_path);

    const std::size_t max_opt = options.simulations.size();
    simulation_results ret;
    ret.options.resize(options.options.cols(), max_opt);
    // TODO ret.attributes.resize(COL, max_opt);
    ret.simulations.resize(max_opt);

    solver_stack solver(model);

    for (std::size_t opt = 0; opt != max_opt; ++opt)
        ret.simulations[opt] = solver.solve(options.options.row(opt));

    for (long int r = 0, end_r = options.options.rows(); r != end_r; ++r)
        for (long int c = 0, end_c = options.options.cols(); c != end_c; ++c)
            ret.options(c, r) = options.options(c, r);

    return ret;
}

inline simulation_results simulate(std::shared_ptr<context> ctx,
                                   const std::string &model_file_path,
                                   const options_data &opts)
{
    auto model = make_model(ctx, model_file_path);
    Options options;

    options.set(opts);

    const std::size_t max_opt = options.simulations.size();
    simulation_results ret;
    ret.options.resize(options.options.cols(), max_opt);
    // TODO ret.attributes.resize(COL, max_opt);
    ret.simulations.resize(max_opt);

    solver_stack solver(model);

    for (std::size_t opt = 0; opt != max_opt; ++opt)
        ret.simulations[opt] = solver.solve(options.options.row(opt));

    for (long int r = 0, end_r = options.options.rows(); r != end_r; ++r)
        for (long int c = 0, end_c = options.options.cols(); c != end_c; ++c)
            ret.options(c, r) = options.options(c, r);

    return ret;
}
inline evaluation_results evaluate(std::shared_ptr<context> ctx,
                                   const std::string &model_file_path,
                                   const std::string &options_file_path)
{
    auto model = make_model(ctx, model_file_path);
    auto options = make_options(ctx, model, options_file_path);

    solver_stack solver(model);
    const std::size_t max_opt = options.simulations.size();
    evaluation_results ret;
    ret.options.resize(options.options.cols(), max_opt);
    // TODO ret.attributes.resize(COL, max_opt);
    ret.simulations.resize(max_opt);
    ret.observations.resize(max_opt);
    ret.confusion.resize(
        model.attributes[0].scale.size(), model.attributes[0].scale.size(), 0);

    for (std::size_t opt = 0; opt != max_opt; ++opt) {
        ret.observations[opt] = options.observed[opt];
        ret.simulations[opt] = solver.solve(options.options.row(opt));
        ret.confusion(ret.observations[opt], ret.simulations[opt])++;
    }

    weighted_kappa_calculator kappa_c(model.attributes[0].scale.size());
    ret.squared_weighted_kappa =
        kappa_c.squared(ret.observations, ret.simulations);

    ret.linear_weighted_kappa =
        kappa_c.linear(ret.observations, ret.simulations);

    for (long int r = 0, end_r = options.options.rows(); r != end_r; ++r)
        for (long int c = 0, end_c = options.options.cols(); c != end_c; ++c)
            ret.options(c, r) = options.options(r, c);

    return ret;
}

inline evaluation_results evaluate(std::shared_ptr<context> ctx,
                                   const std::string &model_file_path,
                                   const options_data &opts)
{
    auto model = make_model(ctx, model_file_path);
    Options options;

    options.set(opts);

    solver_stack solver(model);
    const std::size_t max_opt = options.simulations.size();
    evaluation_results ret;
    ret.options.resize(options.options.cols(), max_opt);
    // TODO ret.attributes.resize(COL, max_opt);
    ret.simulations.resize(max_opt);
    ret.observations.resize(max_opt);
    ret.confusion.resize(
        model.attributes[0].scale.size(), model.attributes[0].scale.size(), 0);

    for (std::size_t opt = 0; opt != max_opt; ++opt) {
        ret.observations[opt] = options.observed[opt];
        ret.simulations[opt] = solver.solve(options.options.row(opt));
        ret.confusion(ret.observations[opt], ret.simulations[opt])++;
    }

    weighted_kappa_calculator kappa_c(model.attributes[0].scale.size());
    ret.squared_weighted_kappa =
        kappa_c.squared(ret.observations, ret.simulations);

    ret.linear_weighted_kappa =
        kappa_c.linear(ret.observations, ret.simulations);

    for (long int r = 0, end_r = options.options.rows(); r != end_r; ++r)
        for (long int c = 0, end_c = options.options.cols(); c != end_c; ++c)
            ret.options(c, r) = options.options(r, c);

    return ret;
}

inline options_data extract_options(std::shared_ptr<context> ctx,
                                    const std::string &model_file_path)
{
    auto model = make_model(ctx, model_file_path);

    return model.write_options();
}

inline options_data extract_options(std::shared_ptr<context> ctx,
                                    const std::string &model_file_path,
                                    const std::string &options_file_path)
{
    auto model = make_model(ctx, model_file_path);
    auto options = make_options(ctx, model, options_file_path);

    options_data ret;
    ret.simulations = options.simulations;
    ret.places = options.places;
    ret.departments = options.departments;
    ret.years = options.years;
    ret.observed = options.observed;

    const auto rows = options.options.rows();
    const auto columns = options.options.cols();

    ret.options.resize(columns, rows);

    for (long int r = 0, end_r = rows; r != end_r; ++r)
        for (long int c = 0, end_c = columns; c != end_c; ++c)
            ret.options(c, r) = options.options(r, c);

    return ret;
}

} // namespace efyj

#endif
