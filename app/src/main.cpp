/* Copyright (C) 2015-2017 INRA
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

#include <EASTL/string.h>

#include <efyj/efyj.hpp>

#include <cstdio>

#include <getopt.h>
#include <unistd.h>

namespace {

void
usage() noexcept
{
    puts("efyj [-h][-m file.dexi][-o file.csv][...]\n\n"
         "Options:\n"
         "    -h                   This help message\n"
         "    -m model.dexi        The model file\n"
         "    -o options.csv       The options file\n"
         "    -e output.csv        Extract the option from dexi files "
         "into csv file\n"
         "    -r                   Without the reduce models generator "
         "algorithm\n"
         "    -l [limit]           Modifier limit [int]\n"
         "                         0 means max available (default)\n"
         "                         > 1\n"
         "    -p                   Compute prediction\n"
         "    -a                   Compute adjustment\n"
         "    -j [threads]         Use threads [int]\n"
         "                         0 means max available\n"
         "                         1 means mono\n"
         "                         2..max\n"
         "\n");
}

void
version() noexcept
{
    puts("efyj 0.6.0\n");
}

int
extract(std::shared_ptr<efyj::context> ctx,
        const std::string& model,
        const std::string& output) noexcept
{
    try {
        auto opts = efyj::extract_options(ctx, model);
    } catch (const std::bad_alloc& e) {
        fprintf(stderr, "not enough memory\n");
        return EXIT_FAILURE;
    } catch (const std::logic_error& e) {
        fprintf(stderr, "internal error: %s\n", e.what());
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        fprintf(stderr, "failure: %s\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int
adjustment(std::shared_ptr<efyj::context> ctx,
           const std::string& model,
           const std::string& option,
           bool reduce,
           int limit,
           unsigned int thread) noexcept
{
    try {
        auto result =
          efyj::adjustment(ctx, model, option, reduce, limit, thread);
    } catch (const std::bad_alloc& e) {
        fprintf(stderr, "not enough memory\n");
        return EXIT_FAILURE;
    } catch (const std::logic_error& e) {
        fprintf(stderr, "internal error: %s\n", e.what());
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        fprintf(stderr, "failure: %s\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int
prediction(std::shared_ptr<efyj::context> ctx,
           const std::string& model,
           const std::string& option,
           bool reduce,
           int limit,
           unsigned int thread) noexcept
{
    try {
        auto result =
          efyj::prediction(ctx, model, option, reduce, limit, thread);
    } catch (const std::bad_alloc& e) {
        fprintf(stderr, "not enough memory\n");
        return EXIT_FAILURE;
    } catch (const std::logic_error& e) {
        fprintf(stderr, "internal error: %s\n", e.what());
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        fprintf(stderr, "failure: %s\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

} // anonymous namespace

class console_logger : public efyj::logger
{
public:
    virtual void write(int priority,
                       const char* file,
                       int line,
                       const char* fn,
                       const char* format,
                       va_list args) noexcept
    {
        if (priority <= 6)
            vfprintf(stdout, format, args);
        else {
            fprintf(stderr,
                    "LOG: %d at %d in function '%s' from file %s: ",
                    priority,
                    line,
                    fn,
                    file);
            vfprintf(stderr, format, args);
        }
    }

    virtual void write(efyj::message_type m,
                       const char* format,
                       va_list args) noexcept
    {
#ifdef __unix__
        if (::isatty(STDIN_FILENO)) {
            switch (m) {
            case efyj::message_type::highlight:
                ::puts("\033[30m\033[2m");
                break;
            case efyj::message_type::observation:
                ::puts("\033[32m\033[1m");
                break;
            case efyj::message_type::important:
                ::puts("\033[33m\033[1m");
                break;
            default:
                break;
            }
            vfprintf(stdout, format, args);
            ::puts("\033[30m\033[0m");
        } else {
            vfprintf(stdout, format, args);
        }
#else
        vfprintf(stdout, format, args);
#endif
    }
};

int
main(int argc, char* argv[])
{
    enum main_mode
    {
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
            if (::optarg) {
                int read = std::sscanf(::optarg, "%u", &threads);
                if (read != 1) {
                    fprintf(stderr, "Fail to read thread argument '%s'. Assumed"
                            "1.\n", ::optarg);
                    threads = 1;
                } else if (threads == 0) {
                    fprintf(stderr, "Bad thread argument '%s'. Assumed 1.\n",
                            ::optarg);
                    threads = 1;
                }
            }
            break;
        case 'e':
            extractfile.assign(::optarg);
            mode = mode | EXTRACT;
            break;
        case 'm':
            modelfilepath.assign(::optarg);
            break;
        case 'o':
            optionfilepath.assign(::optarg);
            break;
        case 'l':
            if (::optarg) {
                int read = std::sscanf(::optarg, "%d", &limit);
                if (read != 1) {
                    fprintf(stderr, "Fail to read limit argument '%s'. Assumed"
                            " 0 (infinity).\n", ::optarg);
                    limit = 0;
                } else if (limit < 0) {
                    fprintf(stderr, "Bad thread argument '%s'. Assumed 0"
                            " (infinity).\n", ::optarg);
                    limit = 0;
                }
            }
            break;
        case 'r':
            reduce = true;
            break;
        case 'p':
            mode = mode | PREDICTION;
            break;
        case 'a':
            mode = mode | ADJUSTMENT;
            break;
        case 'h':
            ::usage();
            exit(EXIT_SUCCESS);
        case 'v':
            ::version();
            exit(EXIT_SUCCESS);
        default:
            ::version();
            exit(EXIT_SUCCESS);
        }
    }

    if (::optind > argc) {
        fprintf(stderr, "Expected argument after -m, -o or -s options\n");
        exit(EXIT_FAILURE);
    }

    auto ctx = std::make_shared<efyj::context>();
    ctx->set_log_priority(7);
    ctx->set_logger(std::make_unique<console_logger>());

    int return_value = EXIT_SUCCESS;
    if ((mode & EXTRACT)) {
        if (::extract(ctx, modelfilepath, extractfile) == EXIT_FAILURE)
            return_value = EXIT_FAILURE;
    }

    if ((mode & ADJUSTMENT)) {
        if (::adjustment(
              ctx, modelfilepath, optionfilepath, reduce, limit, threads) ==
            EXIT_FAILURE)
            return_value = EXIT_FAILURE;
    }

    if ((mode & PREDICTION)) {
        if (::prediction(
              ctx, modelfilepath, optionfilepath, reduce, limit, threads) ==
            EXIT_FAILURE)
            return_value = EXIT_FAILURE;
    }

    return return_value;
}
