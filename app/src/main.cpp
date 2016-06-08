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

#include "efyj.hpp"
#include <fstream>
#include <iostream>
#include <getopt.h>
#include "config.hpp"

namespace
{

void usage() noexcept
{
    std::cout
        << "efyj [-h][-m file.dexi][-o file.csv][...]\n\n"
        << "Options:\n"
        << "    -h                   This help message\n"
        << "    -m model.dexi        The model file\n"
        << "    -o options.csv       The options file\n"
        << "    -e output.csv        Extract the option from dexi files "
           "into csv file\n"
        << "    -r                   Without the reduce models generator "
           "algorithm\n"
        << "    -l [limit]           Modifier limit [int]\n"
        << "                         0 means max available (default)\n"
        << "                         > 1\n"
        << "    -p                   Compute prediction\n"
        << "    -a                   Compute adjustment\n"
        << "    -j [threads]         Use threads [int]\n"
        << "                         0 means max available\n"
        << "                         1 means mono\n"
        << "                         2..max\n"
        << "\n";
}

void version() noexcept { std::cout << "efyj " << EFYJ_VERSION << '\n'; }

int extract(const std::string &model, const std::string &output) noexcept
{
    try {
        efyj::efyj e(model);
        e.extract_options(output);
    } catch (const std::logic_error &e) {
        std::cerr << "internal error: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::runtime_error &e) {
        std::cerr << "failure: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::bad_alloc &e) {
        std::cerr << "not enough memory\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int adjustment(const std::string &model,
               const std::string &option,
               bool reduce,
               int limit,
               unsigned int thread) noexcept
{
    (void)thread;

    try {
        efyj::efyj e(model, option);
        e.compute_adjustment(limit, -1, reduce);
    } catch (const std::logic_error &e) {
        std::cerr << "internal failure: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::runtime_error &e) {
        std::cerr << "fail: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::bad_alloc &e) {
        std::cerr << "not enough memory\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int prediction(const std::string &model,
               const std::string &option,
               bool reduce,
               int limit,
               unsigned int thread) noexcept
{
    (void)thread;

    try {
        efyj::efyj e(model, option);
        e.compute_prediction(limit, -1, reduce);
    } catch (const std::logic_error &e) {
        std::cerr << "internal failure: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::runtime_error &e) {
        std::cerr << "fail: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (const std::bad_alloc &e) {
        std::cerr << "not enough memory\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
    enum main_mode {
        NOTHING = 0,
        EXTRACT = 1 << 1,
        ADJUSTMENT = 1 << 2,
        PREDICTION = 1 << 3
    };

    std::string modelfilepath, optionfilepath, extractfile;

    unsigned int mode = 0;
    unsigned int threads = 0;
    int limit = 0;
    int opt;
    bool reduce = true;

    while ((opt = ::getopt(argc, argv, "j::m:o:l:e:phvra")) != -1) {
        switch (opt) {
        case 'j':
            if (::optarg)
                threads = std::stoi(::optarg);
            else
                threads = 1;
            break;
        case 'e':
            extractfile.assign(::optarg);
            mode = mode | EXTRACT;
            break;
        case 'm': modelfilepath.assign(::optarg); break;
        case 'o': optionfilepath.assign(::optarg); break;
        case 'l':
            if (::optarg)
                limit = std::stoi(::optarg);
            break;
        case 'r': reduce = true; break;
        case 'p': mode = mode | PREDICTION; break;
        case 'a': mode = mode | ADJUSTMENT; break;
        case 'h': ::usage(); exit(EXIT_SUCCESS);
        case 'v': ::version(); exit(EXIT_SUCCESS);
        default: ::version(); exit(EXIT_SUCCESS);
        }
    }

    if (::optind > argc) {
        std::cerr << "Expected argument after -m, -o or -s options\n";
        exit(EXIT_FAILURE);
    }

    int return_value = EXIT_SUCCESS;
    if ((mode & EXTRACT)) {
        if (::extract(modelfilepath, extractfile) == EXIT_FAILURE)
            return_value = EXIT_FAILURE;
    }

    if ((mode & ADJUSTMENT)) {
        if (::adjustment(
                modelfilepath, optionfilepath, reduce, limit, threads) ==
            EXIT_FAILURE)
            return_value = EXIT_FAILURE;
    }

    if ((mode & PREDICTION)) {
        if (::prediction(
                modelfilepath, optionfilepath, reduce, limit, threads) ==
            EXIT_FAILURE)
            return_value = EXIT_FAILURE;
    }

    return return_value;
}
