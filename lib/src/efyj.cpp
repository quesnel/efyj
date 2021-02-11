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

#include "efyj.hpp"
#include "adjustment.hpp"
#include "model.hpp"
#include "options.hpp"
#include "post.hpp"
#include "prediction-thread.hpp"
#include "prediction.hpp"
#include "private.hpp"
#include "solver-stack.hpp"
#include "utils.hpp"

#include <fmt/format.h>

namespace efyj {

Model
make_model(const context& ctx, const std::string& model_file_path)
{
    Model model;

    const auto ifs = input_file(model_file_path.c_str());
    if (!ifs.is_open()) {
        error(ctx, "fail to open `{}'\n", model_file_path.c_str());
        throw file_error(model_file_path);
    }

    model.read(ctx, ifs);

    return model;
}

Options
make_options(const context& ctx,
             Model& model,
             const std::string& options_file_path)
{
    Options options;

    const auto ifs = input_file(options_file_path.c_str());
    if (!ifs.is_open()) {
        error(ctx, "fail to open `{}'", options_file_path.c_str());
        throw file_error(options_file_path);
    }

    options.read(ctx, ifs, model);

    return options;
}

model_data
extract_model(const context& ctx, const std::string& model_file_path)
{
    auto model = make_model(ctx, model_file_path);
    model_data ret;

    ret.number = numeric_cast<int>(model.attributes.size());

    for (size_t i = 0, e = model.attributes.size(); i != e; ++i) {
        auto key = ret.attributes.emplace(model.attributes[i].name,
                                          std::vector<std::string>());

        for (auto& scale : model.attributes[i].scale.scale)
            key.first->second.emplace_back(scale.name);

        if (model.attributes[i].is_basic())
            ret.basic_attributes.emplace_back(static_cast<int>(i));
    }

    return ret;
}

static constexpr status
csv_parser_status_convert(const csv_parser_status::tag s) noexcept
{
    switch (s) {
    case csv_parser_status::tag::file_error:
        return status::csv_parser_file_error;
    case csv_parser_status::tag::column_number_incorrect:
        return status::csv_parser_column_number_incorrect;
    case csv_parser_status::tag::scale_value_unknown:
        return status::csv_parser_scale_value_unknown;
    case csv_parser_status::tag::column_conversion_failure:
        return status::csv_parser_column_conversion_failure;
    case csv_parser_status::tag::basic_attribute_unknown:
        return status::csv_parser_basic_attribute_unknown;
    }

    std::abort();
}

static constexpr status
dexi_parser_status_convert(const dexi_parser_status::tag s) noexcept
{
    switch (s) {
    case dexi_parser_status::tag::done:
        return status::success;
    case dexi_parser_status::tag::scale_definition_error:
        return status::dexi_parser_scale_definition_error;
    case dexi_parser_status::tag::scale_not_found:
        return status::dexi_parser_scale_not_found;
    case dexi_parser_status::tag::scale_too_big:
        return status::dexi_parser_scale_too_big;
    case dexi_parser_status::tag::file_format_error:
        return status::dexi_parser_file_format_error;
    case dexi_parser_status::tag::not_enough_memory:
        return status::dexi_parser_not_enough_memory;
    case dexi_parser_status::tag::element_unknown:
        return status::dexi_parser_element_unknown;
    case dexi_parser_status::tag::option_conversion_error:
        return status::dexi_parser_option_conversion_error;
    }

    std::abort();
}

status
static_information(const context& ctx,
                   const std::string& model_file_path,
                   information_results& ret) noexcept
{
    try {
        const auto model = make_model(ctx, model_file_path);

        ret.basic_attribute_names.clear();
        ret.basic_attribute_scale_value_numbers.clear();

        for (const auto& att : model.attributes) {
            if (att.is_basic()) {
                ret.basic_attribute_names.emplace_back(att.name);
                ret.basic_attribute_scale_value_numbers.emplace_back(
                  att.scale_size());
            }
        }

        return status::success;
    } catch (const numeric_cast_error& /*e*/) {
        return status::numeric_cast_error;
    } catch (const internal_error& /*e*/) {
        return status::internal_error;
    } catch (const file_error& /*e*/) {
        return status::file_error;
    } catch (const solver_error& /*e*/) {
        return status::solver_error;
    } catch (const dexi_parser_status& e) {
        return dexi_parser_status_convert(e.m_tag);
    } catch (...) {
        return status::unknown_error;
    }
}

static evaluation_results
evaluate(const context& ctx, Model& model, Options& options)
{
    solver_stack solver(model);
    const auto max_opt = options.simulations.size();

    evaluation_results ret;
    ret.options.resize(options.options.cols(), max_opt);
    // TODO ret.attributes.resize(COL, max_opt);
    ret.simulations.resize(max_opt, 0);
    ret.observations.resize(max_opt, 0);
    ret.confusion.resize(
      model.attributes[0].scale.size(), model.attributes[0].scale.size(), 0);

    fmt::print("situation;observation;simulation\n");
    for (size_t opt = 0; opt != max_opt; ++opt) {
        ret.observations[opt] = options.observed[opt];
        ret.simulations[opt] = solver.solve(options.options.row(opt));
        ret.confusion(ret.observations[opt], ret.simulations[opt])++;

        fmt::print(
          "{};{};{}\n", opt, ret.observations[opt], ret.simulations[opt]);
    }

    weighted_kappa_calculator kappa_c(model.attributes[0].scale.size());
    ret.squared_weighted_kappa =
      kappa_c.squared(ret.observations, ret.simulations);
    fmt::print("kappa squared: {}\n", ret.squared_weighted_kappa);

    ret.linear_weighted_kappa =
      kappa_c.linear(ret.observations, ret.simulations);
    fmt::print("kappa linear: {}\n", ret.linear_weighted_kappa);

    for (size_t r = 0, end_r = options.options.rows(); r != end_r; ++r)
        for (size_t c = 0, end_c = options.options.cols(); c != end_c; ++c)
            ret.options(c, r) = options.options(r, c);

    return ret;
}

static bool
is_valid_input_size(const size_t simulation,
                    const size_t places,
                    const size_t departments,
                    const size_t year,
                    const size_t observed) noexcept
{
    return simulation == places && simulation == departments &&
           simulation == year && simulation == observed;
}

static status
make_options(const context& ctx,
             const Model& model,
             const std::vector<std::string>& simulations,
             const std::vector<std::string>& places,
             const std::vector<int>& departments,
             const std::vector<int>& years,
             const std::vector<int>& observed,
             const std::vector<int>& scale_values,
             Options& opt)
{
    try {
        const auto rows = simulations.size();

        if (!is_valid_input_size(rows,
                                 places.size(),
                                 departments.size(),
                                 years.size(),
                                 observed.size()))
            return status::unconsistent_input_vector;

        opt.clear();
        opt.simulations = simulations;
        opt.places = places;
        opt.departments = departments;
        opt.years = years;
        opt.observed = observed;

        const auto atts = model.get_basic_attribute();
        const auto cols = atts.size();

        if (cols * rows != scale_values.size())
            return status::option_input_inconsistent;

        opt.options.init(atts.size(), simulations.size());
        for (size_t i = 0, elem = 0; i != rows; ++i) {
            for (size_t j = 0; j != cols; ++i, ++elem) {
                const auto limit = atts[j]->scale_size();

                if (scale_values[elem] < 0u || scale_values[elem] > limit)
                    return {};

                opt.options(i, j) = scale_values[elem];
            }
        }

        opt.init_dataset();
        opt.check();

        return status::success;
    } catch (const std::bad_alloc& /*e*/) {
        return status::not_enough_memory;
    } catch (...) {
        return status::unknown_error;
    }
}

status
static_evaluate(const context& ctx,
                const std::string& model_file_path,
                const std::vector<std::string>& simulations,
                const std::vector<std::string>& places,
                const std::vector<int>& departments,
                const std::vector<int>& years,
                const std::vector<int>& observed,
                const std::vector<int>& scale_values,
                evaluation_results& out) noexcept
{
    try {
        auto model = make_model(ctx, model_file_path);
        Options options;
        if (const auto ret = make_options(ctx,
                                          model,
                                          simulations,
                                          places,
                                          departments,
                                          years,
                                          observed,
                                          scale_values,
                                          options);
            ret != status::success)
            return ret;

        out = evaluate(ctx, model, options);

        return status::success;
    } catch (const numeric_cast_error& /*e*/) {
        return status::numeric_cast_error;
    } catch (const internal_error& /*e*/) {
        return status::internal_error;
    } catch (const file_error& /*e*/) {
        return status::file_error;
    } catch (const solver_error& /*e*/) {
        return status::solver_error;
    } catch (const dexi_parser_status& e) {
        return dexi_parser_status_convert(e.m_tag);
    } catch (...) {
        return status::unknown_error;
    }
}

status
static_evaluate(const context& ctx,
                const std::string& model_file_path,
                const std::string& options_file_path,
                evaluation_results& ret) noexcept
{
    try {
        auto model = make_model(ctx, model_file_path);
        auto options = make_options(ctx, model, options_file_path);

        ret = evaluate(ctx, model, options);
        return status::success;
    } catch (const numeric_cast_error& /*e*/) {
        return status::numeric_cast_error;
    } catch (const internal_error& /*e*/) {
        return status::internal_error;
    } catch (const file_error& /*e*/) {
        return status::file_error;
    } catch (const solver_error& /*e*/) {
        return status::solver_error;
    } catch (const dexi_parser_status& e) {
        return dexi_parser_status_convert(e.m_tag);
    } catch (...) {
        return status::unknown_error;
    }
}

evaluation_results
evaluate(const context& ctx,
         const std::string& model_file_path,
         const std::string& options_file_path)
{
    auto model = make_model(ctx, model_file_path);
    auto options = make_options(ctx, model, options_file_path);

    return evaluate(ctx, model, options);
}

evaluation_results
evaluate(const context& ctx,
         const std::string& model_file_path,
         const options_data& opts)
{
    auto model = make_model(ctx, model_file_path);
    Options options;

    options.set(opts);

    return evaluate(ctx, model, options);
}

std::vector<result>
adjustment(const context& ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           bool reduce,
           int limit,
           unsigned int /*thread*/)
{
    auto model = make_model(ctx, model_file_path);
    auto options = make_options(ctx, model, options_file_path);

    efyj::adjustment_evaluator adj(ctx, model, options);
    return adj.run(limit, 0.0, reduce);
}

std::vector<result>
prediction(const context& ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           bool reduce,
           int limit,
           unsigned int thread)
{
    fmt::print("Start prediction: reduce {} limit {} thread {}\n",
               reduce,
               limit,
               thread);

    auto model = make_model(ctx, model_file_path);
    auto options = make_options(ctx, model, options_file_path);

    if (thread <= 1) {
        efyj::prediction_evaluator pre(ctx, model, options);
        return pre.run(limit, 0.0, reduce);
    } else {
        efyj::prediction_thread_evaluator pre(ctx, model, options);
        return pre.run(limit, 0.0, reduce, thread);
    }
}

status
static_adjustment(const context& ctx,
                  const std::string& model_file_path,
                  const std::vector<std::string>& simulations,
                  const std::vector<std::string>& places,
                  const std::vector<int> departments,
                  const std::vector<int> years,
                  const std::vector<int> observed,
                  const std::vector<int>& scale_values,
                  result_callback callback,
                  bool reduce,
                  int limit,
                  unsigned int thread) noexcept
{
    try {
        if (!is_valid_input_size(simulations.size(),
                                 places.size(),
                                 departments.size(),
                                 years.size(),
                                 observed.size()))
            return status::unconsistent_input_vector;

        auto model = make_model(ctx, model_file_path);
        Options options(
          simulations, places, departments, years, observed, scale_values);

        efyj::adjustment_evaluator adj(ctx, model, options);
        const auto ret = adj.run(limit, 0.0, reduce);
        (void)callback(ret);
        return status::success;
    } catch (const numeric_cast_error& /*e*/) {
        return status::numeric_cast_error;
    } catch (const internal_error& /*e*/) {
        return status::internal_error;
    } catch (const file_error& /*e*/) {
        return status::file_error;
    } catch (const solver_error& /*e*/) {
        return status::solver_error;
    } catch (const dexi_parser_status& e) {
        return dexi_parser_status_convert(e.m_tag);
    } catch (...) {
        return status::unknown_error;
    }

    return status::success;
}

status
static_prediction(const context& ctx,
                  const std::string& model_file_path,
                  const std::vector<std::string>& simulations,
                  const std::vector<std::string>& places,
                  const std::vector<int> departments,
                  const std::vector<int> years,
                  const std::vector<int> observed,
                  const std::vector<int>& scale_values,
                  result_callback callback,
                  bool reduce,
                  int limit,
                  unsigned int thread) noexcept
{
    try {
        if (!is_valid_input_size(simulations.size(),
                                 places.size(),
                                 departments.size(),
                                 years.size(),
                                 observed.size()))
            return status::unconsistent_input_vector;

        auto model = make_model(ctx, model_file_path);

        Options options(
          simulations, places, departments, years, observed, scale_values);

        if (thread <= 1) {
            efyj::prediction_evaluator pre(ctx, model, options);
            auto ret = pre.run(limit, 0.0, reduce);
            (void)callback(ret);
            return status::success;
        } else {
            efyj::prediction_thread_evaluator pre(ctx, model, options);
            auto ret = pre.run(limit, 0.0, reduce, thread);
            (void)callback(ret);
            return status::success;
        }
    } catch (const numeric_cast_error& /*e*/) {
        return status::numeric_cast_error;
    } catch (const internal_error& /*e*/) {
        return status::internal_error;
    } catch (const file_error& /*e*/) {
        return status::file_error;
    } catch (const solver_error& /*e*/) {
        return status::solver_error;
    } catch (const dexi_parser_status& e) {
        return dexi_parser_status_convert(e.m_tag);
    } catch (...) {
        return status::unknown_error;
    }

    return status::success;
}

status
static_merge_options(const context& ctx,
                     const std::string& model_file_path,
                     const std::string& options_file_path,
                     const std::string& output_file_path) noexcept
{
    try {
        debug(ctx,
              "[efyj] make DEXi file {} from the DEXi {}/ csv {}",
              output_file_path,
              model_file_path,
              options_file_path);

        if (model_file_path == output_file_path) {
            if (ctx.file_cb)
                ctx.file_cb(model_file_path);

            return status::merge_option_same_inputoutput;
        }

        auto model = make_model(ctx, model_file_path);
        auto options =
          extract_options(ctx, model_file_path, options_file_path);

        const auto ofs = output_file(output_file_path.c_str());
        if (!ofs.is_open()) {
            if (ctx.file_cb)
                ctx.file_cb(output_file_path);

            return status::merge_option_fail_open_file;
        }

        model.set_options(options);
        model.write(ctx, ofs);
        return status::success;
    } catch (const numeric_cast_error& /*e*/) {
        return status::numeric_cast_error;
    } catch (const internal_error& /*e*/) {
        return status::internal_error;
    } catch (const file_error& /*e*/) {
        return status::file_error;
    } catch (const solver_error& /*e*/) {
        return status::solver_error;
    } catch (const dexi_parser_status& e) {
        return dexi_parser_status_convert(e.m_tag);
    } catch (...) {
        return status::unknown_error;
    }

    return status::success;
}

static void
reorder_basic_attribute(const Model& model,
                        size_t att,
                        std::vector<size_t>& out)
{
    if (model.attributes[att].is_basic())
        out.push_back(att);
    else
        for (auto child : model.attributes[att].children)
            reorder_basic_attribute(model, child, out);
}

EFYJ_API status
static_merge_options(const context& ctx,
                     const std::string& model_file_path,
                     const std::string& output_file_path,
                     const std::vector<std::string>& simulations,
                     const std::vector<std::string>& places,
                     const std::vector<int> departments,
                     const std::vector<int> years,
                     const std::vector<int> observed,
                     const std::vector<int>& scale_values) noexcept
{
    try {
        notice(ctx,
               "[efyj] make DEXi file {} from the DEXi {} and input vectors",
               output_file_path,
               model_file_path);

        if (model_file_path == output_file_path) {
            if (ctx.file_cb)
                ctx.file_cb(model_file_path);

            return status::merge_option_same_inputoutput;
        }

        auto model = make_model(ctx, model_file_path);
        Options options;
        auto ret = make_options(ctx,
                                model,
                                simulations,
                                places,
                                departments,
                                years,
                                observed,
                                scale_values,
                                options);

        if (ret != status::success)
            return ret;

        const auto ofs = output_file(output_file_path.c_str());
        if (!ofs.is_open()) {
            if (ctx.file_cb)
                ctx.file_cb(output_file_path);

            return status::merge_option_fail_open_file;
        }

        std::vector<size_t> ordered_att;
        reorder_basic_attribute(model, 0, ordered_att);

        for (size_t i = 0, e = model.attributes.size(); i != e; ++i)
            model.attributes[i].options.clear();

        for (size_t i = 0, end_i = ordered_att.size(); i != end_i; ++i) {
            const auto att = ordered_att[i];

            for (size_t opt = 0, end_opt = options.options.rows();
                 opt != end_opt;
                 ++opt)
                model.attributes[att].options.emplace_back(
                  options.options(opt, i));
        }

        model.options = options.simulations;

        model.write(ctx, ofs);
        return status::success;
    } catch (const numeric_cast_error& /*e*/) {
        return status::numeric_cast_error;
    } catch (const internal_error& /*e*/) {
        return status::internal_error;
    } catch (const file_error& /*e*/) {
        return status::file_error;
    } catch (const solver_error& /*e*/) {
        return status::solver_error;
    } catch (const dexi_parser_status& e) {
        return dexi_parser_status_convert(e.m_tag);
    } catch (...) {
        return status::unknown_error;
    }

    return status::success;
}

options_data
extract_options(const context& ctx, const std::string& model_file_path)
{
    auto model = make_model(ctx, model_file_path);

    return model.write_options();
}

options_data
extract_options(const context& ctx,
                const std::string& model_file_path,
                const std::string& options_file_path)
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

    ret.options.resize(rows, columns);

    for (size_t r = 0, end_r = rows; r != end_r; ++r)
        for (size_t c = 0, end_c = columns; c != end_c; ++c)
            ret.options(r, c) = options.options(r, c);

    return ret;
}

void
extract_options_to_file(const context& ctx,
                        const std::string& model_file_path,
                        const std::string& output_file_path)
{
    auto model = make_model(ctx, model_file_path);

    const auto file = output_file(output_file_path.c_str());
    if (file.is_open())
        model.write_options(file);
    else
        fmt::print("Fail to open csv file `{}'\n", output_file_path.c_str());
}

void
merge_options(const context& ctx,
              const std::string& model_file_path,
              const std::string& options_file_path,
              const std::string& output_file_path)
{
    debug(ctx,
          "[efyj] make DEXi file {} from the DEXi {}/ csv {}",
          output_file_path.c_str(),
          model_file_path.c_str(),
          options_file_path.c_str());

    if (model_file_path == output_file_path) {
        warning(ctx, "Can not merge into the same file\n");
        return;
    }

    auto model = make_model(ctx, model_file_path);
    auto options = extract_options(ctx, model_file_path, options_file_path);

    const auto ofs = output_file(output_file_path.c_str());
    if (!ofs.is_open()) {
        warning(ctx, "Fail to open DEXi file {}.\n", output_file_path.c_str());
        return;
    }

    model.set_options(options);
    model.write(ctx, ofs);
}

} // namespace efyj
