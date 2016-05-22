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
#include "context.hpp"
#include "utils.hpp"
#include "model.hpp"
#include "options.hpp"
#include "solver-stack.hpp"
#include "prediction.hpp"
#include "adjustment.hpp"
#include "post.hpp"
#include "types.hpp"
#include <fstream>
#include <chrono>
#include <iostream>

namespace efyj {

struct efyj::pimpl
{
    Model pp_model;
    Options pp_options;
    std::shared_ptr<Context> pp_context;

    pimpl()
        : pp_context(std::make_shared<Context>(LOG_OPTION_DEBUG))
    {
    }

    void model_read(const std::string& filepath)
    {
        std::ifstream ifs(filepath);
        if (not ifs) {
            pp_context->dbg().printf("fail to open `%s'", filepath.c_str());
            throw file_error(filepath);
        }

        pp_model.clear();
        pp_options.clear();
        pp_model.read(ifs);
    }

    void options_read(const std::string& filepath)
    {
        std::ifstream ifs(filepath);
        if (not ifs) {
            pp_context->dbg().printf("fail to open `%s'", filepath.c_str());
            throw file_error(filepath);
        }

        pp_options.clear();
        pp_options.read(pp_context, ifs, pp_model);
    }

    void extract_options(const std::string& output) const
    {
        std::ofstream ofs(output);
        if (not ofs) {
            pp_context->dbg().printf("fail to open `%s'", output.c_str());
            throw file_error(output);
        }

        pp_model.write_options(ofs);
    }

    void extract_options(std::vector <std::string>& simulations,
                         std::vector <std::string>& places,
                         std::vector <int>& departments,
                         std::vector <int>& years,
                         std::vector <int>& observed,
                         std::vector <int>& options) const
    {
        simulations = pp_options.simulations;
        places = pp_options.places;
        departments = pp_options.departments;
        years = pp_options.years;
        observed = pp_options.observed;

        const auto rows = pp_options.options.rows();
        const auto columns = pp_options.options.cols();

        options.resize(rows * columns);

        for (auto row = 0; row != rows; ++row)
            for (auto col = 0; col != columns; ++col)
                options[row * columns + col] = pp_options.options(row, col);
    }

    void set_options(const std::vector <std::string>& simulations,
                     const std::vector <std::string>& places,
                     const std::vector <int>& departments,
                     const std::vector <int>& years,
                     const std::vector <int>& observed,
                     const std::vector <int>& options)
    {
        // TODO add test to ensure simulation.size() == departments.size()
        // and etc.

        pp_options.set(simulations, places, departments,
                       years, observed, options);
    }

    int solve(const std::vector<int>& options)
    {
        if (pp_model.empty()) {
            pp_context->dbg().printf(
                "load model before trying compute functions.");
            return {};
        }

        solver_stack solver(pp_model);
        Eigen::VectorXi v = Eigen::Map<Eigen::VectorXi>(
            const_cast<int*>(options.data()),
            options.size());

        return solver.solve(v);
    }

    result compute_kappa() const
    {
        if (pp_options.empty() or pp_model.empty()) {
            pp_context->dbg().printf(
                "load options and model before trying compute functions.");
            return {};
        }

        std::chrono::time_point<std::chrono::system_clock> start, end;
        start = std::chrono::system_clock::now();

        solver_stack solver(pp_model);
        std::vector <int> simulated(pp_options.options.rows());
        weighted_kappa_calculator kappa_c(pp_model.attributes[0].scale.size());

        for (std::size_t i = 0, e = pp_options.options.rows(); i != e; ++i)
            simulated[i] = solver.solve(pp_options.options.row(i));

        auto kappa = kappa_c.squared(pp_options.observed, simulated);
        end = std::chrono::system_clock::now();

        auto duration = std::chrono::duration<double>(end - start).count();

        result ret;
        ret.kappa = kappa;
        ret.time = duration;
        ret.kappa_computed = 1;
        ret.function_computed = pp_options.options.rows();

        return ret;
    }

    std::vector <result> compute_prediction(int line_limit,
                                            double time_limit,
                                            int reduce_mode) const
    {
        if (pp_options.empty() or pp_model.empty()) {
            pp_context->dbg().printf(
                "load options and model before trying compute functions.");
            return {};
        }

        prediction_evaluator computer(pp_context, pp_model, pp_options);
        return computer.run(line_limit, time_limit, reduce_mode);
    }


    std::vector <result> compute_adjustment(int line_limit,
                                            double time_limit,
                                            int reduce_mode) const
    {
        if (pp_options.empty() or pp_model.empty()) {
            pp_context->dbg().printf(
                "load options and model before trying compute functions.");
            return {};
        }

        adjustment_evaluator computer(pp_context, pp_model, pp_options);
        return computer.run(line_limit, time_limit, reduce_mode);
    }

    void clear()
    {
        pp_options.clear();
        pp_model.clear();
    }
};

efyj::efyj()
    : pp_impl(new efyj::pimpl())
{
}

efyj::efyj(const std::string& model_filepath)
    : pp_impl(new efyj::pimpl())
{
    pp_impl->model_read(model_filepath);
}

efyj::efyj(const std::string& model_filepath,
           const std::string& options_filepath)
    : pp_impl(new efyj::pimpl())
{
    pp_impl->model_read(model_filepath);
    pp_impl->options_read(options_filepath);
}

efyj::efyj(efyj&& other) = default;
efyj& efyj::operator=(efyj&& other) = default;
efyj::~efyj() noexcept = default;

void efyj::extract_options(const std::string& output) const
{
    pp_impl->extract_options(output);
}

void efyj::extract_options(std::vector <std::string>& simulations,
                           std::vector <std::string>& places,
                           std::vector <int>& departments,
                           std::vector <int>& years,
                           std::vector <int>& observed,
                           std::vector <int>& options) const
{
    pp_impl->extract_options(simulations, places, departments,
                             years, observed, options);
}

void efyj::set_options(const std::vector <std::string>& simulations,
                       const std::vector <std::string>& places,
                       const std::vector <int>& departments,
                       const std::vector <int>& years,
                       const std::vector <int>& observed,
                       const std::vector <int>& options)
{
    pp_impl->set_options(simulations, places, departments,
                         years, observed, options);
}

int efyj::solve(const std::vector<int>& options)
{
    return pp_impl->solve(options);
}

result efyj::compute_kappa() const
{
    return pp_impl->compute_kappa();
}

std::vector<result> efyj::compute_prediction(int line_limit, double time_limit,
                                             int reduce_mode) const
{
    return pp_impl->compute_prediction(line_limit, time_limit, reduce_mode);
}

std::vector<result> efyj::compute_adjustment(int line_limit, double time_limit,
                                             int reduce_mode) const
{
    return pp_impl->compute_adjustment(line_limit, time_limit, reduce_mode);
}

void efyj::clear()
{
    pp_impl->clear();
}

} // namespace efyj
