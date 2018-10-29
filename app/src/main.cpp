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

#include <efyj/efyj.hpp>

#ifdef __unix__
#include <unistd.h>
#endif

namespace EA {
namespace StdC {

int
Vsnprintf(char8_t* p, size_t n, const char8_t* pFormat, va_list arguments)
{
#ifdef _MSC_VER
    return vsnprintf_s(p, n, _TRUNCATE, pFormat, arguments);
#else
    return vsnprintf(p, n, pFormat, arguments);
#endif
}
}
}

void*
operator new[](size_t size,
               const char* pName,
               int flags,
               unsigned debugFlags,
               const char* file,
               int line)
{
    // #ifndef NDEBUG
    //     fprintf(stderr,
    //             "%zu in %s (flags: %d debug flags: %u) from file %s:%d\n",
    //             size,
    //             pName,
    //             flags,
    //             debugFlags,
    //             file,
    //             line);
    // #endif

    return malloc(size);
}

void*
operator new[](size_t size,
               size_t alignment,
               size_t alignmentOffset,
               const char* pName,
               int flags,
               unsigned debugFlags,
               const char* file,
               int line)
{
    // #ifndef NDEBUG
    //     fprintf(stderr,
    //             "%zu (alignment: %zu offset: %zu) in %s (flags: %d debug
    //             flags: "
    //             "%u) from file %s:%d\n",
    //             size,
    //             alignment,
    //             alignmentOffset,
    //             pName,
    //             flags,
    //             debugFlags,
    //             file,
    //             line);
    // #endif

    if ((alignmentOffset % alignment) == 0)
        return aligned_alloc(alignment, size);

    return malloc(size);
}

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
extract(eastl::shared_ptr<efyj::context> ctx,
        const eastl::string& model,
        const eastl::string& output) noexcept
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
adjustment(eastl::shared_ptr<efyj::context> ctx,
           const eastl::string& model,
           const eastl::string& option,
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
prediction(eastl::shared_ptr<efyj::context> ctx,
           const eastl::string& model,
           const eastl::string& option,
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

    eastl::string modelfilepath, optionfilepath, extractfile;

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
                    fprintf(stderr,
                            "Fail to read thread argument '%s'. Assumed"
                            "1.\n",
                            ::optarg);
                    threads = 1;
                } else if (threads == 0) {
                    fprintf(stderr,
                            "Bad thread argument '%s'. Assumed 1.\n",
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
                    fprintf(stderr,
                            "Fail to read limit argument '%s'. Assumed"
                            " 0 (infinity).\n",
                            ::optarg);
                    limit = 0;
                } else if (limit < 0) {
                    fprintf(stderr,
                            "Bad thread argument '%s'. Assumed 0"
                            " (infinity).\n",
                            ::optarg);
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

    auto ctx = efyj::make_context(7);

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
