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

#include <cstdio>
#include <charconv>

#include <fmt/format.h>
#include <fmt/color.h>

#ifdef __unix__
#include <unistd.h>
#endif

#ifdef _WIN32
void* __cdecl
operator new[](size_t size,
               const char* name,
               int flags,
               unsigned debugFlags,
               const char* file,
               int line)
{
    return new uint8_t[size];
}

void* __cdecl
operator new[](size_t size,
               size_t alignment,
               size_t alignmentOffset,
               const char* name,
               int flags,
               unsigned debugFlags,
               const char* file,
               int line)
{
    return new uint8_t[size];
}
#else
void*
operator new[](size_t size,
               const char* name,
               int flags,
               unsigned debugFlags,
               const char* file,
               int line)
{
    return new uint8_t[size];
}

void*
operator new[](size_t size,
               size_t alignment,
               size_t alignmentOffset,
               const char* name,
               int flags,
               unsigned debugFlags,
               const char* file,
               int line)
{
    return new uint8_t[size];
}
#endif

static void
usage() noexcept
{
    std::puts(
        "efyj [-h][-m file.dexi][-o file.csv][...]\n\n"
         "Options:\n"
        "    -h/--help            This help message\n"
        "    -v/--version          Show efyj version\n"
        "    -e/--extract         Extract the option from dexi files "
         "into csv file (need 1 csv, 1 dexi)\n"
         "    -m/--merge           Merge model and option file into a new "
         "DEXi file (need 1 csv, 2 dexi\n"
         "    -p/--prediction      Compute prediction\n"
         "    -a/--adjustement     Compute adjustment\n"
         "    -e/--evaluate        Compulte evalaution\n"
         "    --without-reduce     Without the reduce models generator "
         "algorithm\n"
         "    -l/--limit integer   Limit of computation\n"
         "    -j/--jobs thread     Use threads [int]\n"
         "...                      DEXi and CSV files\n"
         "\n");
}

static void
version() noexcept
{
    fmt::print("efyj {}.{}.{}\n", EFYJ_MAJOR_VERSION, EFYJ_MINOR_VERSION, EFYJ_PATCH_VERSION);
}

static int
extract(eastl::shared_ptr<efyj::context> ctx,
        const eastl::string& model,
        const eastl::string& output) noexcept
{
    try {
        auto opts = efyj::extract_options(ctx, model);
    } catch (const std::bad_alloc&) {
        fmt::print(stderr, "not enough memory\n");
        return EXIT_FAILURE;
    } catch (const std::logic_error& e) {
        fmt::print(stderr, "internal error: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        fmt::print(stderr, "failure: {}\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int
merge(eastl::shared_ptr<efyj::context> ctx,
      const eastl::string& model,
      const eastl::string& option,
      const eastl::string& output) noexcept
{
    try {
        efyj::merge_options(ctx, model, option, output);
    } catch (const std::bad_alloc&) {
        fmt::print(stderr, "not enough memory\n");
        return EXIT_FAILURE;
    } catch (const std::logic_error& e) {
        fmt::print(stderr, "internal error: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        fmt::print(stderr, "failure: {}\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int
evaluate(eastl::shared_ptr<efyj::context> ctx,
    const eastl::string& model,
    const eastl::string& option) noexcept
{
    try {
        auto result =
            efyj::evaluate(ctx, model, option);
    }
    catch (const std::bad_alloc&) {
        fmt::print(stderr, "not enough memory\n");
        return EXIT_FAILURE;
    }
    catch (const std::logic_error& e) {
        fmt::print(stderr, "internal error: {}\n", e.what());
        return EXIT_FAILURE;
    }
    catch (const std::runtime_error& e) {
        fmt::print(stderr, "failure: {}\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
static int
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
    } catch (const std::bad_alloc&) {
        fmt::print(stderr, "not enough memory\n");
        return EXIT_FAILURE;
    } catch (const std::logic_error& e) {
        fmt::print(stderr, "internal error: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        fmt::print(stderr, "failure: {}\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int
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
    } catch (const std::bad_alloc&) {
        fmt::print(stderr, "not enough memory\n");
        return EXIT_FAILURE;
    } catch (const std::logic_error& e) {
        fmt::print(stderr, "internal error: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        fmt::print(stderr, "failure: {}\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

enum class operation_type {
    none,
    extract,
    merge,
    evaluate,
    adjustment,
    prediction
};

namespace fmt {
    template <>
    struct formatter<eastl::string_view> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template <typename FormatContext>
        auto format(const eastl::string_view &p, FormatContext &ctx) {
            return format_to(ctx.begin(), "{}", p.data());
        }
    };
}

struct attributes
{
    eastl::string_view dexi_file;
    eastl::string_view csv_file;
    eastl::vector<eastl::string_view> optind;

    int threads = 1;

    operation_type type = operation_type::none;

    int limit = eastl::numeric_limits<int>::max();
    int thread = 1;
    bool reduce = true;

    bool show_version = false;
    bool show_help = false;

    bool
    parse_long_option(eastl::string_view opt, eastl::string_view arg)
    {
        fmt::print(fmt::color::beige, "parse long option {} arg {}", opt, arg);

        bool consume_arg = false;

        if (opt.compare("help") == 0)
            show_help = true;
        else if (opt.compare("version") == 0)
            show_version = true;
        else if (opt.compare("jobs") == 0)
            consume_arg = parse_jobs(arg);
        else if (opt.compare("extract") == 0)
            type = operation_type::extract;
        else if (opt.compare("merge") == 0)
            type = operation_type::merge;
        else if (opt.compare("evaluate") == 0)
            type = operation_type::evaluate;
        else if (opt.compare("adjustment") == 0)
            type = operation_type::adjustment;
        else if (opt.compare("prediction") == 0)
            type = operation_type::prediction;
        else if (opt.compare("limit") == 0)
            consume_arg = parse_limit(arg);
        else if (opt.compare("without-reduce") == 0)
            reduce = false;
        else
            fmt::print(stderr, "Unknown long option `{}'.\n", opt);

        return consume_arg;
    }

    bool
    parse_short_option(char opt, eastl::string_view arg)
    {
        fmt::print(fmt::color::beige, "parse short option {} arg {}", opt, arg);

        bool consume_arg = false;

        if (opt == 'h')
            show_help = true;
        else if (opt == 'v')
            show_version = true;
        else if (opt == 'j')
            consume_arg = parse_jobs(arg);
        else if (opt == 'e')
            type = operation_type::extract;
        else if (opt == 'm')
            type = operation_type::merge;
        else if (opt == 'v')
            type = operation_type::evaluate;
        else if (opt == 'a')
            type = operation_type::adjustment;
        else if (opt == 'p')
            type = operation_type::prediction;
        else
            fmt::print(stderr, "Unknown short option `{}'.\n", opt);

        return consume_arg;
    }

    bool parse_jobs(eastl::string_view arg)
    {
        if (arg.empty()) {
            fmt::print(stderr, "Missing argument for -j[threads]\n");
            return false;
        }

        int var;
        if (std::from_chars(arg.data(), arg.data() + arg.size(), var).ec != std::errc()) {
            fmt::print(stderr, "Missing argument for -j[threads]\n");
            return false;
        }

        if (var <= 0) {
            fmt::print(stderr, "Negative or zero argument for -j[threads]. Assume threads=1\n");
            return true;
        }

        thread = var;

        return true;
    }

    bool parse_limit(eastl::string_view arg)
    {
        if (arg.empty()) {
            fmt::print(stderr, "Missing argument for --limit [int]\n");
            return false;
        }

        int var;
        if (std::from_chars(arg.data(), arg.data() + arg.size(), var).ec != std::errc()) {
            fmt::print(stderr, "Missing argument for --limit [int]\n");
            return false;
        }

        if (var <= 0) {
            fmt::print(stderr, "Negative or zero argument for --limit [int]. Assume limit = {}\n", eastl::numeric_limits<int>::max());
            return true;
        }

        limit = var;

        return true;
    }
};


int
main(int argc, char* argv[])
{
    attributes atts;
    int i = 1;

    while (i < argc) {
        const eastl::string_view arg(argv[i]);

        if (arg[0] == '-') {
            if (arg.size() > 1U) {
                if (arg[1] == '-') {
                    auto pos = arg.find_first_of(":=", 2U);

                    if (pos == eastl::string_view::npos && i + 1 < argc) {
                        if (atts.parse_long_option(arg.substr(2), argv[i + 1]))
                            ++i;
                    }
                    else if (pos + 1 < arg.size())
                        atts.parse_long_option(arg.substr(2), arg.substr(pos + 1));
                    else
                        atts.parse_long_option(arg.substr(2), eastl::string_view());
                }
                else {
                    if (arg.size() > 2U)
                        atts.parse_short_option(arg[1], arg.substr(3));
                    else if (i + 1 < argc) {
                        if (atts.parse_short_option(arg[1], argv[i + 1]))
                            ++i;
                    }
                    else
                        atts.parse_short_option(arg[1], eastl::string_view());
                }
            }
            else {
                fmt::print(stderr, "Missing short option {} (position {})\n", argv[i], i);
            }
        }
        else {
            atts.optind.emplace_back(argv[i]);
        }
    }

    eastl::string dexifile1;
    eastl::string dexifile2;
    eastl::string csvfile;

    for (const auto& str : atts.optind) {
        if (str.ends_with(".csv"))
            csvfile = str;
        else if (str.ends_with(".dxi")) {
            if (dexifile1.empty())
                dexifile1 = str;
            else
                dexifile2 = str;
        } 
        else
            fmt::print(stderr, "unknown file type {}.\n", argv[i]);
    }
        
    auto ctx = efyj::make_context(7);

    if (atts.show_help)
        ::usage();

    if (atts.show_version)
        ::version();

    switch (atts.type) {
    case operation_type::extract:
        if (dexifile1.empty())
            fmt::print(stderr, "[extract] missing dexi.\n");
        else if (csvfile.empty())
            fmt::print(stderr, "[extract] missing csv file.\n");
        else {
            fmt::print("Extract options from file `{}' into file `{}'\n", dexifile1.c_str(), csvfile.c_str());
            ::extract(ctx, dexifile1, csvfile);
        }
        break;
    case operation_type::merge:
        if (dexifile1.empty())
            fmt::print(stderr, "[merge] missing dexi.\n");
        else if (csvfile.empty())
            fmt::print(stderr, "[merge] missing csv file.\n");
        else if (dexifile2.empty())
            fmt::print(stderr, "[merge] missing output dexi.\n");
        else {
            fmt::print("Merge options from csv file `{}' and DEXi file `{}' into the new DEXi file `{}'\n",
                csvfile.c_str(), dexifile1.c_str(), dexifile2.c_str());
            ::merge(ctx, dexifile1, csvfile, dexifile2);
        }
    case operation_type::evaluate:
        if (dexifile1.empty())
            fmt::print(stderr, "[evaluate] missing dexi.\n");
        else if (csvfile.empty())
            fmt::print(stderr, "[evaluate] missing csv file.\n");
        else {
            fmt::print("Evaluate options from file `{}' into file `{}'\n", dexifile1.c_str(), csvfile.c_str());
            ::evaluate(ctx, dexifile1, csvfile);
        }
        break;
    case operation_type::adjustment:
        if (dexifile1.empty())
            fmt::print(stderr, "[adjustment] missing dexi.\n");
        else if (csvfile.empty())
            fmt::print(stderr, "[adjustment] missing csv file.\n");
        else {
            fmt::print("Pdjustment options from file `{}' into file `{}'\n", dexifile1.c_str(), csvfile.c_str());
            ::prediction(ctx, dexifile1, csvfile, atts.reduce, atts.limit, atts.threads);
        }
        break;
    case operation_type::prediction:
        if (dexifile1.empty())
            fmt::print(stderr, "[prediction] missing dexi.\n");
        else if (csvfile.empty())
            fmt::print(stderr, "[prediction] missing csv file.\n");
        else {
            fmt::print("Prediction options from file `{}' into file `{}'\n", dexifile1.c_str(), csvfile.c_str());
            ::prediction(ctx, dexifile1, csvfile, atts.reduce, atts.limit, atts.threads);
        }
        break;
    }

    return EXIT_SUCCESS;
}
