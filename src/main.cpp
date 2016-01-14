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

#include <efyj/problem.hpp>
#include <efyj/cstream.hpp>
#include <chrono>
#include <fstream>
#include <getopt.h>

namespace {

void usage() noexcept
{
    using efyj::out;

    out() << "efyj [-h][-m file.dexi][-o file.csv]\n\n"
          << "Options:\n"
          << "    -h                   This help message\n"
          << "    -e output.csv        Extract the option from dexi files "
          << "into csv file\n"
          << "    -m model.dexi        The model file\n"
          << "    -o options.csv       The options file\n"
          << "    -s solver_name       Select the specified solver\n"
          << "    -a [limit]           Compute the best model for kappa\n"
          << "                         0: compute kappa\n"
          << "                         1..n: number of walkers\n"
          << "                         -n: from 1 to n walkers\n"
          << "\n"
          << "Available solvers:\n"
          << "   stack              (default) stack and reverse polish "
          << "notation\n"
          << "   bigmem             default solver fill a big memory space\n";
}

int
model_read(const std::string& filename, efyj::Model& model) noexcept
{
    std::ifstream ifs(filename);
    if (not ifs.is_open()) {
        efyj::err() << "fail to open file: " << efyj::err().red()
                    << filename << efyj::err().def() << "\n";
        return -EINVAL;
    }

    try {
        model.read(ifs);
    } catch (const std::bad_alloc& e) {
        model.clear();
        efyj::err() << "not enough memory to read file: "
                    << efyj::err().red() << filename
                    << efyj::err().def() << "\n";
        return -ENOMEM;
    } catch (const efyj::xml_parser_error& e) {
        efyj::err() << "fail to read file: " << efyj::err().red()
                    << filename << efyj::err().def() << "at line "
                    << e.line() << " column " << e.column() << ": "
                    << e.message() << "\n";
        return -EINVAL;
    }

    return 0;
}

int
model_extract_options(const std::string& filename,
                      const efyj::Model& model) noexcept
{
    std::ofstream ofs(filename);
    if (not ofs.is_open()) {
        efyj::err() << "fail to open file: " << efyj::err().red()
                    << filename << efyj::err().def() << "\n";
        return -EINVAL;
    }

    try {
        model.write_options(ofs);
    } catch (const std::bad_alloc& e) {
        efyj::err() << "not enough memory to write file: "
                    << efyj::err().red() << filename
                    << efyj::err().def() << "\n";
        return -ENOMEM;
    }

    return 0;
}

int
options_read(const std::string& filename, const efyj::Model& model,
             efyj::Options& options) noexcept
{
    std::ifstream ifs(filename);
    if (not ifs.is_open()) {
        efyj::err() << "fail to open file: " << efyj::err().red()
                    << filename << efyj::err().def() << "\n";
        return -EINVAL;
    }

    try {
        options.read(ifs, model);
    } catch (const std::bad_alloc& e) {
        options.clear();
        efyj::err() << "not enough memory to read file: "
                    << efyj::err().red() << filename
                    << efyj::err().def() << "\n";
        return -ENOMEM;
    } catch (const efyj::csv_parser_error& e) {
        efyj::err() << "fail to read file: " << efyj::err().red()
                    << filename << efyj::err().def() << "at line "
                    << e.line() << " column " << e.column() << ": "
                    << e.message() << "\n";
    }

    return 0;
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
        efyj::err() << "Expected argument after -m, -o or -s options\n";
        exit(EXIT_FAILURE);
    }

    efyj::Model model;
    if (::model_read(modelfilepath, model))
        return EXIT_FAILURE;

    efyj::out() << model << "\n";

    if (not extractfile.empty()) {
        efyj::out() << "Extract options file from model.\n";
        ::model_extract_options(extractfile, model);
    }

    efyj::Options options;
    if (::options_read(optionfilepath, model, options))
        return EXIT_FAILURE;

    efyj::out() << options << "\n";

    try {
        if (limit == 0) {
            efyj::out() << "compute Kappa:\n";
            efyj::compute0(model, options, 0, 1);
        } else if (limit > 0) {
            efyj::out() << "compute best Kappa with " << limit << ":\n";
            efyj::computen(model, options, 0, 1, limit);
        } else {
            efyj::out() << "compute best Kappa for all model\n";
            efyj::compute_for_ever(model, options, 0, 1);
        }
    } catch (const std::exception &e) {
        efyj::err() << "failure: " << e.what() << '\n';
        ret = EXIT_FAILURE;
    }

    return ret;
}
