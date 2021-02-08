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

#include <efyj/efyj.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

static void
show_error_message(const efyj::status s) noexcept
{
    switch (s) {
    case efyj::status::success:
        break;
    case efyj::status::numeric_cast_error:
        py::print("internal integer cast error");
        break;
    case efyj::status::internal_error:
        py::print("internal error");
        break;
    case efyj::status::file_error:
        py::print("file access error");
        break;
    case efyj::status::solver_error:
        py::print("internal solver error");
        break;
    case efyj::status::unconsistent_input_vector:
        py::print("unconsistent input vector");
        break;
    case efyj::status::dexi_parser_scale_definition_error:
        py::print("dexi file parser scale definition error");
        break;
    case efyj::status::dexi_parser_scale_not_found:
        py::print("dexi parser scale not found");
        break;
    case efyj::status::dexi_parser_scale_too_big:
        py::print("dexi parser scale too big");
        break;
    case efyj::status::dexi_parser_file_format_error:
        py::print("dexi parser file format error");
        break;
    case efyj::status::dexi_parser_not_enough_memory:
        py::print("dexi parser not enough memory");
        break;
    case efyj::status::dexi_parser_element_unknown:
        py::print("dexi parser element unknown");
        break;
    case efyj::status::dexi_parser_option_conversion_error:
        py::print("dexi parser option conversion error");
        break;
    case efyj::status::csv_parser_file_error:
        py::print("csv parser file error");
        break;
    case efyj::status::csv_parser_column_number_incorrect:
        py::print("csv parser column number incorrect");
        break;
    case efyj::status::csv_parser_scale_value_unknown:
        py::print("csv_parser scale value unknown");
        break;
    case efyj::status::csv_parser_column_conversion_failure:
        py::print("csv parser column conversion failure");
        break;
    case efyj::status::csv_parser_basic_attribute_unknown:
        py::print("csv parser basic attribute unknown");
        break;
    case efyj::status::unknown_error:
        py::print("unknown error");
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

    m.def(
      "information",
      [](const std::string& s) {
          efyj::information_results out;
          const auto ret = efyj::static_information(s, out);
          if (ret != efyj::status::success)
              show_error_message(ret);
          return ret;
      },
      R"pbdoc(
        Retrieves information from a DEXi file.
        Use this information to build correct scale values vector.
    )pbdoc");

    m.def(
      "adjustment",
      [](const std::string& model_file_path,
         const std::vector<std::string>& simulations,
         const std::vector<std::string>& places,
         const std::vector<int> departments,
         const std::vector<int> years,
         const std::vector<int> observed,
         const std::vector<int>& scale_values) {
          std::vector<efyj::result> out;

          const auto ret = efyj::static_adjustment(
            model_file_path,
            simulations,
            places,
            departments,
            years,
            observed,
            scale_values,
            [&out](const std::vector<efyj::result>& r) {
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

          if (ret != efyj::status::success)
              show_error_message(ret);

          return out;
      },
      R"pbdoc(
        Compute adjustment of a DEXi file.
    )pbdoc");

    m.def(
      "prediction",
      [](const std::string& model_file_path,
         const std::vector<std::string>& simulations,
         const std::vector<std::string>& places,
         const std::vector<int> departments,
         const std::vector<int> years,
         const std::vector<int> observed,
         const std::vector<int>& scale_values) {
          std::vector<efyj::result> out;

          const auto ret = efyj::static_prediction(
            model_file_path,
            simulations,
            places,
            departments,
            years,
            observed,
            scale_values,
            [&out](const std::vector<efyj::result>& r) {
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

          if (ret != efyj::status::success)
              show_error_message(ret);

          return out;
      },
      R"pbdoc(
        Compute prediction of a DEXi file.
    )pbdoc");
}