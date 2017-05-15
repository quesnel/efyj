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

#include <Eigen/Core>
#include <vector>

namespace efyj {

inline double
rmsep(const std::vector<int>& observated,
      const std::vector<int>& simulated,
      const std::size_t N,
      const std::size_t NC)
{
    Eigen::ArrayXXi matrix = Eigen::ArrayXXi::Zero(NC, NC);

    for (std::size_t i = 0, e = observated.size(); i != e; ++i)
        matrix(observated[i], simulated[i])++;

    unsigned long sum = 0.0;

    for (std::size_t i = 0; i != NC; ++i)
        for (std::size_t j = 0; j != NC; ++j)
            sum += matrix(i, j) * ((i - j) * (i - j));

    return std::sqrt((double)sum / (double)N);
}

inline double
linear_weighted_kappa(const std::vector<int>& observated,
                      const std::vector<int>& simulated,
                      const std::size_t N,
                      const std::size_t NC)
{
    Eigen::ArrayXXd observed = Eigen::ArrayXXd::Zero(NC, NC);
    Eigen::ArrayX2d distributions = Eigen::ArrayXXd::Zero(NC, 2);

    for (int i = 0; i != (int)N; ++i) {
        ++observed(observated[i], simulated[i]);
        ++distributions(observated[i], 0);
        ++distributions(simulated[i], 1);
    }

    observed /= (double)N;
    distributions /= (double)N;
    Eigen::ArrayXXd expected = Eigen::ArrayXXd::Zero(NC, NC);

    for (int i = 0; i != (int)NC; ++i)
        for (int j = 0; j != (int)NC; ++j)
            expected(i, j) = distributions(i, 0) * distributions(j, 1);

    Eigen::ArrayXXd weighted = Eigen::ArrayXXd(NC, NC);

    for (int i = 0; i != (int)NC; ++i)
        for (int j = 0; j != (int)NC; ++j)
            weighted(i, j) = std::abs(i - j);

    return 1.0 - ((weighted * observed).sum() / (weighted * expected).sum());
}

inline double
squared_weighted_kappa(const std::vector<int>& observated,
                       const std::vector<int>& simulated,
                       const std::size_t N,
                       const std::size_t NC)
{
    Eigen::ArrayXXd observed = Eigen::ArrayXXd::Zero(NC, NC);
    Eigen::ArrayX2d distributions = Eigen::ArrayXXd::Zero(NC, 2);

    for (int i = 0; i != (int)N; ++i) {
        ++observed(observated[i], simulated[i]);
        ++distributions(observated[i], 0);
        ++distributions(simulated[i], 1);
    }

    observed /= (double)N;
    distributions /= (double)N;
    Eigen::ArrayXXd expected = Eigen::ArrayXXd::Zero(NC, NC);

    for (int i = 0; i != (int)NC; ++i)
        for (int j = 0; j != (int)NC; ++j)
            expected(i, j) = distributions(i, 0) * distributions(j, 1);

    Eigen::ArrayXXd weighted = Eigen::ArrayXXd(NC, NC);

    for (int i = 0; i != (int)NC; ++i)
        for (int j = 0; j != (int)NC; ++j)
            weighted(i, j) = std::abs(i - j) * std::abs(i - j);

    return 1.0 - ((weighted * observed).sum() / (weighted * expected).sum());
}

/** The \e weighted_kappa_calculator structure is used to reduce
 * allocation/reallocation/release in comparison with the global functions
 * \e squared_weighted_kappa and \e linear_weighted_kappa when no vector
 * or matrix resizing is necessary.
 */
class weighted_kappa_calculator
{
public:
    weighted_kappa_calculator(std::size_t NC_)
      : observed(Eigen::ArrayXXd::Zero(NC_, NC_))
      , distributions(Eigen::ArrayXXd::Zero(NC_, 2))
      , expected(Eigen::ArrayXXd::Zero(NC_, NC_))
      , weighted(Eigen::ArrayXXd(NC_, NC_))
      , NC(NC_)
    {
        assert(NC_ > 0 and "weighted_kappa_calculator bad parameter");
    }

    double linear(const std::vector<int>& observated,
                  const std::vector<int>& simulated) noexcept
    {
        pre(observated, simulated);

        for (int i = 0; i != (int)NC; ++i)
            for (int j = 0; j != (int)NC; ++j)
                weighted(i, j) = std::abs(i - j);

        return post();
    }

    double squared(const std::vector<int>& observated,
                   const std::vector<int>& simulated) noexcept
    {
        pre(observated, simulated);

        for (int i = 0; i != NC; ++i)
            for (int j = 0; j != NC; ++j)
                weighted(i, j) = std::abs(i - j) * std::abs(i - j);

        return post();
    }

private:
    Eigen::ArrayXXd observed;
    Eigen::ArrayX2d distributions;
    Eigen::ArrayXXd expected;
    Eigen::ArrayXXd weighted;
    const int NC;

    void pre(const std::vector<int>& observated,
             const std::vector<int>& simulated) noexcept
    {
        assert(observated.size() == simulated.size() and
               "weighted_kappa_calculator observated and simulated sizes "
               "are different");

        observed.setZero();
        distributions.setZero();

        for (int i = 0; i != (int)simulated.size(); ++i) {
            ++observed(observated[i], simulated[i]);
            ++distributions(observated[i], 0);
            ++distributions(simulated[i], 1);
        }

        observed /= (double)simulated.size();
        distributions /= (double)simulated.size();

        for (int i = 0; i != (int)NC; ++i)
            for (int j = 0; j != (int)NC; ++j)
                expected(i, j) = distributions(i, 0) * distributions(j, 1);
    }

    double post() noexcept
    {
        return 1. -
               ((weighted * observed).sum() / (weighted * expected).sum());
    }
};
}

#endif
