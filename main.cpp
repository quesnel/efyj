/* Copyright (C) 2014 INRA
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

#include "io.hpp"
#include "model.hpp"
#include "print.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>

namespace {

    void usage() noexcept
    {
        std::cout << "dexi [files...]\n";
    }

    bool process(const std::string& filepath) noexcept
    {
        efyj::dexi dexi_data;
        bool ret = false;

        try {
            std::ifstream is(filepath);
            if (not is)
                throw std::invalid_argument(
                    efyj::stringf("unknown file %s", filepath.c_str()));
            efyj::read(is, dexi_data);
            ret = true;
        } catch (const std::bad_alloc& e) {
            std::cerr << dRED << "fail to allocate memory: " << dNORMAL << e.what();
        } catch (const std::invalid_argument& e) {
            std::cerr << dRED << "bad argument: " << dNORMAL << e.what();
        } catch (const efyj::xml_parse_error& e) {
            std::cerr << dRED << "fail to parse file " << filepath << " in ("
                << e.line << " << " << e.column << "): " << dNORMAL << e.what();
        } catch (const std::exception& e) {
            std::cerr << dRED << "unknown failure: " << dNORMAL <<  e.what();
        }

        return ret;
    }

}

int main(int argc, char *argv[])
{
#if defined NDEBUG
    std::ios_base::sync_with_stdio(false);
#endif

    int ret = EXIT_SUCCESS;

    if (argc == 1) {
        usage();
    } else {
        for (int i = 1; i < argc; ++i) {
            std::cout << "Processing [" << dYELLOW << argv[i] << dNORMAL << "] ";
            if (not process(argv[i]))
                ret = EXIT_FAILURE;
            std::cout << "\n";
        }
    }

    return ret;
}
