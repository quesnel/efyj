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

#include <RcppCommon.h>

#include "../../lib/src/utils.hpp"
#include <efyj/efyj.hpp>

#include <Rcpp.h>

static void
show_context(const efyj::context& ctx) noexcept
{
    switch (ctx.status) {
    case efyj::status::success:
        break;
    case efyj::status::not_enough_memory:
        Rcpp::Rcerr << get_error_message(ctx.status) << ' ' << ctx.size
                    << '\n';
        break;
    case efyj::status::numeric_cast_error:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::internal_error:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::file_error:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::solver_error:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::unconsistent_input_vector:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::dexi_parser_scale_definition_error:
    case efyj::status::dexi_parser_scale_not_found:
    case efyj::status::dexi_parser_scale_too_big:
    case efyj::status::dexi_parser_file_format_error:
    case efyj::status::dexi_parser_not_enough_memory:
    case efyj::status::dexi_parser_element_unknown:
    case efyj::status::dexi_parser_option_conversion_error:
    case efyj::status::dexi_writer_error:
        Rcpp::Rcerr << "dexi error {} - {} at line {} column {}\n"
                    << get_error_message(ctx.status) << ctx.data_1 << ctx.line
                    << ctx.column << '\n';
        break;
    case efyj::status::csv_parser_file_error:
    case efyj::status::csv_parser_column_number_incorrect:
    case efyj::status::csv_parser_scale_value_unknown:
    case efyj::status::csv_parser_column_conversion_failure:
    case efyj::status::csv_parser_basic_attribute_unknown:
    case efyj::status::csv_parser_init_dataset_simulation_empty:
    case efyj::status::csv_parser_init_dataset_cast_error:
        Rcpp::Rcerr << "csv error {} - {} at line {} column {}\n"
                    << get_error_message(ctx.status) << ctx.data_1 << ctx.line
                    << ctx.column << '\n';
        break;
    case efyj::status::extract_option_same_input_files:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::extract_option_fail_open_file:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::merge_option_same_inputoutput:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::merge_option_fail_open_file:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::option_input_inconsistent:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::scale_value_inconsistent:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::option_too_many:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    case efyj::status::unknown_error:
        Rcpp::Rcerr << get_error_message(ctx.status) << '\n';
        break;
    }
}

//' Extract information from DEXi file.
//'
//' This function parses the DEXi file and returns some informations mode.
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
information(const Rcpp::String& model)
{
    try {
        efyj::context ctx;
        ctx.out = &Rcpp::Rcout;
        ctx.err = &Rcpp::Rcerr;
        ctx.line = 0;
        ctx.column = 0;
        ctx.size = 0;
        ctx.data_1.reserve(256u);
        ctx.status = efyj::status::success;
        ctx.log_priority = efyj::log_level::info;

        efyj::information_results out;

        if (const auto ret = efyj::information(ctx, model, out); is_bad(ret)) {
            show_context(ctx);
            const auto msg = efyj::get_error_message(ret);
            Rprintf("Information failed: %s\n", msg);
            return R_NilValue;
        }

        return Rcpp::List::create(
          Rcpp::Named("basic_attribute_names") =
            Rcpp::wrap(out.basic_attribute_names),
          Rcpp::Named("basic_attribute_scale_values") =
            Rcpp::wrap(out.basic_attribute_scale_value_numbers));
    } catch (const std::bad_alloc& e) {
        Rcpp::stop("failed: %s\n", e.what());
    } catch (const std::exception& e) {
        Rcpp::stop("failed: %s\n", e.what());
    } catch (...) {
        Rcpp::stop("failed: unknown error\n");
    }

    return R_NilValue;
}

//' Simulate all options for a DEXi file model.
//'
//' This function parses the DEXi and for each row of all vectors,
//' it simulates the model. This function returns a list of options,
//' agregate attributes and simulations results.
//'
//' @param model The file path of the DEXi model
//' @param simulations A vector of strings
//' @param places A vector of strings
//' @param departments A vector of integers
//' @param years A vector of integers
//' @param observed A vector of integers
//' @param scale_values A vector of integers with the number of aggregate
//' table times number of row in simulations, places and other vectors.
//'
//' @return A List with the list of simulation and observation vectors
//' the kappa linear and the kappa squared.
//'
//' @export
// [[Rcpp::export]]
Rcpp::List
evaluate(const Rcpp::String& model,
         const Rcpp::CharacterVector& simulations,
         const Rcpp::CharacterVector& places,
         const Rcpp::NumericVector& departments,
         const Rcpp::NumericVector& years,
         const Rcpp::NumericVector& observed,
         const Rcpp::NumericVector& scale_values)
{
    try {
        efyj::context ctx;
        ctx.out = &Rcpp::Rcout;
        ctx.err = &Rcpp::Rcerr;
        ctx.line = 0;
        ctx.column = 0;
        ctx.size = 0;
        ctx.data_1.reserve(256u);
        ctx.status = efyj::status::success;
        ctx.log_priority = efyj::log_level::info;

        if (simulations.length() != places.length() ||
            simulations.length() != departments.length() ||
            simulations.length() != years.length() ||
            simulations.length() != observed.length()) {
            Rprintf("'simulations', 'places', 'departments', 'years', "
                    "'observed' must have the same length.\n");
            return R_NilValue;
        }
        efyj::evaluation_results out;

        efyj::data d;
        d.simulations = Rcpp::as<std::vector<std::string>>(simulations);
        d.places = Rcpp::as<std::vector<std::string>>(places);
        d.departments = Rcpp::as<std::vector<int>>(departments);
        d.years = Rcpp::as<std::vector<int>>(years);
        d.observed = Rcpp::as<std::vector<int>>(observed);
        d.scale_values = Rcpp::as<std::vector<int>>(scale_values);

        if (const auto ret = efyj::evaluate(ctx, model, d, out); is_bad(ret)) {
            show_context(ctx);
            Rprintf("Evaluation failed: %s\n", efyj::get_error_message(ret));
            return R_NilValue;
        }

        return Rcpp::List::create(
          Rcpp::Named("simulations") = Rcpp::wrap(out.simulations),
          Rcpp::Named("observation") = Rcpp::wrap(out.observations),
          Rcpp::Named("linear_weighted_kappa") =
            Rcpp::wrap(out.linear_weighted_kappa),
          Rcpp::Named("squared_weighted_kappa") =
            Rcpp::wrap(out.squared_weighted_kappa));
    } catch (const std::bad_alloc& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (const std::exception& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (...) {
        Rprintf("failed: unknown error\n");
    }

    return R_NilValue;
}

//' Extracts options from DEXi file to a CSV file.
//'
//' @param model The file path of the DEXi model.
//' @param options The output path for the CSV file.
//'
//' @return Nothing
//'
//' @export
// [[Rcpp::export]]
void
extract_to_file(const Rcpp::String& model, const Rcpp::String& options)
{
    try {
        efyj::context ctx;
        ctx.out = &Rcpp::Rcout;
        ctx.err = &Rcpp::Rcerr;
        ctx.line = 0;
        ctx.column = 0;
        ctx.size = 0;
        ctx.data_1.reserve(256u);
        ctx.status = efyj::status::success;
        ctx.log_priority = efyj::log_level::info;

        if (const auto ret =
              efyj::extract_options_to_file(ctx, model, options);
            is_bad(ret)) {
            show_context(ctx);
            const auto msg = efyj::get_error_message(ret);
            Rprintf("Extact-to-file failed: %s\n", msg);
        }
    } catch (const std::bad_alloc& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (const std::exception& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (...) {
        Rprintf("failed: unknown error\n");
    }
}

//' Extracts options from DEXi file.
//'
//' @param model The file path of the DEXi model.
//'
//' @return A List with the list simulation identifiers, places, departments,
//' years, observation and all scale values (the number of aggregate
//' table times number of row in simulations, places and other vectors).
//'
//' @export
// [[Rcpp::export]]
Rcpp::List
extract(const Rcpp::String& model)
{
    try {
        efyj::context ctx;
        ctx.out = &Rcpp::Rcout;
        ctx.err = &Rcpp::Rcerr;
        ctx.line = 0;
        ctx.column = 0;
        ctx.size = 0;
        ctx.data_1.reserve(256u);
        ctx.status = efyj::status::success;
        ctx.log_priority = efyj::log_level::info;

        efyj::data d;

        if (const auto ret = efyj::extract_options(ctx, model, d);
            is_bad(ret)) {
            show_context(ctx);
            const auto msg = efyj::get_error_message(ret);
            Rprintf("Extract failed: %s\n", msg);
            return R_NilValue;
        }

        return Rcpp::List::create(
          Rcpp::Named("simulations") = Rcpp::wrap(d.simulations),
          Rcpp::Named("places") = Rcpp::wrap(d.places),
          Rcpp::Named("departments") = Rcpp::wrap(d.departments),
          Rcpp::Named("years") = Rcpp::wrap(d.years),
          Rcpp::Named("observed") = Rcpp::wrap(d.observed),
          Rcpp::Named("scale_values") = Rcpp::wrap(d.scale_values));
    } catch (const std::bad_alloc& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (const std::exception& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (...) {
        Rprintf("failed: unknown error\n");
    }

    return R_NilValue;
}

static void
check_user_interrupt(void* /*user_data*/)
{
    Rcpp::checkUserInterrupt();
}

struct result_fn
{
    std::vector<int>& all_modifiers;
    std::vector<double>& all_kappa;
    std::vector<double>& all_time;
    const int limit = 0;
    int current_limit = 0;

    result_fn(std::vector<int>& all_modifiers_,
              std::vector<double>& all_kappa_,
              std::vector<double>& all_time_,
              const int limit_)
      : all_modifiers(all_modifiers_)
      , all_kappa(all_kappa_)
      , all_time(all_time_)
      , limit(limit_)
    {
    }
};

static bool
update_result(const efyj::result& r, void* user_data)
{
    auto* result = reinterpret_cast<result_fn*>(user_data);

    try {
        for (const auto& elem : r.modifiers) {
            Rprintf("[%d %d %d] ", elem.attribute, elem.line, elem.value);
            result->all_modifiers.emplace_back(elem.attribute);
            result->all_modifiers.emplace_back(elem.line);
            result->all_modifiers.emplace_back(elem.value);
        }

        Rprintf("%.10g %.10gs %d %d\n",
                r.kappa,
                r.time,
                r.kappa_computed,
                r.function_computed);

        result->all_kappa.emplace_back(r.kappa);
        result->all_time.emplace_back(r.time);

        ++result->current_limit;

        return result->current_limit < result->limit;
    } catch (...) {
        return false;
    }
}

//' Adjustment for all options for a DExi file.
//'
//' This function parses the dexi and csv files and for each row of the csv
//' file, it simulates the model. This function returns a list of options,
//' agregate attributes and simulations results.
//'
//' @param model The file path of the DEXi model
//' @param simulations A vector of strings
//' @param places A vector of strings
//' @param departments A vector of integers
//' @param years A vector of integers
//' @param observed A vector of integers
//' @param scale_values A vector of integers with the number of aggregate
//' table times number of row in simulations, places and other vectors.
//'
//' @return A List with all change in DEXi file to get the better values
//' the vector the kappa linear and the kappa squared.
//'
//' @export
// [[Rcpp::export]]
Rcpp::List
adjustment(const Rcpp::String& model,
           const Rcpp::CharacterVector& simulations,
           const Rcpp::CharacterVector& places,
           const Rcpp::NumericVector& departments,
           const Rcpp::NumericVector& years,
           const Rcpp::NumericVector& observed,
           const Rcpp::NumericVector& scale_values,
           const bool reduce,
           const int limit,
           const int thread)
{
    try {
        efyj::context ctx;
        ctx.out = &Rcpp::Rcout;
        ctx.err = &Rcpp::Rcerr;
        ctx.line = 0;
        ctx.column = 0;
        ctx.size = 0;
        ctx.data_1.reserve(256u);
        ctx.status = efyj::status::success;
        ctx.log_priority = efyj::log_level::info;

        std::vector<int> all_modifiers;
        std::vector<double> all_kappa;
        std::vector<double> all_time;
        result_fn fn(all_modifiers, all_kappa, all_time, limit);

        if (simulations.length() != places.length() ||
            simulations.length() != departments.length() ||
            simulations.length() != years.length() ||
            simulations.length() != observed.length()) {
            Rprintf("'simulations', 'places', 'departments', 'years', "
                    "'observed' must have the same length.\n");
            return R_NilValue;
        }

        if (scale_values.length() % simulations.length() > 0) {
            Rprintf(
              "'scale_values' lenght must be a multiple of other vectors.\n");
            return R_NilValue;
        }

        if (thread <= 0) {
            Rprintf("'thread' must be a positive value.\n");
            return R_NilValue;
        }

        efyj::data d;
        d.simulations = Rcpp::as<std::vector<std::string>>(simulations);
        d.places = Rcpp::as<std::vector<std::string>>(places);
        d.departments = Rcpp::as<std::vector<int>>(departments);
        d.years = Rcpp::as<std::vector<int>>(years);
        d.observed = Rcpp::as<std::vector<int>>(observed);
        d.scale_values = Rcpp::as<std::vector<int>>(scale_values);

        if (const auto ret = efyj::adjustment(ctx,
                                              model,
                                              d,
                                              update_result,
                                              &fn,
                                              check_user_interrupt,
                                              nullptr,
                                              reduce,
                                              limit,
                                              thread);
            is_bad(ret)) {
            const auto msg = efyj::get_error_message(ret);
            Rprintf("Adjustment failed: %s\n", msg);
            return R_NilValue;
        }

        return Rcpp::List::create(Rcpp::Named("modifiers") =
                                    Rcpp::wrap(all_modifiers),
                                  Rcpp::Named("kappa") = Rcpp::wrap(all_kappa),
                                  Rcpp::Named("time") = Rcpp::wrap(all_time));
    } catch (const std::bad_alloc& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (const std::exception& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (...) {
        Rprintf("failed: unknown error\n");
    }

    return R_NilValue;
}

//' Prediction for a limited options for a DExi file.
//'
//' This function parses the dexi and csv files and for each row of the csv
//' file, it simulates the model. This function returns a list of options,
//' agregate attributes and simulations results.
//'
//' @param model The file path of the DEXi model
//' @param simulations A vector of strings
//' @param places A vector of strings
//' @param departments A vector of integers
//' @param years A vector of integers
//' @param observed A vector of integers
//' @param scale_values A vector of integers with the number of aggregate
//' table times number of row in simulations, places and other vectors.
//'
//' @return A List with all change in DEXi file to get the better values
//' the vector the kappa linear and the kappa squared.
//'
//' @export
// [[Rcpp::export]]
Rcpp::List
prediction(const Rcpp::String& model,
           const Rcpp::CharacterVector& simulations,
           const Rcpp::CharacterVector& places,
           const Rcpp::NumericVector& departments,
           const Rcpp::NumericVector& years,
           const Rcpp::NumericVector& observed,
           const Rcpp::NumericVector& scale_values,
           const bool reduce,
           const int limit,
           const int thread)
{
    try {
        efyj::context ctx;
        ctx.out = &Rcpp::Rcout;
        ctx.err = &Rcpp::Rcerr;
        ctx.line = 0;
        ctx.column = 0;
        ctx.size = 0;
        ctx.data_1.reserve(256u);
        ctx.status = efyj::status::success;
        ctx.log_priority = efyj::log_level::info;

        std::vector<int> all_modifiers;
        std::vector<double> all_kappa;
        std::vector<double> all_time;
        result_fn fn(all_modifiers, all_kappa, all_time, limit);

        if (simulations.length() != places.length() ||
            simulations.length() != departments.length() ||
            simulations.length() != years.length() ||
            simulations.length() != observed.length()) {
            Rprintf("'simulations', 'places', 'departments', 'years', "
                    "'observed' must have the same length.");
            return R_NilValue;
        }

        if (scale_values.length() % simulations.length() > 0) {
            Rprintf(
              "'scale_values' lenght must be a multiple of other vectors.");
            return R_NilValue;
        }

        if (thread <= 0) {
            Rprintf("'thread' must be a positive value");
            return R_NilValue;
        }

        efyj::data d;
        d.simulations = Rcpp::as<std::vector<std::string>>(simulations);
        d.places = Rcpp::as<std::vector<std::string>>(places);
        d.departments = Rcpp::as<std::vector<int>>(departments);
        d.years = Rcpp::as<std::vector<int>>(years);
        d.observed = Rcpp::as<std::vector<int>>(observed);
        d.scale_values = Rcpp::as<std::vector<int>>(scale_values);

        if (const auto ret = efyj::prediction(ctx,
                                              model,
                                              d,
                                              update_result,
                                              &fn,
                                              check_user_interrupt,
                                              nullptr,
                                              reduce,
                                              limit,
                                              thread);
            is_bad(ret)) {
            show_context(ctx);
            const auto msg = efyj::get_error_message(ret);
            Rprintf("failed: %s\n", msg);
            return R_NilValue;
        }

        return Rcpp::List::create(Rcpp::Named("modifiers") =
                                    Rcpp::wrap(all_modifiers),
                                  Rcpp::Named("kappa") = Rcpp::wrap(all_kappa),
                                  Rcpp::Named("time") = Rcpp::wrap(all_time));
    } catch (const std::bad_alloc& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (const std::exception& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (...) {
        Rprintf("failed: unknown error\n");
    }

    return R_NilValue;
}

//' Extract information from DEXi file.
//'
//' This function parses the dexi and returns some informations about DEXi
//' file.
//'
//' @param model The file path of the DEXi model
//' @param output The file path of the new DEXi model
//' @param simulations A vector of strings
//' @param places A vector of strings
//' @param departments A vector of integers
//' @param years A vector of integers
//' @param observed A vector of integers
//' @param scale_values A vector integers with
//'
//' @return A List of two List: a list of basic attribute names and a list of
//' basic attribute max scale value number.
//'
//' @useDynLib refyj
//' @importFrom Rcpp sourceCpp
//'
//' @export
// [[Rcpp::export]]
bool
merge(const Rcpp::String& model,
      const Rcpp::String& out,
      const Rcpp::CharacterVector& simulations,
      const Rcpp::CharacterVector& places,
      const Rcpp::NumericVector& departments,
      const Rcpp::NumericVector& years,
      const Rcpp::NumericVector& observed,
      const Rcpp::NumericVector& scale_values)
{
    try {
        efyj::context ctx;
        ctx.out = &Rcpp::Rcout;
        ctx.err = &Rcpp::Rcerr;
        ctx.line = 0;
        ctx.column = 0;
        ctx.size = 0;
        ctx.data_1.reserve(256u);
        ctx.status = efyj::status::success;
        ctx.log_priority = efyj::log_level::info;

        if (simulations.length() != places.length() ||
            simulations.length() != departments.length() ||
            simulations.length() != years.length() ||
            simulations.length() != observed.length()) {
            Rprintf("'simulations', 'places', 'departments', 'years', "
                    "'observed' must have the same length.");
            return R_NilValue;
        }

        if (scale_values.length() % simulations.length() > 0) {
            Rprintf(
              "'scale_values' lenght must be a multiple of other vectors.");
            return R_NilValue;
        }

        efyj::data d;
        d.simulations = Rcpp::as<std::vector<std::string>>(simulations);
        d.places = Rcpp::as<std::vector<std::string>>(places);
        d.departments = Rcpp::as<std::vector<int>>(departments);
        d.years = Rcpp::as<std::vector<int>>(years);
        d.observed = Rcpp::as<std::vector<int>>(observed);
        d.scale_values = Rcpp::as<std::vector<int>>(scale_values);

        if (const auto ret = efyj::merge_options(ctx, model, out, d);
            is_bad(ret)) {
            show_context(ctx);
            const auto msg = efyj::get_error_message(ret);
            Rprintf("failed: %s\n", msg);
            return R_NilValue;
        }
    } catch (const std::bad_alloc& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (const std::exception& e) {
        Rprintf("failed: %s\n", e.what());
    } catch (...) {
        Rprintf("failed: unknown error\n");
    }

    return R_NilValue;
}
