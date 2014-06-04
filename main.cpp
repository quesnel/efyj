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

#include "io.hpp"
#include "model.hpp"
#include "solver.hpp"
#include "print.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <cstdlib>
#include <ctime>
#include <cstdlib>
#include <cstring>

namespace {

    void usage() noexcept
    {
        std::cout << "efyj [-h|--help] [-s|--stress [number]] [files...]\n\n"
                  << "Options:\n"
                  << "    -h, --help            This help message\n"
                  << "    -s, --stress [int]    Stress mode (check all solver/cache)\n"
                  << "                          If no argument, the default"
                  " make 100000 run\n"
                  << "    [files...]            DEXi file to run\n"
                  << std::endl;
    }

    void process(const std::string& filepath)
    {
        efyj::dexi dexi_data;

        std::ifstream is(filepath);
        if (not is)
            throw std::invalid_argument(
                efyj::stringf("unknown file %s", filepath.c_str()));

        efyj::read(is, dexi_data);
    }

    void generate_new_options(std::vector <std::uint8_t>& options,
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

            efyj::read(is, model);

            time_result[0] = std::chrono::system_clock::now() - start;
        }

        std::vector <std::size_t> high_level(model.basic_scale_number);
        int i = 0;
        for (const auto& att : model.attributes)
            if (att.children.empty())
                high_level[i++] = att.scale_size();

        {
            start = std::chrono::system_clock::now();

            std::vector <std::uint8_t> options(model.basic_scale_number, 0u);
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

            std::vector <std::uint8_t> options(model.basic_scale_number, 0u);
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

            std::vector <std::uint8_t> options(model.basic_scale_number, 0u);
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

int main(int argc, char *argv[])
{
#if defined NDEBUG
    std::ios_base::sync_with_stdio(false);
#endif

    bool stress_test = false;
    unsigned long stress_test_number = 100000;
    int ret = EXIT_SUCCESS;
    int i = 1;

    while (i < argc) {
        if (not std::strcmp(argv[i], "--stress") or
            not std::strcmp(argv[i], "-s")) {
            stress_test = true;
            i++;

            if (i < argc) {
                unsigned long nb = std::strtoul(argv[i], nullptr, 10);
                if (nb == 0 or errno)
                    i--;
                else
                    stress_test_number = nb;
            }

            std::cout << "- run stress mode for " << stress_test_number << " random run\n";
            i++;
            continue;
        }

        if (not std::strcmp(argv[i], "--help") or
            not std::strcmp(argv[i], "-h")) {
            usage();
            break;
        }

        std::cout << "Processing [" << dYELLOW << argv[i] << dNORMAL << "] \n";
        try {
            if (stress_test)
                process_stress_test(argv[i], stress_test_number);
            else
                process(argv[i]);

            i++;
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
    }

    return ret;
}
