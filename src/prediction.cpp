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

#include "problem.hpp"
#include "solver-stack.hpp"
#include "model.hpp"
#include "context.hpp"
#include "exception.hpp"
#include "utils.hpp"
#include "options.hpp"
#include "post.hpp"
#include <iterator>
#include <fstream>
#include <chrono>
#include <thread>

namespace efyj {

void prediction_0(std::shared_ptr<Context> context,
                  const Model& model, const Options& options)
{
    context->info() << "Prediction started\n";

    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    std::vector <int> simulated(options.observated.size(), 0);
    std::vector <solver_details::line_updater> bestupdaters;

    solver_details::for_each_model_solver solver(context, model);
    solver.reduce(options);

    int walker_number = solver.get_max_updaters();

    for (int step = 1; step < walker_number; ++step) {
        start = std::chrono::system_clock::now();
        std::tuple <unsigned long, double> best {0, 0};
        unsigned long long int number_bestkappa = 0;

        do {
            auto it = options.ordered.cbegin();
            while (it != options.ordered.cend()) {
                auto id = it->first;

                std::fill(simulated.begin(), simulated.end(), 0);
                for (; it != options.ordered.cend() && it->first == id; ++it)
                    simulated[it->second] = solver.solve(options.options.row(
                                                             it->second));

                auto ret = squared_weighted_kappa(
                    options.observated,
                    simulated,
                    options.options.rows(),
                    model.attributes[0].scale.size());

                if (ret > std::get<1>(best)) {
                    number_bestkappa = 0;
                    std::get<1>(best) = ret;
                    bestupdaters = solver.updaters();

                    context->info() << "  - best kappa found: "
                                   << std::get<1>(best)
                                   << '\n';

                } else if (ret == std::get<1>(best)) {
                    number_bestkappa++;
                }

                ++std::get<0>(best);
            }
        } while (solver.next() == true);

        end = std::chrono::system_clock::now();

        context->info() << "- " << step
                        << " / kappa: " << std::get<1>(best)
                        << " / loop: " << std::get<0>(best)
                        << " / updaters: ";

        for (const auto& upd : bestupdaters)
            context->info() << '[' << upd.attribute << ',' << upd.line << ']';

        context->info() << " / time: "
                        << std::chrono::duration<double>(end - start).count()
                        << "s\n";

        //
        // TODO: be carefull, solver.init can throw when end of
        // computation is reached.
        //

        solver.init(step + 1);
    }
}

} // namespace efyj
