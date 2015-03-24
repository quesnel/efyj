/* Copyright (C) 2014 INRA
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

#ifndef INRA_EFYj_POST_HPP
#define INRA_EFYj_POST_HPP

#include <efyj/model.hpp>
#include "options.hpp"

#include <Eigen/Core>
#include <ostream>
#include <cmath>

namespace efyj {

typedef std::function <void(const efyj::dexi&,
                            const Options&,
                            const std::size_t,
                            const std::size_t,
                            std::ostream&)> method_fn;

void rmsep(const efyj::dexi&, const Options& options,
           const std::size_t N, const std::size_t NC,
           std::ostream& os)
{
    Eigen::ArrayXXi matrix = Eigen::ArrayXXi::Zero(NC, NC);

    for (const auto& id : options.ids)
        matrix(id.observated, id.simulated)++;

    unsigned long sum = 0.0;
    for (std::size_t i = 0; i != NC; ++i)
        for (std::size_t j = 0; j != NC; ++j)
            sum += matrix(i, j) * ((i - j) * (i - j));

    os << "RMSE:\n"
       << "----\n"
       << std::sqrt((double)sum / (double)N)
       << "\n\n";
}

void weighted_kappa(const efyj::dexi&, const Options& options,
                    const std::size_t N, const std::size_t NC,
                    std::ostream& os)
{
    (void)N;
    Eigen::ArrayXXi matrix = Eigen::ArrayXXi::Zero(NC, NC);

    for (const auto& id : options.ids)
        matrix(id.observated, id.simulated)++;

    auto pi_i = matrix.colwise().sum();
    auto pi_j = matrix.rowwise().sum();

    os << "confusion matrix:\n"
       << "----------------\n"
       << matrix
       << "\n\n"
       << "n_i+:\n" << pi_i << "\n"
       << "n_+j:\n" << pi_j << "\n"
       << "\n\n";

    double qfirst = 0.0;
    double qsecond = 0.0;
    double lfirst = 0.0;
    double lsecond = 0.0;
    for (int i = 0; i != (int)NC; ++i) {
        for (int j = 0; j != (int)NC; ++j) {
            double wijq = 1.0 - (((i - j) * (i - j)) / (double)((NC - 1) * (NC - 1.0)));
            double wijl = 1.0 - (std::abs(i - j) / (double)(NC - 1));

            qfirst += wijq * matrix(i, j);
            qsecond += wijq * pi_i(i) * pi_j(j);

            lfirst += wijl * matrix(i, j);
            lsecond += wijl * pi_i(i) * pi_j(j);
        }
    }

    os << "K_qw:\n"
       << "--\n"
       << ((qfirst - qsecond) / (1 - qsecond))
       << "\n\n"
       << "K_lw:\n"
       << "--\n"
       << ((lfirst - lsecond) / (1 - lsecond))
       << "\n\n";
};

struct Post
{
    void apply(const efyj::dexi& model, const Options& options,
               std::ostream& os);

    std::vector <method_fn> functions;
};

void Post::apply(const efyj::dexi& model, const Options& options,
                 std::ostream& os)
{
    const std::size_t n = options.options.rows();
    const std::size_t nc = model.child->scale.size();

    for (auto fn : functions)
        fn(model, options, n, nc, os);
}

}

#endif
