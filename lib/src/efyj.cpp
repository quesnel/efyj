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

eastl::shared_ptr<context>
make_context(int log_priority)
{
    auto ret = eastl::make_shared<context>();

    ret->log_priority = log_priority < 0
                          ? log_level::emerg
                          : log_priority > 7
                              ? log_level::debug
                              : static_cast<log_level>(log_priority);

    return ret;
}

Model
make_model(eastl::shared_ptr<context> ctx, const eastl::string& model_file_path)
{
    Model model;

    FILE* ifs = fopen(model_file_path.c_str(), "r");
    if (!ifs) {
        error(ctx, "fail to open `{}'\n", model_file_path.c_str());
        throw file_error(model_file_path);
    }

    model.read(ifs);

    fclose(ifs);

    return model;
}

Options
make_options(eastl::shared_ptr<context> ctx,
             Model& model,
             const eastl::string& options_file_path)
{
    Options options;

    auto* ifs = fopen(options_file_path.c_str(), "r");
    if (!ifs) {
        error(ctx, "fail to open `{}'", options_file_path.c_str());
        throw file_error(options_file_path);
    }

    options.read(ctx, ifs, model);

    fclose(ifs);

    return options;
}

model_data
extract_model(eastl::shared_ptr<context> ctx,
              const eastl::string& model_file_path)
{
    auto model = make_model(ctx, model_file_path);
    model_data ret;

    ret.number = numeric_cast<int>(model.attributes.size());

    for (size_t i = 0, e = model.attributes.size(); i != e; ++i) {
        auto key = ret.attributes.emplace(model.attributes[i].name,
                                          eastl::vector<eastl::string>());

        for (auto& scale : model.attributes[i].scale.scale)
            key.first->second.emplace_back(scale.name);

        if (model.attributes[i].is_basic())
            ret.basic_attributes.emplace_back(i);
    }

    return ret;
}

simulation_results
simulate(eastl::shared_ptr<context> ctx,
         const eastl::string& model_file_path,
         const eastl::string& options_file_path)
{
    auto model = make_model(ctx, model_file_path);
    auto options = make_options(ctx, model, options_file_path);

    const size_t max_opt = options.simulations.size();
    simulation_results ret;
    ret.options.resize(options.options.cols(), max_opt);
    // TODO ret.attributes.resize(COL, max_opt);
    ret.simulations.resize(max_opt);

    solver_stack solver(model);

    for (size_t opt = 0; opt != max_opt; ++opt)
        ret.simulations[opt] = solver.solve(options.options.row(opt));

    for (long int r = 0, end_r = options.options.rows(); r != end_r; ++r)
        for (long int c = 0, end_c = options.options.cols(); c != end_c; ++c)
            ret.options(c, r) = options.options(c, r);

    return ret;
}

simulation_results
simulate(eastl::shared_ptr<context> ctx,
         const eastl::string& model_file_path,
         const options_data& opts)
{
    auto model = make_model(ctx, model_file_path);
    Options options;

    options.set(opts);

    const size_t max_opt = options.simulations.size();
    simulation_results ret;
    ret.options.resize(options.options.cols(), max_opt);
    // TODO ret.attributes.resize(COL, max_opt);
    ret.simulations.resize(max_opt);

    solver_stack solver(model);

    for (size_t opt = 0; opt != max_opt; ++opt)
        ret.simulations[opt] = solver.solve(options.options.row(opt));

    for (long int r = 0, end_r = options.options.rows(); r != end_r; ++r)
        for (long int c = 0, end_c = options.options.cols(); c != end_c; ++c)
            ret.options(c, r) = options.options(c, r);

    return ret;
}
evaluation_results
evaluate(eastl::shared_ptr<context> ctx,
         const eastl::string& model_file_path,
         const eastl::string& options_file_path)
{
    auto model = make_model(ctx, model_file_path);
    auto options = make_options(ctx, model, options_file_path);

    solver_stack solver(model);
    const size_t max_opt = options.simulations.size();
    evaluation_results ret;
    ret.options.resize(options.options.cols(), max_opt);
    // TODO ret.attributes.resize(COL, max_opt);
    ret.simulations.resize(max_opt, 0);
    ret.observations.resize(max_opt, 0);
    ret.confusion.resize(
      model.attributes[0].scale.size(), model.attributes[0].scale.size(), 0);

    fmt::print("observation | simulation\n");
    for (size_t opt = 0; opt != max_opt; ++opt) {
        ret.observations[opt] = options.observed[opt];
        ret.simulations[opt] = solver.solve(options.options.row(opt));
        ret.confusion(ret.observations[opt], ret.simulations[opt])++;

        fmt::print(
          "{}: {} / {}\n", opt, ret.observations[opt], ret.simulations[opt]);
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

evaluation_results
evaluate(eastl::shared_ptr<context> ctx,
         const eastl::string& model_file_path,
         const options_data& opts)
{
    auto model = make_model(ctx, model_file_path);
    Options options;

    options.set(opts);

    solver_stack solver(model);
    const size_t max_opt = options.simulations.size();
    evaluation_results ret;
    ret.options.resize(options.options.cols(), max_opt);
    // TODO ret.attributes.resize(COL, max_opt);
    ret.simulations.resize(max_opt);
    ret.observations.resize(max_opt);
    ret.confusion.resize(
      model.attributes[0].scale.size(), model.attributes[0].scale.size(), 0);

    for (size_t opt = 0; opt != max_opt; ++opt) {
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

eastl::vector<result>
adjustment(eastl::shared_ptr<context> ctx,
           const eastl::string& model_file_path,
           const eastl::string& options_file_path,
           bool reduce,
           int limit,
           unsigned int thread)
{
    auto model = make_model(ctx, model_file_path);
    auto options = make_options(ctx, model, options_file_path);

    efyj::adjustment_evaluator adj(ctx, model, options);
    return adj.run(limit, 0.0, reduce);
}

eastl::vector<result>
prediction(eastl::shared_ptr<context> ctx,
           const eastl::string& model_file_path,
           const eastl::string& options_file_path,
           bool reduce,
           int limit,
           unsigned int thread)
{
    auto model = make_model(ctx, model_file_path);
    auto options = make_options(ctx, model, options_file_path);

    if (thread <= 1) {
        efyj::prediction_evaluator pre(ctx, model, options);
        return pre.run(limit, 0.0, reduce);
    }

    efyj::prediction_thread_evaluator pre(ctx, model, options);
    return pre.run(limit, 0.0, reduce, thread);
}

options_data
extract_options(eastl::shared_ptr<context> ctx,
                const eastl::string& model_file_path)
{
    auto model = make_model(ctx, model_file_path);

    return model.write_options();
}

options_data
extract_options(eastl::shared_ptr<context> ctx,
                const eastl::string& model_file_path,
                const eastl::string& options_file_path)
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

struct c_file
{
    enum class file_mode
    {
        read,
        write
    };

    FILE* file = nullptr;

    c_file(const char* file_path, file_mode mode = file_mode::read)
      : file(fopen(file_path, mode == file_mode::read ? "r" : "w"))
    {}

    FILE* get() const
    {
        return file;
    }

    bool is_open() const
    {
        return file != nullptr;
    }

    ~c_file()
    {
        if (file)
            fclose(file);
    }
};

void
merge_options(eastl::shared_ptr<context> ctx,
              const eastl::string& model_file_path,
              const eastl::string& options_file_path,
              const eastl::string& output_file_path)
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

    c_file ofs(output_file_path.c_str(), c_file::file_mode::write);
    if (!ofs.is_open()) {
        warning(ctx, "Fail to open DEXi file {}.\n", output_file_path.c_str());
        return;
    }

    model.set_options(options);
    model.write(ofs.get());
}

} // namespace efyj
