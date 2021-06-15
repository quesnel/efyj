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

#include "../src/utils.hpp"
#include <efyj/efyj.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

static void
show_context(const efyj::context& ctx) noexcept
{
    switch (ctx.status) {
    case efyj::status::success:
        break;
    case efyj::status::not_enough_memory:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    case efyj::status::numeric_cast_error:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    case efyj::status::internal_error:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    case efyj::status::file_error:
        py::print(
          "File error: ", get_error_message(ctx.status), " at ", ctx.data_1);
        break;
    case efyj::status::solver_error:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    case efyj::status::unconsistent_input_vector:
        break;
    case efyj::status::dexi_parser_scale_definition_error:
    case efyj::status::dexi_parser_scale_not_found:
    case efyj::status::dexi_parser_scale_too_big:
    case efyj::status::dexi_parser_file_format_error:
    case efyj::status::dexi_parser_not_enough_memory:
    case efyj::status::dexi_parser_element_unknown:
    case efyj::status::dexi_parser_option_conversion_error:
    case efyj::status::dexi_writer_error:
        py::print("DEXi error: ",
                  get_error_message(ctx.status),
                  " at line ",
                  ctx.line,
                  " column ",
                  ctx.column,
                  "with value: ",
                  ctx.data_1);
        break;
    case efyj::status::csv_parser_file_error:
    case efyj::status::csv_parser_column_number_incorrect:
    case efyj::status::csv_parser_scale_value_unknown:
    case efyj::status::csv_parser_column_conversion_failure:
    case efyj::status::csv_parser_basic_attribute_unknown:
    case efyj::status::csv_parser_init_dataset_simulation_empty:
    case efyj::status::csv_parser_init_dataset_cast_error:
        py::print("CSV error: ",
                  get_error_message(ctx.status),
                  " at line ",
                  ctx.line,
                  " column ",
                  ctx.column);
        break;
    case efyj::status::extract_option_same_input_files:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    case efyj::status::extract_option_fail_open_file:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    case efyj::status::merge_option_same_inputoutput:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    case efyj::status::merge_option_fail_open_file:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    case efyj::status::option_input_inconsistent:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    case efyj::status::scale_value_inconsistent:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    case efyj::status::option_too_many:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    case efyj::status::unknown_error:
        py::print("Error: ", get_error_message(ctx.status));
        break;
    }
}

PYBIND11_MODULE(pyefyj, m)
{
    m.doc() = R"pbdoc(
        pyefyj binding
        --------------
        .. currentmodule:: pyefyj
        .. autosummary::
           :toctree: _generate
           information
           adjustment
           prediction
    )pbdoc";

    m.attr("__version__") = MACRO_STRINGIFY(VERSION_MAJOR) "." MACRO_STRINGIFY(
      VERSION_MINOR) "." MACRO_STRINGIFY(VERSION_PATCH);

    py::class_<efyj::information_results>(m, "information_results")
      .def(py::init<>())
      .def_readonly("names", &efyj::information_results::basic_attribute_names)
      .def_readonly(
        "values",
        &efyj::information_results::basic_attribute_scale_value_numbers);

    py::class_<efyj::modifier>(m, "modifier")
      .def_readonly("attribute", &efyj::modifier::attribute)
      .def_readonly("line", &efyj::modifier::line)
      .def_readonly("value", &efyj::modifier::value);

    py::class_<efyj::result>(m, "results")
      .def(py::init<>())
      .def_readonly("modifiers", &efyj::result::modifiers)
      .def_readonly("kappa", &efyj::result::kappa)
      .def_readonly("time", &efyj::result::time)
      .def_readonly("kappa_computed", &efyj::result::kappa_computed)
      .def_readonly("function_computed", &efyj::result::function_computed);

    py::class_<efyj::data>(m, "data")
      .def(py::init<>())
      .def_readwrite("simulations", &efyj::data::simulations)
      .def_readwrite("places", &efyj::data::places)
      .def_readwrite("departments", &efyj::data::departments)
      .def_readwrite("years", &efyj::data::years)
      .def_readwrite("observed", &efyj::data::observed)
      .def_readwrite("scale_values", &efyj::data::scale_values);

    efyj::context ctx;
    ctx.out = &std::cout;
    ctx.err = &std::cerr;
    ctx.line = 0;
    ctx.column = 0;
    ctx.size = 0;
    ctx.data_1.reserve(256u);
    ctx.status = efyj::status::success;
    ctx.log_priority = efyj::log_level::info;

    m.def(
      "information",
      [&ctx](const std::string& s) -> efyj::information_results {
          efyj::information_results out;

          if (const auto ret = efyj::information(ctx, s, out); is_bad(ret))
              py::print("information(...) failed");

          return out;
      },
      R"pbdoc(
        Retrieves information from a DEXi file.
        Use this information to build correct scale values vector.
    )pbdoc");

    m.def(
      "evaluate",
      [&ctx](const std::string& s,
             const efyj::data& d) -> efyj::evaluation_results {
          efyj::evaluation_results out;

          if (const auto ret = efyj::evaluate(ctx, s, d, out); is_bad(ret)) {
              py::print("evaluation(...) failed");
          }

          return out;
      },
      R"pbdoc(
        Evaluation of DEXi file with data.
    )pbdoc");

    m.def(
      "adjustment",
      [&ctx](const std::string& model_file_path,
             const efyj::data& d) -> efyj::result {
          efyj::result out;
          const auto ret = efyj::adjustment(
            ctx,
            model_file_path,
            d,
            [&out](const efyj::result& r) {
                try {
                    out = r;
                    return true;
                } catch (...) {
                    return false;
                }
            },
            true,
            0,
            1u);

          if (is_bad(ret))
              py::print("adjustment failed");

          return out;
      },
      R"pbdoc(
        Compute adjustment of a DEXi file.
    )pbdoc");

    m.def(
      "prediction",
      [&ctx](const std::string& model_file_path,
             const efyj::data& d) -> efyj::result {
          efyj::result out;
          const auto ret = efyj::prediction(
            ctx,
            model_file_path,
            d,
            [&out](const efyj::result& r) {
                try {
                    out = r;
                    return true;
                } catch (...) {
                    return false;
                }
            },
            true,
            0,
            1u);

          if (is_bad(ret))
              py::print("adjustment failed");

          return out;
      },
      R"pbdoc(
        Compute prediction of a DEXi file.
    )pbdoc");

    m.def(
      "merge",
      [&ctx](const std::string& model_file_path,
             const std::string& output_file_path,
             const efyj::data& d) -> bool {
          if (const auto ret =
                efyj::merge_options(ctx, model_file_path, output_file_path, d);
              efyj::is_bad(ret)) {
              py::print("merge failed\n");
              return false;
          }

          return true;
      },
      R"pbdoc(
        Merge options data into existing DEXi file.
    )pbdoc");
}
