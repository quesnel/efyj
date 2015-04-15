/* Copyright (C) 2014-2015 INRA
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
              << "    -m model.dexi        The model file\n"
              << "    -o options.csv       The options file\n"
              << "    -s solver_name       Select the specified solver\n"
              << "\n"
              << "Available solvers:\n"
              << "   classic            the default tree path\n"
              << "   stack              stack and reverse polish notation\n"
              << "   bigmem             default solver fill a big memory space\n"
              << std::endl;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;
    int opt;
    std::string modelfilepath, optionfilepath;
    std::string solvername;

    while ((opt = ::getopt(argc, argv, "m:o:s:h")) != -1) {
        switch (opt) {
        case 'm':
            modelfilepath.assign(::optarg);
            break;

        case 'o':
            optionfilepath.assign(::optarg);
            break;

        case 's':
            solvername.assign(::optarg);
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
        efyj::Context ctx = std::make_shared <efyj::ContextImpl>(efyj::LOG_OPTION_ERR);
        efyj::problem pb(ctx, modelfilepath, optionfilepath);

        efyj::show(pb.model, std::cout);
        std::cout << '\n';

        if (solvername == "stack")
          pb.compute <efyj::solver_stack>(0, 1);
        else if (solvername == "bigmem")
          pb.compute <efyj::solver_bigmem>(0, 1);
        else
          pb.compute <efyj::solver_basic>(0, 1);
    } catch (const std::exception &e) {
        std::cerr << "failure: " << e.what() << '\n';
    }

    return ret;
}
