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

#ifndef INRA_EFYj_OPTIONS_HPP
#define INRA_EFYj_OPTIONS_HPP

#include "model.hpp"

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#endif
#include <Eigen/src/Core/util/DisableStupidWarnings.h>
#include <Eigen/Core>

namespace efyj {

struct OptionsId
{
    OptionsId(const std::string& simulation_,
              const std::string& place_,
              int department_,
              int year_,
              int observated_)
        : simulation(simulation_)
        , place(place_)
        , department(department_)
        , year(year_)
        , observated(observated_)
        , simulated(-1)
    {}

    std::string simulation;
    std::string place;
    int department;
    int year;

    int observated;
    int simulated;
};

typedef Eigen::ArrayXXi ArrayOptions;

struct Options
{
    std::vector <OptionsId> ids;
    ArrayOptions options;
};

Options array_options_read(std::istream& is, const efyj::dexi& model);

}

#endif
