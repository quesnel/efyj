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

#include "parser.hpp"
#include "model.hpp"
#include "dbg.hpp"
#include <sstream>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("test empty object equality", "[model]")
{
    efyj::dexi x1;
    efyj::dexi x2(x1);

    bool is_equal = x1 == x2;

    REQUIRE(is_equal == true);
}

TEST_CASE("test empty object read/write", "[model]")
{
    efyj::dexi x1, x2;

    {
        std::string result;
        {
            std::ostringstream os;
            efyj::write(os, x1);
            result = os.str();
        }

        {
            std::istringstream is(result);
            efyj::read(is, x2);
        }
    }

    bool is_equal = x1 == x2;
    REQUIRE(is_equal == true);
}

#if defined EXAMPLES_DIR && defined __unix__
#include <unistd.h>

TEST_CASE("test classic dexi file", "[model]")
{
    int ret = ::chdir(EXAMPLES_DIR);
    REQUIRE(ret == 0);

    std::vector <std::string> filepaths = { "Car.dxi", "Employ.dxi",
        "Enterprise.dxi", "IPSIM_PV_simulation1-1.dxi", "Nursery.dxi",
        "Shuttle.dxi"};

    for (const auto& filepath : filepaths) {
        dInfo("Now we check:", filepath);

        std::ifstream is(filepath);
        REQUIRE(is.is_open());

        efyj::dexi dex;
        REQUIRE_NOTHROW(efyj::read(is, dex));
    }
}
#endif
