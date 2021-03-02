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

static status
get_options_model(const context& ctx, const Model& mdl, const output_file& os)
{
    std::vector<size_t> ordered_att;
    reorder_basic_attribute(mdl, (size_t)0, ordered_att);
    os.print("simulation;place;department;year;");

    for (auto child : ordered_att)
        os.print("{};", mdl.attributes[child].name);

    os.print("{}\n", mdl.attributes[0].name);

    for (size_t opt{ 0 }; opt != mdl.options.size(); ++opt) {
        os.print("{}../;-;0;0;", mdl.options[opt]);

        for (auto child : ordered_att)
            os.print("{};",
                     mdl.attributes[child]
                       .scale.scale[mdl.attributes[child].options[opt]]
                       .name);

        os.print(
          "{}\n",
          mdl.attributes[0].scale.scale[mdl.attributes[0].options[opt]].name);
    }

    return status::success;
}

static status
get_options_model(const context& ctx, const Model& mdl, Options& opts)
{
    std::vector<size_t> ordered_att;
    reorder_basic_attribute(mdl, (size_t)0, ordered_att);

    opts.options.init(ordered_att.size(), mdl.options.size());

    for (size_t opt = 0; opt != mdl.options.size(); ++opt) {
        opts.simulations.emplace_back(mdl.options[opt] + "../");
        opts.places.emplace_back("-");
        opts.departments.emplace_back(0);
        opts.years.emplace_back(0);

        for (size_t c = 0, ec = ordered_att.size(); c != ec; ++c)
            opts.options(opt, c) = mdl.attributes[ordered_att[c]].options[opt];

        opts.observed.emplace_back(mdl.attributes[0].options[opt]);
    }

    return status::success;
}

static status
set_options_model(const context& ctx, Model& mdl, const Options& opts)
{
    std::vector<size_t> ordered_att;
    reorder_basic_attribute(mdl, 0, ordered_att);

    for (size_t i = 0, e = mdl.attributes.size(); i != e; ++i)
        mdl.attributes[i].options.clear();

    for (size_t i = 0, end_i = ordered_att.size(); i != end_i; ++i) {
        const auto att = ordered_att[i];

        for (size_t opt = 0, end_opt = opts.options.rows(); opt != end_opt;
             ++opt)
            mdl.attributes[att].options.emplace_back(opts.options(opt, i));
    }

    mdl.options = opts.simulations;

    return status::success;
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
information(const context& ctx,
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

static void
evaluate(const context& ctx,
         Model& model,
         Options& options,
         evaluation_results& out)
{
    solver_stack solver(model);
    const auto max_opt = options.simulations.size();
    out.options.resize(options.options.cols(), max_opt);
    out.simulations.resize(max_opt, 0);
    out.observations.resize(max_opt, 0);
    out.confusion.resize(
      model.attributes[0].scale.size(), model.attributes[0].scale.size(), 0);

    for (size_t opt = 0; opt != max_opt; ++opt) {
        out.observations[opt] = options.observed[opt];
        out.simulations[opt] = solver.solve(options.options.row(opt));
        out.confusion(out.observations[opt], out.simulations[opt])++;
    }

    weighted_kappa_calculator kappa_c(model.attributes[0].scale.size());
    out.squared_weighted_kappa =
      kappa_c.squared(out.observations, out.simulations);
    out.linear_weighted_kappa =
      kappa_c.linear(out.observations, out.simulations);

    for (size_t r = 0, end_r = options.options.rows(); r != end_r; ++r)
        for (size_t c = 0, end_c = options.options.cols(); c != end_c; ++c)
            out.options(c, r) = options.options(r, c);
}

status
evaluate(const context& ctx,
         const std::string& model_file_path,
         const data& d,
         evaluation_results& out) noexcept
{
    try {
        auto model = make_model(ctx, model_file_path);
        Options options(d);

        evaluate(ctx, model, options, out);

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
evaluate(const context& ctx,
         const std::string& model_file_path,
         const std::string& options_file_path,
         evaluation_results& ret) noexcept
{
    try {
        auto model = make_model(ctx, model_file_path);
        auto options = make_options(ctx, model, options_file_path);

        evaluate(ctx, model, options, ret);
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
adjustment(const context& ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           const result_callback& callback,
           bool reduce,
           int limit,
           unsigned int thread) noexcept
{
    try {
        auto model = make_model(ctx, model_file_path);
        auto options = make_options(ctx, model, options_file_path);
        efyj::adjustment_evaluator adj(ctx, model, options);
        adj.run(callback, limit, 0.0, reduce);
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
adjustment(const context& ctx,
           const std::string& model_file_path,
           const data& d,
           const result_callback& callback,
           bool reduce,
           int limit,
           unsigned int thread) noexcept
{
    try {
        auto model = make_model(ctx, model_file_path);
        Options options(d);
        efyj::adjustment_evaluator adj(ctx, model, options);
        adj.run(callback, limit, 0.0, reduce);
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
prediction(const context& ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           const result_callback& callback,
           bool reduce,
           int limit,
           unsigned int thread) noexcept
{
    try {
        auto model = make_model(ctx, model_file_path);
        auto options = make_options(ctx, model, options_file_path);

        if (thread <= 1) {
            efyj::prediction_evaluator pre(ctx, model, options);
            pre.run(callback, limit, 0.0, reduce);
            return status::success;
        } else {
            efyj::prediction_thread_evaluator pre(ctx, model, options);
            pre.run(callback, limit, 0.0, reduce, thread);
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
prediction(const context& ctx,
           const std::string& model_file_path,
           const data& d,
           const result_callback& callback,
           bool reduce,
           int limit,
           unsigned int thread) noexcept
{
    try {
        auto model = make_model(ctx, model_file_path);
        Options options(d);

        if (thread <= 1) {
            efyj::prediction_evaluator pre(ctx, model, options);
            pre.run(callback, limit, 0.0, reduce);
            return status::success;
        } else {
            efyj::prediction_thread_evaluator pre(ctx, model, options);
            pre.run(callback, limit, 0.0, reduce, thread);
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
extract_options_to_file(const context& ctx,
                        const std::string& model_file_path,
                        const std::string& output_file_path) noexcept
{
    try {
        debug(ctx,
              "[efyj] extract options from DEXi file {} to csv file {}",
              model_file_path,
              output_file_path);

        if (model_file_path == output_file_path) {
            if (ctx.file_cb)
                ctx.file_cb(model_file_path);

            return status::extract_option_same_input_files;
        }

        auto model = make_model(ctx, model_file_path);

        const auto ofs = output_file(output_file_path.c_str());
        if (!ofs.is_open()) {
            if (ctx.file_cb)
                ctx.file_cb(output_file_path);

            return status::merge_option_fail_open_file;
        }

        return get_options_model(ctx, model, ofs);
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
extract_options(const context& ctx,
                const std::string& model_file_path,
                data& out) noexcept
{
    try {
        debug(
          ctx, "[efyj] extract options from DEXi file {}", model_file_path);

        auto model = make_model(ctx, model_file_path);
        Options opts;

        if (auto ret = get_options_model(ctx, model, opts); !is_success(ret))
            return ret;

        out.simulations = std::move(opts.simulations);
        out.places = std::move(opts.places);
        out.departments = std::move(opts.departments);
        out.years = std::move(opts.years);
        out.observed = std::move(opts.observed);

        out.scale_values.clear();
        std::copy_n(opts.options.data(),
                    opts.options.size(),
                    std::back_inserter(out.scale_values));

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
extract_options(const context& ctx,
                const std::string& model_file_path,
                const std::string& options_file_path,
                data& out) noexcept
{
    try {
        debug(
          ctx, "[efyj] extract options from DEXi file {}", model_file_path);

        auto model = make_model(ctx, model_file_path);
        auto options = make_options(ctx, model, options_file_path);

        out.simulations = std::move(options.simulations);
        out.places = std::move(options.places);
        out.departments = std::move(options.departments);
        out.years = std::move(options.years);
        out.observed = std::move(options.observed);

        out.scale_values.clear();
        std::copy_n(options.options.data(),
                    options.options.size(),
                    std::back_inserter(out.scale_values));

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
merge_options_to_file(const context& ctx,
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
        auto options = make_options(ctx, model, options_file_path);

        const auto ofs = output_file(output_file_path.c_str());
        if (!ofs.is_open()) {
            if (ctx.file_cb)
                ctx.file_cb(output_file_path);

            return status::merge_option_fail_open_file;
        }

        set_options_model(ctx, model, options);
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

status
merge_options(const context& ctx,
              const std::string& model_file_path,
              const std::string& output_file_path,
              const data& d) noexcept
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
        Options options(d);

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

} // namespace efyj
