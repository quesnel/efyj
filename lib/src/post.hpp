/* Copyright (C) 2015-2016 INRA
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

#ifndef INRA_EFYj_POST_HPP
#define INRA_EFYj_POST_HPP

#include <EASTL/vector.h>

#include <fmt/color.h>
#include <fmt/format.h>

#include <efyj/efyj.hpp>

#include <cmath>

namespace efyj {

template<typename T>
void
print(const matrix<T>& m)
{
    using size_type = typename matrix<T>::size_type;

    const size_type rows = m.rows();
    const size_type cols = m.columns();

    for (size_type r = { 0 }; r != rows; ++r) {
        for (size_type c = { 0 }; c != cols; ++c) {
            fmt::print("{:>10} ", m(r, c));
        }
        fmt::print("\n");
    }
    fmt::print("\n");
}

inline double
rmsep(const eastl::vector<int>& observated,
      const eastl::vector<int>& simulated,
      const size_t N,
      const size_t NC)
{
    matrix<int> mat(NC, NC, 0);

    for (size_t i = 0, e = observated.size(); i != e; ++i)
        mat(observated[i], simulated[i])++;

    int sum = { 0 };

    for (size_t i = 0; i != NC; ++i)
        for (size_t j = 0; j != NC; ++j)
            sum += mat(i, j) * ((i - j) * (i - j));

    return sqrt((double)sum / (double)N);
}

inline double
mult_and_sum(const matrix<double>& m1, const matrix<double>& m2)
{
    // Do not compute a matrix multiplication but a coefficient multiplication.
    //
    // matrix<double> product(m1.rows(), m2.columns(), 0.0);
    //
    // assert(m1.rows() == m2.columns());
    // assert(m1.columns() == m2.rows());
    //
    // for (size_t row = 0; row != m1.rows(); ++row) {
    //     for (size_t col = 0; col != m2.columns(); ++col) {
    //         fmt::print("x({},{}) =\n", row, col);
    //         for (size_t i = 0; i != m1.columns(); ++i) {
    //             product(row, col) += m1(row, i) * m2(i, col);
    //         }
    //     }
    // }
    //
    // double ret = { 0 };
    // for (auto& elem : product)
    //     ret += elem;

    double ret = { 0 };

    for (size_t row = 0; row != m1.rows(); ++row)
        for (size_t col = 0; col != m2.columns(); ++col)
            ret += m1(row, col) * m2(row, col);

    return ret;
}

/**
 * @details The @e weighted_kappa_calculator structure is used to reduce
 *     allocation/reallocation/release in comparison with the global functions
 *     @e squared_weighted_kappa and \e linear_weighted_kappa when no vector or
 *         matrix resizing is necessary.
 */
class weighted_kappa_calculator
{
public:
    weighted_kappa_calculator(size_t NC_)
      : observed(NC_, NC_)
      , distributions(NC_, 2)
      , expected(NC_, NC_)
      , weighted(NC_, NC_)
      , NC(NC_)
    {
        assert(NC_ > 0 && "weighted_kappa_calculator bad parameter");
    }

    double linear(const eastl::vector<int>& observated,
                  const eastl::vector<int>& simulated) noexcept
    {
        pre(observated, simulated);

        for (int i = 0; i != (int)NC; ++i)
            for (int j = 0; j != (int)NC; ++j)
                weighted(i, j) = std::abs(i - j);

        return post();
    }

    double squared(const eastl::vector<int>& observated,
                   const eastl::vector<int>& simulated) noexcept
    {
        pre(observated, simulated);

        for (int i = 0; i != NC; ++i)
            for (int j = 0; j != NC; ++j)
                weighted(i, j) = std::abs(i - j) * std::abs(i - j);

        return post();
    }

private:
    matrix<double> observed;
    matrix<double> distributions;
    matrix<double> expected;
    matrix<double> weighted;
    const int NC;

    void pre(const eastl::vector<int>& observated,
             const eastl::vector<int>& simulated) noexcept
    {
        assert(observated.size() == simulated.size() &&
               "weighted_kappa_calculator observated and simulated sizes "
               "are different");

        eastl::fill(observed.begin(), observed.end(), 0.0);
        eastl::fill(distributions.begin(), distributions.end(), 0.0);

        for (int i = 0; i != (int)simulated.size(); ++i) {
            ++observed(observated[i], simulated[i]);
            ++distributions(observated[i], 0);
            ++distributions(simulated[i], 1);
        }

        for (auto& elem : observed)
            elem /= (double)simulated.size();

        for (auto& elem : distributions)
            elem /= (double)simulated.size();

        for (int i = 0; i != (int)NC; ++i)
            for (int j = 0; j != (int)NC; ++j)
                expected(i, j) = distributions(i, 0) * distributions(j, 1);
    }

    double post() noexcept
    {
        auto sum_expected = mult_and_sum(weighted, expected);
        if (sum_expected)
            return 1.0 - (mult_and_sum(weighted, observed) / sum_expected);

        return 1.0;
    }
};
}

#endif
