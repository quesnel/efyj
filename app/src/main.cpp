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
#include "../../lib/src/utils.hpp"

#include <optional>

#include <charconv>
#include <cstdio>

#include <fmt/color.h>
#include <fmt/format.h>

#ifdef __unix__
#include <unistd.h>
#endif

static void
usage()
{
    std::puts(
      "efyj [-h][-m file.dexi][-o file.csv][...]\n\n"
      "Options:\n"
      "    -h/--help            This help message\n"
      "    -v/--version          Show efyj version\n"
      "    -x/--extract         Extract the option from dexi files "
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
      "    ...                  DEXi and CSV files\n"
      "\n");
}

static void
version()
{
    fmt::print("efyj {}.{}.{}\n",
               EFYJ_MAJOR_VERSION,
               EFYJ_MINOR_VERSION,
               EFYJ_PATCH_VERSION);
}

static bool
ends_with(const std::string_view str, const std::string_view suffix) noexcept
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// static bool
// starts_with(const std::string_view str, const std::string_view prefix)
// noexcept
// {
//     return str.size() >= prefix.size() &&
//            str.compare(0, prefix.size(), prefix) == 0;
// }

static int
extract(const efyj::context& ctx,
        const std::string& model,
        const std::string& output)
{
    try {
        efyj::extract_options_to_file(ctx, model, output);
    } catch (const std::bad_alloc&) {
        fmt::print(stderr, "not enough memory\n");
        return EXIT_FAILURE;
    } catch (const std::logic_error& e) {
        fmt::print(stderr, "internal error: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        fmt::print(stderr, "failure: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (const efyj::csv_parser_status& e) {
        fmt::print(stderr,
                   "fail to read csv file at line {} column {}: {}\n",
                   e.m_line,
                   e.m_column,
                   e.tag_name());
        return EXIT_FAILURE;
    } catch (const efyj::dexi_parser_status& e) {
        fmt::print(stderr,
                   "Fail to parse DEXi file at line {} column {}: {}\n",
                   e.m_line,
                   e.m_column,
                   e.tag_name());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int
merge(const efyj::context& ctx,
      const std::string& model,
      const std::string& option,
      const std::string& output)
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
    } catch (const efyj::csv_parser_status& e) {
        fmt::print(stderr,
                   "fail to read csv file at line {} column {}: {}\n",
                   e.m_line,
                   e.m_column,
                   e.tag_name());
        return EXIT_FAILURE;
    } catch (const efyj::dexi_parser_status& e) {
        fmt::print(stderr,
                   "Fail to parse DEXi file at line {} column {}: {}\n",
                   e.m_line,
                   e.m_column,
                   e.tag_name());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int
evaluate(const efyj::context& ctx,
         const std::string& model,
         const std::string& option)
{
    try {
        auto result = efyj::evaluate(ctx, model, option);
    } catch (const std::bad_alloc&) {
        fmt::print(stderr, "not enough memory\n");
        return EXIT_FAILURE;
    } catch (const std::logic_error& e) {
        fmt::print(stderr, "internal error: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        fmt::print(stderr, "failure: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (const efyj::csv_parser_status& e) {
        fmt::print(stderr,
                   "fail to read csv file at line {} column {}: {}\n",
                   e.m_line,
                   e.m_column,
                   e.tag_name());
        return EXIT_FAILURE;
    } catch (const efyj::dexi_parser_status& e) {
        fmt::print(stderr,
                   "Fail to parse DEXi file at line {} column {}: {}\n",
                   e.m_line,
                   e.m_column,
                   e.tag_name());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int
adjustment(const efyj::context& ctx,
           const std::string& model,
           const std::string& option,
           bool reduce,
           int limit,
           unsigned int thread)
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
    } catch (const efyj::csv_parser_status& e) {
        fmt::print(stderr,
                   "fail to read csv file at line {} column {}: {}\n",
                   e.m_line,
                   e.m_column,
                   e.tag_name());
        return EXIT_FAILURE;
    } catch (const efyj::dexi_parser_status& e) {
        fmt::print(stderr,
                   "Fail to parse DEXi file at line {} column {}: {}\n",
                   e.m_line,
                   e.m_column,
                   e.tag_name());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int
prediction(const efyj::context& ctx,
           const std::string& model,
           const std::string& option,
           bool reduce,
           int limit,
           unsigned int thread)
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
    } catch (const efyj::csv_parser_status& e) {
        fmt::print(stderr,
                   "fail to read csv file at line {} column {}: {}\n",
                   e.m_line,
                   e.m_column,
                   e.tag_name());
        return EXIT_FAILURE;
    } catch (const efyj::dexi_parser_status& e) {
        fmt::print(stderr,
                   "Fail to parse DEXi file at line {} column {}: {}\n",
                   e.m_line,
                   e.m_column,
                   e.tag_name());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

enum class operation_type
{
    none,
    extract,
    merge,
    evaluate,
    adjustment,
    prediction
};

struct attributes
{
    std::vector<std::string_view> optind;

    int threads = 1;

    operation_type type = operation_type::none;

    int limit = std::numeric_limits<int>::max();
    bool reduce = true;

    bool show_version = false;
    bool show_help = false;

    bool parse_long_option(std::string_view opt,
                           std::optional<std::string_view> arg)
    {
        if (arg)
            fmt::print("parse long option {} arg {}\n", opt, *arg);
        else
            fmt::print("parse long option {} without arg\n", opt);

        bool consume_arg = false;

        if (opt.compare("help") == 0)
            show_help = true;
        else if (opt.compare("version") == 0)
            show_version = true;
        else if (opt.compare("jobs") == 0 && arg)
            consume_arg = parse_jobs(*arg);
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
        else if (opt.compare("limit") == 0 && arg)
            consume_arg = parse_limit(*arg);
        else if (opt.compare("without-reduce") == 0)
            reduce = false;
        else
            fmt::print(stderr, "Unknown long option `{}'.\n", opt);

        return consume_arg;
    }

    bool parse_short_option(char opt, std::optional<std::string_view> arg)
    {
        if (arg)
            fmt::print("parse short option {} arg {}\n", opt, *arg);
        else
            fmt::print("parse short option {} without arg\n", opt);

        bool consume_arg = false;

        if (opt == 'h')
            show_help = true;
        else if (opt == 'v')
            show_version = true;
        else if (opt == 'j' && arg)
            consume_arg = parse_jobs(*arg);
        else if (opt == 'x')
            type = operation_type::extract;
        else if (opt == 'm')
            type = operation_type::merge;
        else if (opt == 'e')
            type = operation_type::evaluate;
        else if (opt == 'a')
            type = operation_type::adjustment;
        else if (opt == 'p')
            type = operation_type::prediction;
        else
            fmt::print(stderr, "Unknown short option `{}'.\n", opt);

        return consume_arg;
    }

    bool parse_jobs(std::string_view arg)
    {
        if (arg.empty()) {
            fmt::print(stderr, "Missing argument for -j[threads]\n");
            return false;
        }

        int var;
        if (std::from_chars(arg.data(), arg.data() + arg.size(), var).ec !=
            std::errc()) {
            fmt::print(stderr, "Missing argument for -j[threads]\n");
            return false;
        }

        if (var <= 0) {
            fmt::print(
              stderr,
              "Negative or zero argument for -j[threads]. Assume threads=1\n");
            return true;
        }

        threads = var;

        return true;
    }

    bool parse_limit(std::string_view arg)
    {
        if (arg.empty()) {
            fmt::print(stderr, "Missing argument for --limit [int]\n");
            return false;
        }

        int var;
        if (std::from_chars(arg.data(), arg.data() + arg.size(), var).ec !=
            std::errc()) {
            fmt::print(stderr, "Missing argument for --limit [int]\n");
            return false;
        }

        if (var <= 0) {
            fmt::print(stderr,
                       "Negative or zero argument for --limit [int]. Assume "
                       "limit = {}\n",
                       std::numeric_limits<int>::max());
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
        const std::string_view arg(argv[i]);

        fmt::print("Param `{}`\n", arg);

        if (arg[0] == '-') {
            if (arg.size() > 1U) {
                if (arg[1] == '-') {
                    auto pos = arg.find_first_of(":=", 2U);

                    if (pos == std::string_view::npos && i + 1 < argc) {
                        if (atts.parse_long_option(
                              arg.substr(2),
                              std::optional<std::string_view>(argv[i + 1])))
                            ++i;
                    } else if (pos + 1 < arg.size())
                        atts.parse_long_option(arg.substr(2),
                                               std::optional<std::string_view>(
                                                 arg.substr(pos + 1)));
                    else
                        atts.parse_long_option(
                          arg.substr(2), std::optional<std::string_view>());
                } else {
                    if (arg.size() > 2U)
                        atts.parse_short_option(
                          arg[1], std::make_optional(arg.substr(3)));
                    else if (i + 1 < argc) {
                        if (atts.parse_short_option(
                              arg[1],
                              std::optional<std::string_view>(argv[i + 1])))
                            ++i;
                    } else
                        atts.parse_short_option(
                          arg[1], std::optional<std::string_view>());
                }
            } else {
                fmt::print(stderr,
                           "Missing short option {} (position {})\n",
                           argv[i],
                           i);
            }
        } else {
            atts.optind.emplace_back(argv[i]);
        }

        ++i;
    }

    std::string dexifile1;
    std::string dexifile2;
    std::string csvfile;

    for (const auto& str : atts.optind) {
        if (ends_with(str, ".csv"))
            csvfile = str;
        else if (ends_with(str, ".dxi")) {
            if (dexifile1.empty())
                dexifile1 = str;
            else
                dexifile2 = str;
        } else
            fmt::print(stderr, "unknown file type {}.\n", argv[i]);
    }

    efyj::context ctx;

    ctx.dexi_cb = [](const efyj::status s,
                     int line,
                     int column,
                     const std::string_view tag) {
        fmt::print(stderr,
                   "DEXi error: {} at line {} column {} with tag {}",
                   get_error_message(s),
                   line,
                   column,
                   tag);
    };

    ctx.csv_cb = [](const efyj::status s, int line, int column) {
        fmt::print(stderr,
                   "CSV error: {} at line {} column {}",
                   get_error_message(s),
                   line,
                   column);
    };

    ctx.eov_cb = []() { fmt::print(stderr, "Not enough memory to continue"); };
    ctx.cast_cb = []() { fmt::print(stderr, "Internal error: cast failure"); };
    ctx.solver_cb = []() { fmt::print(stderr, "Solver error"); };
    ctx.file_cb = [](const std::string_view file_name) {
        fmt::print(stderr, "Error to access file `{}'", file_name);
    };

    if (atts.show_help)
        ::usage();

    if (atts.show_version)
        ::version();

    switch (atts.type) {
    case operation_type::none:
        break;
    case operation_type::extract:
        if (dexifile1.empty())
            fmt::print(stderr, "[extract] missing dexi.\n");
        else if (csvfile.empty())
            fmt::print(stderr, "[extract] missing csv file.\n");
        else {
            fmt::print("Extract options from file `{}' into file `{}'\n",
                       dexifile1.c_str(),
                       csvfile.c_str());
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
            fmt::print("Merge options from csv file `{}' and DEXi file `{}' "
                       "into the new DEXi file `{}'\n",
                       csvfile.c_str(),
                       dexifile1.c_str(),
                       dexifile2.c_str());
            ::merge(ctx, dexifile1, csvfile, dexifile2);
        }
        break;
    case operation_type::evaluate:
        if (dexifile1.empty())
            fmt::print(stderr, "[evaluate] missing dexi.\n");
        else if (csvfile.empty())
            fmt::print(stderr, "[evaluate] missing csv file.\n");
        else {
            fmt::print("Evaluate options from file `{}' into file `{}'\n",
                       dexifile1.c_str(),
                       csvfile.c_str());
            ::evaluate(ctx, dexifile1, csvfile);
        }
        break;
    case operation_type::adjustment:
        if (dexifile1.empty())
            fmt::print(stderr, "[adjustment] missing dexi.\n");
        else if (csvfile.empty())
            fmt::print(stderr, "[adjustment] missing csv file.\n");
        else {
            fmt::print("Pdjustment options from file `{}' into file `{}'\n",
                       dexifile1.c_str(),
                       csvfile.c_str());
            ::adjustment(
              ctx, dexifile1, csvfile, atts.reduce, atts.limit, atts.threads);
        }
        break;
    case operation_type::prediction:
        if (dexifile1.empty())
            fmt::print(stderr, "[prediction] missing dexi.\n");
        else if (csvfile.empty())
            fmt::print(stderr, "[prediction] missing csv file.\n");
        else {
            fmt::print("Prediction options from file `{}' into file `{}'\n",
                       dexifile1.c_str(),
                       csvfile.c_str());
            ::prediction(
              ctx, dexifile1, csvfile, atts.reduce, atts.limit, atts.threads);
        }
        break;
    }

    return EXIT_SUCCESS;
}
