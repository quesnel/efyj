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
#include <efyj/options.hpp>

#include <Eigen/Core>
#include <cmath>

namespace efyj {

typedef std::function <void(const efyj::Model &,
                            const Options &,
                            const std::size_t,
                            const std::size_t,
                            Context)> method_fn;

void rmsep(const efyj::Model &, const Options &options,
           const std::size_t N, const std::size_t NC,
           Context ctx)
{
    Eigen::ArrayXXi matrix = Eigen::ArrayXXi::Zero(NC, NC);

    for (const auto &id : options.ids)
        matrix(id.observated, id.simulated)++;

    unsigned long sum = 0.0;

    for (std::size_t i = 0; i != NC; ++i)
        for (std::size_t j = 0; j != NC; ++j)
            sum += matrix(i, j) * ((i - j) * (i - j));

    double result = std::sqrt((double)sum / (double)N);
    efyj_info(ctx, boost::format("RMSE: %1%") % result);
}

void weighted_kappa(const efyj::Model &, const Options &options,
                    const std::size_t N, const std::size_t NC,
                    Context ctx)
{
    Eigen::ArrayXXd observed = Eigen::ArrayXXd::Zero(NC, NC);
    Eigen::ArrayX2d distributions = Eigen::ArrayXXd::Zero(NC, 2);

    for (int i = 0; i != (int)N; ++i) {
        ++observed(options.ids[i].observated, options.ids[i].simulated);
        ++distributions(options.ids[i].observated, 0);
        ++distributions(options.ids[i].simulated, 1);
    }

    efyj_info(ctx, "Confusion matrix");
    efyj_info(ctx, observed);
    observed /= (double)N;
    distributions /= (double)N;
    Eigen::ArrayXXd expected = Eigen::ArrayXXd::Zero(NC, NC);

    for (int i = 0; i != (int)NC; ++i)
        for (int j = 0; j != (int)NC; ++j)
            expected(i, j) = distributions(i, 0) * distributions(j, 1);

    Eigen::ArrayXXd weighted = Eigen::ArrayXXd(NC, NC);
    {
        for (int i = 0; i != (int)NC; ++i)
            for (int j = 0; j != (int)NC; ++j)
                weighted(i, j) = std::abs(i - j); // * std::abs(i - j);

        double kapa = 1.0 - ((weighted * observed).sum() /
                             (weighted * expected).sum());
        efyj_info(ctx, boost::format("kapa linear: %1%") % kapa);
    }
    {
        for (int i = 0; i != (int)NC; ++i)
            for (int j = 0; j != (int)NC; ++j)
                weighted(i, j) = std::abs(i - j) * std::abs(i - j);

        double kapa = 1.0 - ((weighted * observed).sum() /
                             (weighted * expected).sum());
        efyj_info(ctx, boost::format("kapa squared: %1%") % kapa);
    }
};

struct Post {
    void apply(const efyj::Model &model, const Options &options, Context os);

    std::vector <method_fn> functions;
};

void Post::apply(const efyj::Model &model, const Options &options, Context os)
{
    const std::size_t n = options.options.rows();
    const std::size_t nc = model.attributes[0].scale.size();

    for (auto fn : functions)
        fn(model, options, n, nc, os);
}

}

#endif
