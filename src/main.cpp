/* Copyright (C) 2015 INRA
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

#include <efyj/model.hpp>
#include <efyj/exception.hpp>
#include <efyj/problem.hpp>

#include <iostream>
#include <chrono>
#include <fstream>
#include <getopt.h>

#ifdef EFYj_MPI_SUPPORT
    #include <mpi.h>
#endif

namespace {

void usage() noexcept
{
    std::cout << "efyj [-h][-m file.dexi][-o file.csv]\n\n"
              << "Options:\n"
              << "    -h                   This help message\n"
              << "    -e output.csv        Extract the option from dexi files into csv file\n"
              << "    -m model.dexi        The model file\n"
              << "    -o options.csv       The options file\n"
              << "    -s solver_name       Select the specified solver\n"
              << "    -a [limit]           Compute the best model for kappa\n"
              << "                         0: compute kappa\n"
              << "                         1..n: number of walkers\n"
              << "                         -n: from 1 to n walkers\n"
              << "\n"
              << "Available solvers:\n"
              << "   stack              (default) stack and reverse polish notation\n"
              << "   bigmem             default solver fill a big memory space\n"
              << std::endl;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;
    int opt;
    std::string modelfilepath, optionfilepath, extractfile;
    std::string solvername;
    int limit = 0;

    while ((opt = ::getopt(argc, argv, "m:o:s:e:ha:")) != -1) {
        switch (opt) {
        case 'e':
            extractfile.assign(::optarg);
            break;

        case 'm':
            modelfilepath.assign(::optarg);
            break;

        case 'o':
            optionfilepath.assign(::optarg);
            break;

        case 's':
            solvername.assign(::optarg);
            break;

        case 'a':
            if (::optarg)
                limit = std::stoi(::optarg);

            break;

        case 'h':
        default:
            ::usage();
            exit(EXIT_SUCCESS);
        }
    }

    if (::optind > argc) {
        std::cerr << "Expected argument after -m, -o or -s options\n";
        exit(EXIT_FAILURE);
    }

    try {
        efyj::Context ctx = std::make_shared <efyj::ContextImpl>(
            efyj::LOG_OPTION_ERR);

        efyj::Model model = model_read(ctx, modelfilepath);
        efyj::model_show(model, std::cout);

        if (!extractfile.empty())
            efyj::option_extract(ctx, model, extractfile);

        if (!optionfilepath.empty()) {
            efyj::Options options = option_read(ctx, model, optionfilepath);

            std::cout << options << "\n";

            if (limit == 0) {
                std::cout << "compute Kappa:\n";
                efyj::compute0(ctx, model, options, 0, 1);
            } else if (limit > 0) {
                std::cout << "compute best Kappa with " << limit << ":\n";
                efyj::computen(ctx, model, options, 0, 1, limit);
            } else {
                std::cout << "compute best Kappa for all model\n";
                efyj::compute_for_ever(ctx, model, options, 0, 1);
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "failure: " << e.what() << '\n';
        ret = EXIT_FAILURE;
    }

    return ret;
}
