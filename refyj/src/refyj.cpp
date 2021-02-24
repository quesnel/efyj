/* Copyright (C) 2016-2021 INRAE
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

#include "../../lib/src/utils.hpp"
#include <efyj/efyj.hpp>

#include <Rcpp.h>

static void
init_context(efyj::context& ctx) noexcept
{
    ctx.dexi_cb = [](const efyj::status s,
                     int line,
                     int column,
                     const std::string_view tag) {
        const auto s_str = efyj::get_error_message(s);

        Rprintf("DEXi error: %.*s at line %d column %d with tag %.*s\n",
                static_cast<int>(s_str.size()),
                s_str.data(),
                line,
                column,
                static_cast<int>(tag.size()),
                tag.data());
    };

    ctx.csv_cb = [](const efyj::status s, int line, int column) {
        const auto s_str = efyj::get_error_message(s);

        Rprintf("CSV error: %.*s at line %d column %d\n",
                static_cast<int>(s_str.size()),
                s_str.data(),
                line,
                column);
    };

    ctx.eov_cb = []() { Rprintf("Not enough memory to continue\n"); };

    ctx.cast_cb = []() { Rprintf("Internal error: cast failure\n"); };

    ctx.solver_cb = []() { Rprintf("Solver error\n"); };

    ctx.file_cb = [](const std::string_view file_name) {
        Rprintf("Error to access file `%.*s\n",
                static_cast<int>(file_name.size()),
                file_name.data());
        ;
    };
}

//' Extract information from DEXi file.
//'
//' This function parses the dexi and returns some informations about DEXi
//' file.
//'
//' @param model The file path of the DEXi model
//'
//' @return A List of two List: a list of basic attribute names and a list of
//' basic attribute max scale value number.
//'
//' @useDynLib refyj
//' @importFrom Rcpp sourceCpp
//'
//' @export
// [[Rcpp::export]]
Rcpp::List
information(const std::string& model)
{
    try {
        efyj::context ctx;
        efyj::information_results out;

        init_context(ctx);

        if (const auto ret = efyj::information(ctx, model, out); is_bad(ret)) {
            Rprintf("refyj::information(...) failed\n");
            return Rcpp::List();
        }

        return Rcpp::List::create(
          Rcpp::Named("basic_attribute_names") =
            Rcpp::wrap(out.basic_attribute_names),
          Rcpp::Named("basic_attribute_scale_values") =
            Rcpp::wrap(out.basic_attribute_scale_value_numbers));
    } catch (const std::bad_alloc& e) {
        Rprintf("refyj::information: %s\n", e.what());
    } catch (const std::exception& e) {
        Rprintf("refyj::information: %s\n", e.what());
    } catch (...) {
        Rprintf("refyj::information: unknown error\n");
    }

    return Rcpp::List();
}

//' Simulate all options for a dexi file.
//'
//' This function parses the dexi and csv files and for each row of the csv
//' file, it simulates the omdel. This function returns a list of options,
//' agregate attributes and simulations results.
//'
//' @param model The file path of the DEXi model
//' @param simulations A vector of strings
//' @param places A vector of strings
//' @param departments A vector of integers
//' @param years A vector of integers
//' @param observed A vector of integers
//' @param scale_values A vector integers with
//'
//' @return A List with the list of observation scale value, the list of
//' simulation scale values, and two double kappa linear and kappa squared.
//'
//' @export
// [[Rcpp::export]]
Rcpp::List
evaluate(const std::string& model,
         const std::vector<std::string>& simulations,
         const std::vector<std::string>& places,
         const std::vector<int>& departments,
         const std::vector<int>& years,
         const std::vector<int>& observed,
         const std::vector<int>& scale_values)
{
    try {
        efyj::context ctx;
        efyj::evaluation_results out;

        efyj::data d;
        d.simulations = simulations;
        d.places = places;
        d.departments = departments;
        d.years = years;
        d.observed = observed;
        d.scale_values = scale_values;

        init_context(ctx);

        if (const auto ret = efyj::evaluate(ctx, model, d, out); is_bad(ret)) {
            Rprintf("refyj::evaluate(...) failed\n");
            return Rcpp::List();
        }

        return Rcpp::List::create(
          Rcpp::Named("simulations") = Rcpp::wrap(out.simulations),
          Rcpp::Named("observation") = Rcpp::wrap(out.observations),
          Rcpp::Named("linear_weighted_kappa") =
            Rcpp::wrap(out.linear_weighted_kappa),
          Rcpp::Named("squared_weighted_kappa") =
            Rcpp::wrap(out.squared_weighted_kappa));
    } catch (const std::bad_alloc& e) {
        Rprintf("refyj::evaluate: %s\n", e.what());
    } catch (const std::exception& e) {
        Rprintf("refyj::evaluate: %s\n", e.what());
    } catch (...) {
        Rprintf("refyj::evaluate: unknown error\n");
    }

    return Rcpp::List();
}
