/* Copyright (C) 2015 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <efyj/model.hpp>
#include <efyj/exception.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>

#pragma GCC diagnostic ignored "-Wdeprecated-register"
#include <Eigen/Core>
#include <Eigen/Eigen>

namespace efyj {

Eigen::ArrayXi csv_reader(const dexi& model, const std::string& filepath)
{
    std::ifstream ifs(filepath);
    if (not ifs)
        throw csv_parser_error(filepath, "can not open");

    Eigen::MatrixXi options(1u, model.basic_attribute_scale_size.size());
    std::size_t row = 0;
    std::string line;
    std::vector <std::string> buffer(model.basic_attribute_scale_size.size(),
                                     std::string());

    while (true) {
        std::getline(ifs, line);

        if (not ifs)
            break;

        boost::algorithm::split(buffer, line, boost::algorithm::is_any_of(";"));
        if (buffer.size() != model.basic_attribute_scale_size.size() + 1u)
            throw csv_parser_error(row + 1, filepath, "size problem, csv number of column"
                                   " must be equal to basic attribute number plus one for"
                                   " results");

        options.conservativeResize(options.rows(), options.cols() + 1u);

        for (std::size_t i = 0, e = buffer.size();  i != e; ++i) {
            try {
                // TODO this attributes[i] is wrong since we needs only
                // scale value for atomic attribute.
                options(i, row) = model.attributes[i].scale.find_scale_value(buffer[i]);
            } catch (const efyj_error& e) {
                throw csv_parser_error(row, i, filepath, e.what());
            }
        }
    }

    return std::move(options);
}

}
