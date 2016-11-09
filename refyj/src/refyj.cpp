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

#include <Rcpp.h>
#include <efyj/efyj.hpp>

class Rcontext : public efyj::logger
{
public:
    Rcontext()
      : efyj::logger()
    {
    }

    virtual ~Rcontext() noexcept {}

    virtual void write(int priority,
                       const char* file,
                       int line,
                       const char* fn,
                       const char* format,
                       va_list ap) noexcept
    {
        if (priority <= 3) {
            Rprintf("efyj %d at %d in function %s form file %s: ",
                    priority,
                    line,
                    fn,
                    file);

            try {
                std::string buffer(256, '\0');
                int sz;

                for (;;) {
                    sz = std::vsnprintf(&buffer[0], buffer.size(), format, ap);

                    if (sz < 0)
                        return;

                    if (static_cast<std::size_t>(sz) < buffer.size()) {
                        Rprintf("%s", buffer.c_str());
                        return;
                    }

                    buffer.resize(sz + 1);
                }
            } catch (const std::exception& /*e*/) {
            }
        }
    }

    virtual void write(efyj::message_type m,
                       const char* format,
                       va_list args) noexcept
    {
        (void)m;
        (void)format;
        (void)args;
    }
};

//' Extract information from DEXi file.
//'
//' This function parses the dexi and returns some informations about DEXi
//' file.
//'
//' @param model The file path of the DEXi model
//'
//' @return A List with a list of scale value names for each attributes, a
//' list of basic attributes and the number of attributes.
//'
//' @useDynLib refyj
//' @importFrom Rcpp sourceCpp
//'
//' @export
// [[Rcpp::export]]
Rcpp::List
extract_model(const std::string& model)
{
    try {
        auto ctx = std::make_shared<efyj::context>();
        ctx->set_logger(std::unique_ptr<efyj::logger>(new Rcontext));

        const auto x = efyj::extract_model(ctx, model);

        return Rcpp::List::create(
          Rcpp::wrap(x.attributes), Rcpp::wrap(x.basic_attributes), x.number);
    } catch (const std::bad_alloc& e) {
        Rcpp::Rcout << "refyj: " << e.what() << '\n';
    } catch (const std::exception& e) {
        Rcpp::Rcout << "refyj: " << e.what() << '\n';
    } catch (...) {
        Rcpp::Rcout << "refyj: unknown error\n";
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
//' @param options The file path of the csv options
//'
//' @return A List with options (matrix of options), aggregate attributes and
//' simulation results.
//'
//' @export
// [[Rcpp::export]]
Rcpp::List
simulate(const std::string& model, const std::string& options)
{
    try {
        auto ctx = std::make_shared<efyj::context>();
        ctx->set_logger(std::unique_ptr<efyj::logger>(new Rcontext));

        const auto x = efyj::simulate(ctx, model, options);

        Rcpp::IntegerMatrix options(x.options.columns(), x.options.rows());
        Rcpp::IntegerVector simulations(x.simulations.size());
        Rcpp::IntegerMatrix attributes(x.attributes.columns(),
                                       x.attributes.rows());

        for (std::size_t r = 0, end_r = x.attributes.rows(); r != end_r; ++r)
            for (std::size_t c = 0, end_c = x.attributes.columns(); c != end_c;
                 ++c)
                attributes(c, r) = x.attributes(c, r);

        for (std::size_t r = 0, end_r = x.options.rows(); r != end_r; ++r)
            for (std::size_t c = 0, end_c = x.options.columns(); c != end_c;
                 ++c)
                options(c, r) = x.options(c, r);

        for (std::size_t i = 0, end_i = simulations.size(); i != end_i; ++i)
            simulations(i) = x.simulations[i];

        return Rcpp::List::create(options, attributes, simulations);
    } catch (const std::bad_alloc& e) {
        Rcpp::Rcout << "refyj: " << e.what() << '\n';
    } catch (const std::exception& e) {
        Rcpp::Rcout << "refyj: " << e.what() << '\n';
    } catch (...) {
        Rcpp::Rcout << "refyj: unknown error\n";
    }

    return Rcpp::List();
}

//' Evaluate all options for a dexi file.
//'
//' This function parses the dexi and csv files and for each row of the csv
//' file, it simulates the omdel. This function returns a list of options,
//' agregate attributes and simulations results.
//'
//' @param model The file path of the DEXi model
//' @param options The file path of the csv options
//'
//' @return A List with options (matrix of options), aggregate attributes and
//' simulation results.
//'
//' @export
// [[Rcpp::export]]
Rcpp::List
evaluate(const std::string& model, const std::string& options)
{
    try {
        auto ctx = std::make_shared<efyj::context>();
        ctx->set_logger(std::unique_ptr<efyj::logger>(new Rcontext));

        const auto x = efyj::evaluate(ctx, model, options);

        Rcpp::IntegerMatrix options(x.options.columns(), x.options.rows());
        Rcpp::IntegerVector simulations(x.simulations.size());
        Rcpp::IntegerVector observations(x.observations.size());
        Rcpp::IntegerMatrix attributes(x.attributes.columns(),
                                       x.attributes.rows());
        Rcpp::IntegerMatrix confusion(x.confusion.columns(),
                                      x.confusion.rows());

        for (std::size_t r = 0, end_r = x.attributes.rows(); r != end_r; ++r)
            for (std::size_t c = 0, end_c = x.attributes.columns(); c != end_c;
                 ++c)
                attributes(c, r) = x.attributes(c, r);

        for (std::size_t r = 0, end_r = x.options.rows(); r != end_r; ++r)
            for (std::size_t c = 0, end_c = x.options.columns(); c != end_c;
                 ++c)
                options(c, r) = x.options(c, r);

        for (std::size_t i = 0, end_i = simulations.size(); i != end_i; ++i) {
            simulations(i) = x.simulations[i];
            observations(i) = x.observations[i];
        }

        for (std::size_t r = 0, end_r = x.confusion.rows(); r != end_r; ++r)
            for (std::size_t c = 0, end_c = x.confusion.columns(); c != end_c;
                 ++c)
                confusion(c, r) = x.confusion(c, r);

        return Rcpp::List::create(options,
                                  attributes,
                                  simulations,
                                  observations,
                                  confusion,
                                  x.linear_weighted_kappa,
                                  x.squared_weighted_kappa);
    } catch (const std::bad_alloc& e) {
        Rcpp::Rcout << "refyj: " << e.what() << '\n';
    } catch (const std::exception& e) {
        Rcpp::Rcout << "refyj: " << e.what() << '\n';
    } catch (...) {
        Rcpp::Rcout << "refyj: unknown error\n";
    }

    return Rcpp::List();
}

//' Extract the options from a Dexi file.
//'
//' This function parses the dexi and csv files and for each row of the csv
//' file, it simulates the omdel. This function returns a list of options,
//' agregate attributes and simulations results.
//'
//' @param model The file path of the DEXi model
//'
//' @return A List with: simulations name vector, places name vector,
//' departments vector, years vector, observed vector and matrix of options.
//'
//' @export
// [[Rcpp::export]]
Rcpp::List
extract_options(const std::string& model)
{
    try {
        auto ctx = std::make_shared<efyj::context>();
        ctx->set_logger(std::unique_ptr<efyj::logger>(new Rcontext));

        const auto x = efyj::extract_options(ctx, model);

        Rcpp::StringVector simulations(x.simulations.size());
        Rcpp::StringVector places(x.simulations.size());
        Rcpp::IntegerVector departments(x.simulations.size());
        Rcpp::IntegerVector years(x.simulations.size());
        Rcpp::IntegerMatrix options(x.options.columns(), x.options.rows());

        for (std::size_t i = 0, end_i = x.simulations.size(); i != end_i; ++i)
            simulations[i] = x.simulations[i];

        for (std::size_t i = 0, end_i = x.simulations.size(); i != end_i; ++i)
            places[i] = x.places[i];

        for (std::size_t i = 0, end_i = x.simulations.size(); i != end_i; ++i)
            departments[i] = x.departments[i];

        for (std::size_t i = 0, end_i = x.simulations.size(); i != end_i; ++i)
            years[i] = x.years[i];

        for (std::size_t r = 0, end_r = x.options.rows(); r != end_r; ++r)
            for (std::size_t c = 0, end_c = x.options.columns(); c != end_c;
                 ++c)
                options(c, r) = x.options(c, r);

        return Rcpp::List::create(
          simulations, places, departments, years, options);
    } catch (const std::bad_alloc& e) {
        Rcpp::Rcout << "refyj: " << e.what() << '\n';
    } catch (const std::exception& e) {
        Rcpp::Rcout << "refyj: " << e.what() << '\n';
    } catch (...) {
        Rcpp::Rcout << "refyj: unknown error\n";
    }

    return Rcpp::List();
}
