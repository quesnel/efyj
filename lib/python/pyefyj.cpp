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

    efyj::context ctx;

    ctx.dexi_cb = [](const efyj::status s,
                     int line,
                     int column,
                     const std::string_view tag) {
        py::print("DEXi error: ",
                  get_error_message(s),
                  " at line ",
                  line,
                  " column ",
                  column,
                  "with value: ",
                  tag);
    };

    ctx.csv_cb = [](const efyj::status s, int line, int column) {
        py::print("CSV error: ",
                  get_error_message(s),
                  " at line ",
                  line,
                  " column ",
                  column);
    };

    ctx.eov_cb = []() { py::print("Not enough memory to continue"); };
    ctx.cast_cb = []() { py::print("Internal error: cast failure"); };
    ctx.solver_cb = []() { py::print("Solver error"); };
    ctx.file_cb = [](const std::string_view file_name) {
        py::print("Error to access file `", file_name, "`");
    };

    m.def(
      "information",
      [&ctx](const std::string& s) -> efyj::information_results {
          efyj::information_results out;
          if (efyj::static_information(ctx, s, out) != efyj::status::success)
              py::print("information(...) failed");
          return out;
      },
      R"pbdoc(
        Retrieves information from a DEXi file.
        Use this information to build correct scale values vector.
    )pbdoc");

    m.def(
      "adjustment",
      [&ctx](
        const std::string& model_file_path,
        const std::vector<std::string>& simulations,
        const std::vector<std::string>& places,
        const std::vector<int> departments,
        const std::vector<int> years,
        const std::vector<int> observed,
        const std::vector<int>& scale_values) -> std::vector<efyj::result> {
          std::vector<efyj::result> out;
          const auto ret = efyj::static_adjustment(
            ctx,
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
              py::print("adjustment failed");

          return out;
      },
      R"pbdoc(
        Compute adjustment of a DEXi file.
    )pbdoc");

    m.def(
      "prediction",
      [&ctx](const std::string& model_file_path,
             const std::vector<std::string>& simulations,
             const std::vector<std::string>& places,
             const std::vector<int> departments,
             const std::vector<int> years,
             const std::vector<int> observed,
             const std::vector<int>& scale_values) {
          std::vector<efyj::result> out;

          const auto ret = efyj::static_prediction(
            ctx,
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
              py::print("prediction failed");

          return out;
      },
      R"pbdoc(
        Compute prediction of a DEXi file.
    )pbdoc");
}