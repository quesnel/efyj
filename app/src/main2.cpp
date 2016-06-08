/* Copyright (C) 2016 INRA
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

int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;
    int opt_index;
    std::string optionfile;

    const char* const short_opts = "o:h";
    const struct option long_opts[] = {
        {"help", 0, nullptr, 'h'},
        {"option", 1, nullptr, 'o'},
    };

    for (;;) {
        const auto opt = getopt_long(argc, argv, short_opts,
                                     long_opts, &opt_index);
        if (opt == -1)
            break;

        switch (opt) {
        case 'o':
            optionfile = ::optarg;
            break;
        case 'h':
        case '\?':
        default:
            printf("charlotte -o file_option file_dexi [file_dexi...]\n");
            break;
        }
    }

    std::vector<std::string> dexifiles(argv + ::optind, argv + argc);

    std::ofstream os_result("results.dat");
    std::ofstream os_kappa("kappa.dat");

    for (auto& elem : dexifiles) {
        try {
            efyj::efyj e(elem, optionfile);
            e.solve(elem, os_result, os_kappa);
        } catch (const std::exception& e) {
            fprintf(stderr, "An error occured during processing %s %s: %s\n",
                    elem.c_str(), optionfile.c_str(), e.what());
            ret = EXIT_FAILURE;
            break;
        }
    }

    return ret;
}
