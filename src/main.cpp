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

#include <efyj/problem.hpp>
#include <efyj/cstream.hpp>
#include <chrono>
#include <fstream>
#include <getopt.h>

namespace {

void usage(std::shared_ptr<efyj::Context> context) noexcept
{
    context->info() << "efyj [-h][-m file.dexi][-o file.csv][...]\n\n"
                   << "Options:\n"
                   << "    -h                   This help message\n"
                   << "    -m model.dexi        The model file\n"
                   << "    -o options.csv       The options file\n"
                   << "    -e output.csv        Extract the option from dexi files "
                   << "into csv file\n"
                   << "    -g generate.data     Extract all models from model and "
                   << "options.\n"
                   << "    -r                   Without the reduce models generator "
                   << "algorithm\n"
                   << "    -a [limit]           Compute the best model for kappa\n"
                   << "                         0: compute kappa\n"
                   << "                         1..n: number of walkers\n"
                   << "                         -n: from 1 to n walkers\n"
                   << "    -p                   Compute prediction\n"
                   << "    -j [threads]         Use threads [int]\n"
                   << "\n";
}

int
model_read(std::shared_ptr<efyj::Context> context,
           const std::string& filename,
           efyj::Model& model) noexcept
{
    std::ifstream ifs(filename);
    if (not ifs.is_open()) {
        context->err() << "fail to open file: " << context->err().red()
                      << filename << context->err().def() << "\n";
        return -EINVAL;
    }

    try {
        model.read(ifs);
    } catch (const std::bad_alloc& e) {
        model.clear();
        context->err() << "not enough memory to read file: "
                    << context->err().red() << filename
                    << context->err().def() << "\n";
        return -ENOMEM;
    } catch (const efyj::xml_parser_error& e) {
        context->err() << "fail to read file: " << context->err().red()
                    << filename << context->err().def() << "at line "
                    << e.line() << " column " << e.column() << ": "
                    << e.message() << "\n";
        return -EINVAL;
    }

    return 0;
}

int
model_extract_options(std::shared_ptr<efyj::Context> context,
                      const std::string& filename,
                      const efyj::Model& model) noexcept
{
    std::ofstream ofs(filename);
    if (not ofs.is_open()) {
        context->err() << "fail to open file: " << context->err().red()
                    << filename << context->err().def() << "\n";
        return -EINVAL;
    }

    try {
        model.write_options(ofs);
    } catch (const std::bad_alloc& e) {
        context->err() << "not enough memory to write file: "
                    << context->err().red() << filename
                    << context->err().def() << "\n";
        return -ENOMEM;
    }

    return 0;
}

int
options_read(std::shared_ptr<efyj::Context> context,
             const std::string& filename,
             const efyj::Model& model,
             efyj::Options& options) noexcept
{
    std::ifstream ifs(filename);
    if (not ifs.is_open()) {
        context->err() << "fail to open file: " << context->err().red()
                    << filename << context->err().def() << "\n";
        return -EINVAL;
    }

    try {
        options.read(context, ifs, model);
    } catch (const std::bad_alloc& e) {
        options.clear();
        context->err() << "not enough memory to read file: "
                    << context->err().red() << filename
                    << context->err().def() << "\n";
        return -ENOMEM;
    } catch (const efyj::csv_parser_error& e) {
        context->err() << "fail to read file: " << context->err().red()
                    << filename << context->err().def() << "at line "
                    << e.line() << " column " << e.column() << ": "
                    << e.message() << "\n";
    }

    return 0;
}

int
models_generate(std::shared_ptr<efyj::Context> context,
                const std::string& filename,
                const efyj::Model& model,
                const efyj::Options& options) noexcept
{
    std::ofstream ofs(filename);
    if (not ofs.is_open()) {
        context->err() << "fail to open file: " << context->err().red()
                       << filename << context->err().def() << "\n";
        return -EINVAL;
    }

    try {
        efyj::Problem problem(context);
        problem.generate_all_models(model, options, ofs);
    } catch (const std::bad_alloc& e) {
        context->err() << "not enough memory to generate all models: "
                    << context->err().red() << filename
                    << context->err().def() << "\n";
        return -ENOMEM;
    }

    return 0;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;
    int opt;
    std::string modelfilepath, optionfilepath, extractfile, generatedfilepath;
    int limit = 0;
    unsigned int threads = 0;
    bool without_reduce = false;
    bool with_prediction = false;

    auto context = std::make_shared<efyj::Context>(efyj::LOG_OPTION_DEBUG);

    while ((opt = ::getopt(argc, argv, "j::m:o:s:g:e:phra:")) != -1) {
        switch (opt) {
        case 'j':
            if (::optarg)
                threads = std::stoi(::optarg);
            else
                threads = 1;
            break;

        case 'g':
            generatedfilepath.assign(::optarg);
            break;

        case 'e':
            extractfile.assign(::optarg);
            break;

        case 'm':
            modelfilepath.assign(::optarg);
            break;

        case 'o':
            optionfilepath.assign(::optarg);
            break;

        case 'a':
            if (::optarg)
                limit = std::stoi(::optarg);

            break;

        case 'r':
            without_reduce = true;
            break;

        case 'p':
            with_prediction = true;
            break;

        case 'h':
        default:
            ::usage(context);
            exit(EXIT_SUCCESS);
        }
    }

    if (::optind > argc) {
        context->err() << "Expected argument after -m, -o or -s options\n";
        exit(EXIT_FAILURE);
    }

    efyj::Model model;
    if (::model_read(context, modelfilepath, model))
        return EXIT_FAILURE;

    context->info() << model << "\n";

    if (not extractfile.empty()) {
        context->info() << "Extract options file from model.\n";
        ::model_extract_options(context, extractfile, model);
    }

    efyj::Options options;
    if (::options_read(context, optionfilepath, model, options))
        return EXIT_FAILURE;

    context->dbg() << options << "\n";

    std::unique_ptr<efyj::Problem> problem;

    if (threads == 0)
        problem = std::make_unique<efyj::Problem>(context);
    else
        problem = std::make_unique<efyj::Problem>(context,
                                                  threads == 1 ? 0 : threads);

    if (not generatedfilepath.empty()) {
        context->info() << "Generates all models.\n";
        ::models_generate(context, generatedfilepath, model, options);
    }

    try {
        if (with_prediction) {
            context->info() << "compute prediction:\n";
            problem->prediction(model, options);
        } else {
            if (limit == 0) {
                context->info() << "compute Kappa:\n";
                problem->compute0(model, options);
            } else if (limit > 0) {
                context->info() << "compute best Kappa with " << limit << ":\n";
                problem->computen(model, options, limit);
            } else {
                context->info() << "compute best Kappa for all model\n";
                problem->compute_for_ever(model, options, not without_reduce);
            }
        }
    } catch (const std::exception &e) {
        context->err() << "failure: " << e.what() << '\n';
        ret = EXIT_FAILURE;
    }

    return ret;
}
