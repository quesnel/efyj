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
#include <mutex>

namespace {

std::string
make_new_name(const std::string& filepath, unsigned int thread_id) noexcept
{
    if (filepath.empty()) {
        std::ostringstream os;
        os << "worker-" << thread_id << ".log";
        return os.str();
    }

    auto dotposition = filepath.find_last_of('.');

    if (dotposition == 0u) {
        std::ostringstream os;
        os << "worker-" << thread_id << ".log";
        return os.str();
    }

    if (dotposition == filepath.size() - 1) {
        std::ostringstream os;
        os << filepath.substr(0, dotposition)
           << '-' << thread_id;

        return os.str();
    }

    std::ostringstream os;
    os << filepath.substr(0, dotposition)
       << '-' << thread_id
       << filepath.substr(dotposition + 1);

    return os.str();

}

}

namespace efyj {

class Results
{
    std::shared_ptr<Context> m_context;
    std::mutex m_container_mutex;

    struct Result {
        double kappa;
        unsigned long loop;
        std::vector <std::pair<int, int>> updaters;
    };

    std::vector<Result> m_results;
    std::vector<int> m_level;
    const unsigned int m_threads;
    std::chrono::time_point<std::chrono::system_clock> m_start, m_end;

public:
    Results(std::shared_ptr<Context> context, unsigned int threads)
        : m_context(context)
        , m_threads(threads)
        , m_start(std::chrono::system_clock::now())
    {
        m_results.reserve(32);
        m_results.resize(1u);

        m_level.reserve(32);
        m_level.resize(1u);
        m_level[0] = threads;
    }

    void emplace_result(int i, double kappa,
                        unsigned long loop,
                        const std::vector <solver_details::line_updater>& updaters)
    {
        assert(static_cast<std::size_t>(i) < m_level.size() and
               static_cast<std::size_t>(i) < m_results.size());

        m_results[i].kappa = kappa;
        m_results[i].loop += loop;
        m_results[i].updaters.resize(updaters.size());
        m_results[i].updaters.clear();

        for (const auto& x : updaters)
            m_results[i].updaters.emplace_back(x.attribute, x.line);
    }

    void push(int step, double kappa, unsigned long loop,
              const std::vector <solver_details::line_updater>& updaters)
    {
        std::lock_guard <std::mutex> locker(m_container_mutex);

        assert(step >= 1);

        if (static_cast<std::size_t>(step - 1) >= m_results.size()) {
            /* A new level (threshold) is reached we append new elements
               to the level and results vectors. */
            m_level.emplace_back(static_cast<int>(m_threads - 1));
            m_results.emplace_back();

            emplace_result(m_results.size() - 1, kappa, loop, updaters);
            m_level[m_results.size()]--;
        } else {
            if (m_results[step - 1].kappa < kappa)
                emplace_result(step - 1, kappa, loop, updaters);
            else
                m_results[step - 1].loop += loop;

            m_level[step - 1]--;
        }

        if (m_level[step - 1] <= 0) {
            m_end = std::chrono::system_clock::now();
            auto duration = std::chrono::duration<double>(m_end - m_start).count();

            m_context->info().printf("- %d kappa: %f / loop: %" PRIuMAX
                                     " / time: %fs / updaters: ",
                                     step,
                                     m_results[step - 1].kappa,
                                     m_results[step - 1].loop,
                                     duration);

            for (const auto& x : m_results[step - 1].updaters)
                m_context->info() << '[' << x.first << ',' << x.second << ']';

            m_context->info()  << '\n';
        }
    }
};

void parallel_prediction_worker(std::shared_ptr<Context> context,
                                const Model& model,
                                const Options& options,
                                const std::vector<int>& ids,
                                const bool& stop,
                                Results& results)
{
    context->info() << "Parallel prediction worker starts\n";

    std::vector <int> simulated(options.observated.size(), 0);
    std::vector <solver_details::line_updater> bestupdaters;

    solver_details::for_each_model_solver solver(context, model);
    solver.reduce(options, ids);
    int walker_number = solver.get_max_updaters();

    for (int step = 1; step < walker_number; ++step) {
        unsigned long loop = 0;
        double kappa = 0;
        unsigned long long int number_bestkappa = 0;

        do {
            for (auto it = ids.cbegin(); it != ids.cend(); ++it) {
                std::fill(simulated.begin(), simulated.end(), 0);

                auto bounds = options.ordered.equal_range(*it);

                for (auto jt = bounds.first; jt != bounds.second; ++jt)
                    simulated[jt->second] = solver.solve(
                        options.options.row(jt->second));

                auto ret = squared_weighted_kappa(
                    options.observated,
                    simulated,
                    options.options.rows(),
                    model.attributes[0].scale.size());

                if (ret > kappa) {
                    number_bestkappa = 0;
                    kappa = ret;
                    bestupdaters = solver.updaters();
                } else if (ret == kappa) {
                    number_bestkappa++;
                }

                loop++;
            }

            if (stop)
                return;

        } while (solver.next() == true);

        context->err() << "Send kappa: " << kappa << " " << loop << '\n';

        results.push(step, kappa, loop, bestupdaters);

        bestupdaters.clear();

        //
        // TODO: be carefull, solver.init can throw when end of
        // computation is reached.
        //

        solver.init(step + 1);
    }
}

void prediction_n(std::shared_ptr<Context> context,
                  const Model& model,
                  const Options& options,
                  unsigned int threads)
{
    context->info() << "Parallelized Prediction started\n"
                    << "- Threads: "
                    << threads
                    << '\n';

    assert(threads > 1);

    std::vector<std::vector<int>> jobs(threads);
    auto sz = options.observated.size();
    auto sz_div_threads = sz / threads;
    auto sz_mod_threads = sz % threads;

    auto id = 0u;
    for (auto& x : jobs)
        for (auto y = 0u; y != sz_div_threads; ++y)
            x.push_back(id++);

    for (auto y = 0u; y != sz_mod_threads; ++y)
        jobs[y].push_back(id++);

    assert(id == sz);

    Results results(context, threads);
    bool stop = false;

    std::vector<std::thread> workers { threads };

    for (auto i = 0u; i != threads; ++i) {
        auto filepath = ::make_new_name(context->get_log_filename(), i);
        auto new_ctx = std::make_shared<efyj::Context>(context->log_priority());
        auto ret = new_ctx->set_log_file_stream(filepath);

        if (not ret)
            context->err().printf("Failed to assign '%s' to thread %d. Switch "
                                  "to console.\n", filepath.c_str(), i);


        workers[i] = std::thread(parallel_prediction_worker,
                                 new_ctx,
                                 std::cref(model),
                                 std::cref(options),
                                 std::cref(jobs[i]),
                                 std::cref(stop),
                                 std::ref(results));
    }

    /* Here try to read outputs of compuation from prediction workers. */
    /* TODO what use? */

    /* Stop work and wait for all prediction workers. */
    /* TODO stop = true; */

    for (auto& w : workers)
        w.join();
}

} // namespace efyj