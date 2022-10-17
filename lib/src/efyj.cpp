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

status
make_model(context& ctx, const std::string& model_file_path, Model& model)
{
    const auto ifs = input_file(model_file_path.c_str());
    if (!ifs.is_open()) {
        ctx.data_1 = model_file_path;
        return ctx.status = status::file_error;
    }

    if (auto ret = model.read(ctx, ifs); is_bad(ret))
        return ret;

    return status::success;
}

status
make_options(context& ctx,
             Model& model,
             const std::string& options_file_path,
             Options& options)
{
    const auto ifs = input_file(options_file_path.c_str());
    if (!ifs.is_open()) {
        ctx.data_1 = options_file_path;
        return ctx.status = status::file_error;
    }

    if (auto ret = options.read(ctx, ifs, model); is_bad(ret)) {
        ctx.data_1 = options_file_path;
        ctx.line = static_cast<int>(options.error_at_line);
        ctx.column = static_cast<int>(options.error_at_column);
        return ctx.status = ret;
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

static status
get_options_model(const Model& mdl, const output_file& os)
{
    std::vector<size_t> ordered_att;
    reorder_basic_attribute(mdl, (size_t)0, ordered_att);
    os.print("simulation;place;department;year;");

    for (auto child : ordered_att)
        os.print("{};", mdl.attributes[child].name);

    os.print("{}\n", mdl.attributes[0].name);

    for (size_t opt{ 0 }; opt != mdl.options.size(); ++opt) {
        os.print("{};-;0;0;", mdl.options[opt]);

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
get_options_model(const Model& mdl, Options& opts)
{
    std::vector<size_t> ordered_att;
    reorder_basic_attribute(mdl, (size_t)0, ordered_att);

    opts.options.init(mdl.options.size(), ordered_att.size());

    for (size_t opt = 0; opt != mdl.options.size(); ++opt) {
        opts.simulations.emplace_back(mdl.options[opt]);
        opts.places.emplace_back("-");
        opts.departments.emplace_back(0);
        opts.years.emplace_back(0);

        for (size_t c = 0, ec = ordered_att.size(); c != ec; ++c) {
            const auto child = ordered_att[c];
            opts.options(opt, c) = mdl.attributes[child].options[opt];
        }

        opts.observed.emplace_back(mdl.attributes[0].options[opt]);
    }

    return status::success;
}

static status
set_options_model(Model& mdl, const Options& opts)
{
    const auto rows = opts.simulations.size();

    std::vector<size_t> ordered_att;
    reorder_basic_attribute(mdl, 0, ordered_att);

    for (size_t i = 0, e = mdl.attributes.size(); i != e; ++i) {
        mdl.attributes[i].options.resize(rows);

        std::fill_n(mdl.attributes[i].options.data(), rows, 0);
    }

    for (size_t i = 0, end_i = ordered_att.size(); i != end_i; ++i) {
        const auto att = ordered_att[i];

        for (size_t row = 0; row != rows; ++row)
            mdl.attributes[att].options[row] = opts.options(rows, i);
    }

    mdl.options = opts.simulations;

    return status::success;
}

status
information(context& ctx,
            const std::string& model_file_path,
            information_results& out) noexcept
{
    try {
        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        out.basic_attribute_names.clear();
        out.basic_attribute_scale_value_numbers.clear();

        for (const auto& att : model.attributes) {
            if (att.is_basic()) {
                out.basic_attribute_names.emplace_back(att.name);
                out.basic_attribute_scale_value_numbers.emplace_back(
                  att.scale_size());
            }
        }

        return status::success;
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
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
make_options(context& ctx, const Model& model, const data& d, Options& opt)
{
    try {
        const auto option_number = d.simulations.size();

        if (!is_valid_input_size(option_number,
                                 d.places.size(),
                                 d.departments.size(),
                                 d.years.size(),
                                 d.observed.size()))
            return status::unconsistent_input_vector;

        opt.clear();
        opt.simulations = d.simulations;
        opt.places = d.places;
        opt.departments = d.departments;
        opt.years = d.years;
        opt.observed = d.observed;

        const auto attribute_number = model.get_basic_attribute().size();

        std::vector<size_t> ordered_att;
        reorder_basic_attribute(model, 0, ordered_att);

        if (attribute_number * option_number != d.scale_values.size())
            return status::option_input_inconsistent;

        opt.options.init(option_number, attribute_number);
        size_t optid = 0;
        size_t attid = 0;
        for (auto& elem : d.scale_values) {
            const auto attribute = ordered_att[attid];
            const auto limit = model.attributes[attribute].scale_size();

            if (elem > limit) {
                error(ctx,
                      "bad scale value: {} with a limit of {} for "
                      "attribute {}\n",
                      elem,
                      limit,
                      model.attributes[attribute].name);
                return ctx.status = status::scale_value_inconsistent;
            }

            opt.options(optid, attid) = elem;
            ++attid;

            if (attid == attribute_number) {
                ++optid;
                attid = 0;
            }
        }

        opt.init_dataset();

        if (!opt.check())
            return ctx.status = status::option_input_inconsistent;

        return status::success;
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
    }
}

static void
evaluate([[maybe_unused]] context& ctx,
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
evaluate(context& ctx,
         const std::string& model_file_path,
         const data& d,
         evaluation_results& out) noexcept
{
    try {
        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        model.clear_options();

        Options options;
        if (auto ret = make_options(ctx, model, d, options); is_bad(ret))
            return ret;

        out.clear();
        evaluate(ctx, model, options, out);
        return ctx.status = status::success;
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
    }
}

status
evaluate(context& ctx,
         const std::string& model_file_path,
         const std::string& options_file_path,
         evaluation_results& out) noexcept
{
    try {
        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        model.clear_options();

        Options options;
        if (auto ret = make_options(ctx, model, options_file_path, options);
            is_bad(ret))
            return ret;

        out.clear();
        evaluate(ctx, model, options, out);
        return ctx.status = status::success;
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
    }
}

status
adjustment(context& ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           result_callback callback,
           void* user_data_callback,
           check_user_interrupt_callback interrupt,
           void* user_data_interrupt,
           bool reduce,
           int limit,
           [[maybe_unused]] unsigned int thread) noexcept
{
    try {
        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        model.clear_options();

        Options options;
        if (auto ret = make_options(ctx, model, options_file_path, options);
            is_bad(ret))
            return ret;

        efyj::adjustment_evaluator adj(ctx, model, options);
        return interrupt
                 ? adj.run(interrupt,
                           user_data_interrupt,
                           callback,
                           user_data_callback,
                           limit,
                           0.0,
                           reduce,
                           "")
                 : adj.run(
                     callback, user_data_callback, limit, 0.0, reduce, "");
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
    }

    return status::success;
}

status
adjustment(context& ctx,
           const std::string& model_file_path,
           const data& d,
           result_callback callback,
           void* user_data_callback,
           check_user_interrupt_callback interrupt,
           void* user_data_interrupt,
           bool reduce,
           int limit,
           [[maybe_unused]] unsigned int thread) noexcept
{
    try {
        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        model.clear_options();

        Options options;
        if (auto ret = make_options(ctx, model, d, options); is_bad(ret))
            return ret;

        efyj::adjustment_evaluator adj(ctx, model, options);
        return interrupt
                 ? adj.run(interrupt,
                           user_data_interrupt,
                           callback,
                           user_data_callback,
                           limit,
                           0.0,
                           reduce,
                           "")
                 : adj.run(
                     callback, user_data_callback, limit, 0.0, reduce, "");
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
    }
}

status
prediction(context& ctx,
           const std::string& model_file_path,
           const std::string& options_file_path,
           result_callback callback,
           void* user_data_callback,
           check_user_interrupt_callback /*interrupt*/,
           void* /*user_data_interrupt*/,
           bool reduce,
           int limit,
           unsigned int thread) noexcept
{
    try {
        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        model.clear_options();

        Options options;
        if (auto ret = make_options(ctx, model, options_file_path, options);
            is_bad(ret))
            return ret;

        if (thread <= 1) {
            efyj::prediction_evaluator pre(ctx, model, options);
            pre.run(callback, user_data_callback, limit, 0.0, reduce, "");
            return ctx.status = status::success;
        } else {
            efyj::prediction_thread_evaluator pre(ctx, model, options);
            pre.run(
              callback, user_data_callback, limit, 0.0, reduce, thread, "");
            return ctx.status = status::success;
        }
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
    }
}

status
prediction(context& ctx,
           const std::string& model_file_path,
           const data& d,
           result_callback callback,
           void* user_data_callback,
           check_user_interrupt_callback /*interrupt*/,
           void* /*user_data_interrupt*/,
           bool reduce,
           int limit,
           unsigned int thread) noexcept
{
    try {
        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        model.clear_options();

        Options options;
        if (auto ret = make_options(ctx, model, d, options); is_bad(ret))
            return ret;

        if (!options.have_subdataset())
            return status::option_input_inconsistent;

        if (thread <= 1) {
            efyj::prediction_evaluator pre(ctx, model, options);
            pre.run(callback, user_data_callback, limit, 0.0, reduce, "");
            return ctx.status = status::success;
        } else {
            efyj::prediction_thread_evaluator pre(ctx, model, options);
            pre.run(
              callback, user_data_callback, limit, 0.0, reduce, thread, "");
            return ctx.status = status::success;
        }
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
    }
}

status
extract_options_to_file(context& ctx,
                        const std::string& model_file_path,
                        const std::string& output_file_path) noexcept
{
    try {
        debug(ctx,
              "[efyj] extract options from DEXi file {} to csv file {}",
              model_file_path,
              output_file_path);

        if (model_file_path == output_file_path) {
            ctx.data_1 = model_file_path;
            return ctx.status = status::extract_option_same_input_files;
        }

        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        const auto ofs = output_file(output_file_path.c_str());
        if (!ofs.is_open()) {
            ctx.data_1 = output_file_path;
            return ctx.status = status::merge_option_fail_open_file;
        }

        return get_options_model(model, ofs);
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
    }
}

status
extract_options(context& ctx,
                const std::string& model_file_path,
                data& out) noexcept
{
    try {
        debug(
          ctx, "[efyj] extract options from DEXi file {}", model_file_path);

        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        Options opts;
        if (auto ret = get_options_model(model, opts); is_bad(ret))
            return ret;

        out.simulations = opts.simulations;
        out.places = opts.places;
        out.departments = opts.departments;
        out.years = opts.years;
        out.observed = opts.observed;

        const auto rows = out.simulations.size();
        const auto cols = model.get_basic_attribute().size();

        out.scale_values.clear();
        out.scale_values.reserve(rows * cols);
        for (size_t i = 0; i != rows; ++i)
            for (size_t j = 0; j != cols; ++j)
                out.scale_values.emplace_back(opts.options(i, j));

        return ctx.status = status::success;
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
    }
}

status
extract_options(context& ctx,
                const std::string& model_file_path,
                const std::string& options_file_path,
                data& out) noexcept
{
    try {
        debug(
          ctx, "[efyj] extract options from DEXi file {}", model_file_path);

        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        Options options;
        if (auto ret = make_options(ctx, model, options_file_path, options);
            is_bad(ret))
            return ret;

        out.simulations = std::move(options.simulations);
        out.places = std::move(options.places);
        out.departments = std::move(options.departments);
        out.years = std::move(options.years);
        out.observed = std::move(options.observed);

        const auto rows = out.simulations.size();
        const auto cols = model.get_basic_attribute().size();

        out.scale_values.clear();
        out.scale_values.reserve(rows * cols);
        for (size_t i = 0; i != rows; ++i)
            for (size_t j = 0; j != cols; ++j)
                out.scale_values.emplace_back(options.options(i, j));

        return ctx.status = status::success;
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
    }
}

status
merge_options_to_file(context& ctx,
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
            ctx.data_1 = model_file_path;
            return ctx.status = status::merge_option_same_inputoutput;
        }

        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        Options options;
        if (auto ret = make_options(ctx, model, options_file_path, options);
            is_bad(ret))
            return ret;

        const auto ofs = output_file(output_file_path.c_str());
        if (!ofs.is_open()) {
            ctx.data_1 = output_file_path;
            return ctx.status = status::merge_option_fail_open_file;
        }

        set_options_model(model, options);
        model.write(ctx, ofs);
        return ctx.status = status::success;
    } catch (const std::bad_alloc& e) {
        error(ctx, "c++ bad alloc: {}\n", e.what());
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        error(ctx, "c++ exception: {}\n", e.what());
        return ctx.status = status::unknown_error;
    } catch (...) {
        error(ctx, "c++ unknown exception\n");
        return ctx.status = status::unknown_error;
    }
}

status
merge_options(context& ctx,
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
            ctx.data_1 = model_file_path;
            return ctx.status = status::merge_option_same_inputoutput;
        }

        Model model;
        if (auto ret = make_model(ctx, model_file_path, model); is_bad(ret))
            return ret;

        Options options;
        if (auto ret = make_options(ctx, model, d, options); is_bad(ret))
            return ret;

        const auto ofs = output_file(output_file_path.c_str());
        if (!ofs.is_open()) {
            ctx.data_1 = output_file_path;
            return ctx.status = status::merge_option_fail_open_file;
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
        return ctx.status = status::success;
    } catch (const std::bad_alloc& /*e*/) {
        return ctx.status = status::not_enough_memory;
    } catch (const std::exception& e) {
        ctx.data_1 = e.what();
        return ctx.status = status::unknown_error;
    } catch (...) {
        return ctx.status = status::unknown_error;
    }
}

} // namespace efyj
