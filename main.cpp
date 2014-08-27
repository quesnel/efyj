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

#include "log.hpp"
#include "model.hpp"
#include "problem.hpp"
#include "solver.hpp"
#include "print.hpp"
#include "utils.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iterator>
#include <random>
#include <cstdlib>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <mpi.h>

namespace {

void usage() noexcept
{
    std::cout << "efyj [-h|--help] [-s|--stress [number]] [files...]\n\n"
              << "Options:\n"
              << "    -h, --help            This help message\n"
              << "    -s, --stress [int]    Stress mode (check all solver/cache)\n"
              << "                          If no argument, the default"
              << "                          make 100000 run\n"
              << "    -l, --list-solver     Show the list of solver and"
              << "                          cache\n"
              << "    -c, --select [name]   Select the specified solver\n"
              << "    -o str                Add option to run solver\n"
              << "    -m str str            Compute all models and run with\n"
              << "                          options csv file\n"
              << "    [files...]            DEXi file to run\n"
              << std::endl;
}

void show_solver() noexcept
{
    std::cout << "available solver:\n"
              << " classic            Default solver\n"
              << " hash               classic solver, hash table and"
              <<  "                   integer vector options\n"
              << " hash_string        classic solver, hash table and"
              << "                    string options\n"
              << " bigmem             classic solver, big memory cache"
              << "                    and integer vector options\n"
              << " bigmem_integer     classic solver, big memory cache"
              << "                    and integer options"
              << std::endl;
}

void show_model(const efyj::dexi& model)
{
    std::cout << "(attribute: " << model.attributes.size()
              << ", basic attribute: " << model.basic_scale_number
              << ", problem size: " << model.problem_size
              << ")" << std::endl;
}

std::ostream& operator<<(std::ostream& os, const efyj::result_type& rt)
{
    std::copy(rt.cbegin(), rt.cend(),
              std::ostream_iterator <efyj::scale_id>(os, " "));

    return os;
}

void process(const std::string& filepath,
             const std::map <std::string, bool>& solvers,
             const std::vector <std::string>& options)
{
    efyj::dexi model;
    std::ifstream is(filepath);
    if (not is)
        throw std::invalid_argument(
            efyj::stringf("unknown file %s", filepath.c_str()));

    is >> model;

    show_model(model);

    std::cout << "Results:\n";
    for (const auto& slv : solvers) {
        if (slv.second)
            std::cout << " " << slv.first << ": ";

        if (slv.first == "classic" && slv.second) {
            efyj::solver_basic si(model);
            for (const auto& opt : options)
                std::cout << si.solve(opt) << "\n";
        } else if (slv.first == "hash" && slv.second) {
            efyj::solver_hash sh(model);
            for (const auto& opt : options)
                std::cout << sh.solve(opt) << "\n";
        } else if (slv.first == "hash_string" && slv.second) {
            efyj::solver_hash sh(model);
            for (const auto& opt : options)
                std::cout << sh.solve(opt) << "\n";
        } else if (slv.first == "bigmem" && slv.second) {
            efyj::solver_bigmem sbm(model);
            for (const auto& opt : options)
                std::cout << sbm.solve(opt) << "\n";
        } else if (slv.first == "bigmem_integer" && slv.second) {
            efyj::solver_bigmem sbm(model);
            for (const auto& opt : options)
                std::cout << sbm.solve(opt) << "\n";
        }
    }
}

bool process_models(const std::string& dexi_filepath,
                    const std::string& option_filepath,
                    int argc, char *argv[])
{
    bool ret = true;

    MPI::Init(argc, argv);
    int world_size = MPI::COMM_WORLD.Get_size();
    int rank = MPI::COMM_WORLD.Get_rank();

//    efyj::log a("output", rank);
    efyj::scope_exit finalize([]() { MPI::Finalize(); });

    try {
        efyj::logf("efyj::problem: open files: ");
        efyj::problem pb(dexi_filepath, option_filepath);
        efyj::logf("ok");

        efyj::logf("efyj::problem: solve: ");
        pb.solve(rank, world_size);
        efyj::logf("ok");
    } catch (const std::exception& e) {
        efyj::logf("failed: %s", e.what());
        ret = false;
    }

    return ret;
}

void generate_new_options(std::vector <efyj::scale_id>& options,
                          const std::vector <std::size_t>& high_level,
                          std::mt19937& rng)
{
    std::uniform_int_distribution<int> distribution(0, 9);

    for (std::size_t i = 0, e = options.size(); i != e; ++i)
        options[i] = distribution(rng) % high_level[i];
}

void generate_new_options(std::string& options,
                          const std::vector <std::size_t>& high_level,
                          std::mt19937& rng)
{
    std::uniform_int_distribution<int> distribution(0, 9);
    options.assign(high_level.size(), '0');

    for (std::size_t i = 0, e = high_level.size(); i != e; ++i)
        options[i] += (distribution(rng) % high_level[i]);
}

void generate_new_options(std::size_t& options,
                          const std::vector <std::size_t>& high_level,
                          std::mt19937& rng)
{
    std::uniform_int_distribution<int> distribution(0, 9);

    options = 0;
    std::size_t indice = 0;

    for (std::size_t i = 0, e = high_level.size(); i != e; ++i) {
        options += ((distribution(rng) % high_level[i]) << indice);
        indice += std::floor(std::log2(high_level[i]) + 1);
    }
}

void process_stress_test(const std::string& filepath,
                         unsigned long run_number)
{
    std::vector <std::chrono::duration <double>> time_result(6);
    std::chrono::time_point <std::chrono::system_clock> start;
    efyj::dexi model;

    {
        start = std::chrono::system_clock::now();

        std::ifstream is(filepath);
        if (not is)
            throw std::invalid_argument(
                efyj::stringf("unknown file %s", filepath.c_str()));

        is >> model;

        time_result[0] = std::chrono::system_clock::now() - start;
    }

    show_model(model);

    std::vector <std::size_t> high_level(model.basic_scale_number);
    int i = 0;
    for (const auto& att : model.attributes)
        if (att.children.empty())
            high_level[i++] = att.scale_size();

    {
        start = std::chrono::system_clock::now();

        std::vector <efyj::scale_id> options(model.basic_scale_number, 0u);
        efyj::solver_basic si(model);
        std::mt19937 generator;

        for (unsigned long i = 0; i < run_number; ++i) {
            ::generate_new_options(options, high_level, generator);
            si.solve(options);
        }

        time_result[1] = std::chrono::system_clock::now() - start;
    }

    {
        start = std::chrono::system_clock::now();

        std::vector <efyj::scale_id> options(model.basic_scale_number, 0u);
        efyj::solver_hash sh(model);
        std::mt19937 generator;

        for (unsigned long i = 0; i < run_number; ++i) {
            ::generate_new_options(options, high_level, generator);
            sh.solve(options);
        }

        time_result[2] = std::chrono::system_clock::now() - start;
    }

    {
        start = std::chrono::system_clock::now();

        std::string options('0', model.basic_scale_number);
        efyj::solver_hash sh(model);
        std::mt19937 generator;

        for (unsigned long i = 0; i < run_number; ++i) {
            ::generate_new_options(options, high_level, generator);
            sh.solve(options);
        }

        time_result[3] = std::chrono::system_clock::now() - start;
    }

    {
        start = std::chrono::system_clock::now();

        std::vector <efyj::scale_id> options(model.basic_scale_number, 0u);
        efyj::solver_bigmem sbm(model);
        std::mt19937 generator;

        for (unsigned long i = 0; i < run_number; ++i) {
            ::generate_new_options(options, high_level, generator);
            sbm.solve(options);
        }

        time_result[4] = std::chrono::system_clock::now() - start;
    }

    {
        start = std::chrono::system_clock::now();

        std::size_t options;
        efyj::solver_bigmem sbm(model);
        std::mt19937 generator;

        for (unsigned long i = 0; i < run_number; ++i) {
            ::generate_new_options(options, high_level, generator);
            sbm.solve(options);
        }

        time_result[5] = std::chrono::system_clock::now() - start;
    }

    auto min_max = std::minmax_element(time_result.begin() + 1, time_result.end(),
                                       [](std::chrono::duration <double>& a,
                                          std::chrono::duration <double>& b)
                                       {
                                           return a.count() < b.count();
                                       });

    std::cout << "Time elapsed to:\n"
              << "- read model........ : " << time_result[0].count() << "s\n"
              << "- get response for random " << run_number << " options:\n"
              << "  - basic solver.............. : "
              << ((time_result[1].count() == min_max.first->count()) ? dCYAN : ((time_result[1].count() == min_max.second->count()) ? dRED : ""))
              << time_result[1].count() << "s (" << (run_number / (1000000 * time_result[1].count())) << " million op/s)\n" << dNORMAL
              << "  - hash cache................ : "
              << ((time_result[2].count() == min_max.first->count()) ? dCYAN : ((time_result[2].count() == min_max.second->count()) ? dRED : ""))
              << time_result[2].count() << "s (" << (run_number / (1000000 * time_result[2].count())) << " million op/s)\n" << dNORMAL
              << "  - hash cache (string)....... : "
              << ((time_result[3].count() == min_max.first->count()) ? dCYAN : ((time_result[3].count() == min_max.second->count()) ? dRED : ""))
              << time_result[3].count() << "s (" << (run_number / (1000000 * time_result[3].count())) << " million op/s)\n" << dNORMAL
              << "  - big memory cache.......... : "
              << ((time_result[4].count() == min_max.first->count()) ? dCYAN : ((time_result[4].count() == min_max.second->count()) ? dRED : ""))
              << time_result[4].count() << "s (" << (run_number / (1000000 * time_result[4].count())) << " million op/s)\n" << dNORMAL
              << "  - big memory cache (binary). : "
              << ((time_result[5].count() == min_max.first->count()) ? dCYAN : ((time_result[5].count() == min_max.second->count()) ? dRED : ""))
              << time_result[5].count() << "s (" << (run_number / (1000000 * time_result[5].count())) << " million op/s)\n" << dNORMAL
              << "\n";
}
}

enum class cli_mode { solve, stress, compute_models };

int main(int argc, char *argv[])
{
//#if defined NDEBUG
    std::ios_base::sync_with_stdio(true);
//#endif

    cli_mode mode = cli_mode::solve;

    unsigned long stress_test_number = 100000;
    int ret = EXIT_SUCCESS;
    int i = 1;

    std::map <std::string, bool> solvers = {
        {"classic", false},
        {"hash", false}, {"hash_string", false}, {"bigmem", false},
        {"bigmem_integer", false}};

    std::vector <std::string> options;
    std::vector <std::string> files;

    while (i < argc) {
        if (not std::strcmp(argv[i], "--stress") or
            not std::strcmp(argv[i], "-s")) {
            mode = cli_mode::stress;
            i++;

            if (i < argc) {
                unsigned long nb = std::strtoul(argv[i], nullptr, 10);
                if (nb == 0 or errno)
                    i--;
                else
                    stress_test_number = nb;
            }

            i++;
            continue;
        }

        if (not std::strcmp(argv[i], "--models") or not std::strcmp(argv[i], "-m")) {
            if (i + 2 > argc)
                throw std::invalid_argument(
                    efyj::stringf("Missing argument for model"));

            return process_models(argv[i + 1], argv[i + 2], argc, argv);
        }

        if (not std::strcmp(argv[i], "-l") or
            not std::strcmp(argv[i], "--list-solver")) {
            show_solver();
            break;
        }

        if (not std::strcmp(argv[i], "--help") or
            not std::strcmp(argv[i], "-h")) {
            usage();
            break;
        }

        if (not std::strcmp(argv[i], "-c") or
            not std::strcmp(argv[i], "--select")) {
            i++;

            if (i == argc)
                throw std::invalid_argument(
                    efyj::stringf("Missing argument for `%s'", argv[i - 1]));

            auto it = solvers.find(argv[i]);
            if (it == solvers.end())
                throw std::invalid_argument(
                    efyj::stringf("Unknown solver `%s'", argv[i]));

            it->second = true;
            i++;
            continue;
        }

        if (not std::strcmp(argv[i], "-o")) {
            i++;

            if (i == argc)
                throw std::invalid_argument("Missing argument for `-o'");

            options.emplace_back(argv[i]);
            i++;
            continue;
        }

        files.emplace_back(argv[i]);
        i++;
    }

    try {
        switch(mode) {
        case cli_mode::stress:
            std::cout << "- run stress mode for "
                      << stress_test_number << " random run\n";
            for (const auto& file : files) {
                std::cout << "Processing [" << dYELLOW << file << dNORMAL << "] \n";
                process_stress_test(file, stress_test_number);
            }
            break;
        case cli_mode::solve:
            std::cout << "- run solver model\n";
            for (const auto& file : files) {
                std::cout << "Processing [" << dYELLOW << file << dNORMAL << "] \n";
                process(file, solvers, options);
            }
            break;
        case cli_mode::compute_models:
            break;
        }
        std::cout << "\n";
    } catch (const std::bad_alloc& e) {
        std::cerr << dRED << "fail to allocate memory: " << dNORMAL << e.what()
                  << std::endl;
        ret = EXIT_FAILURE;
    } catch (const std::invalid_argument& e) {
        std::cerr << dRED << "bad argument: " << dNORMAL << e.what() <<
            std::endl;
        ret = EXIT_FAILURE;
    } catch (const efyj::xml_parse_error& e) {
        std::cerr << dRED << "fail to parse file " << argv[i] << " in ("
                  << e.line << " << " << e.column << "): " << dNORMAL << e.what()
                  << std::endl;
        ret = EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << dRED << "unknown failure: " << dNORMAL <<  e.what() <<
            std::endl;
        ret = EXIT_FAILURE;
    }

    return ret;
}
