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
#include <cstdlib>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("test empty object equality", "[model]")
{
    efyj::dexi x1;
    efyj::dexi x2;

    REQUIRE(x1 == x2);
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

TEST_CASE("test car.dxi load/save/load via sstream", "[model]")
{
    int ret = ::chdir(EXAMPLES_DIR);
    REQUIRE(ret == 0);

    efyj::dexi car;
    std::stringstream ss;

    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(efyj::read(is, car));
        REQUIRE_NOTHROW(efyj::write(ss, car));;
    }

    efyj::dexi car2;
    REQUIRE_NOTHROW(efyj::read(ss, car2));

    REQUIRE(car == car2);
}

#if (GCC_VERSION >= 40900) or (defined __clang__)
std::ofstream make_temporary(std::string& name)
{
    static const char *names[] = { "TMPDIR", "TMP", "TEMP" };
    static const int names_size = sizeof(names) / sizeof(names[0]);
    std::string ret;

    // TODO replace X with random bits
    // transform_if(...);

    for (int i = 0; i != names_size and ret.empty(); ++i)
        if (::getenv(names[i]))
            ret = ::getenv(names[i]);

    if (ret.empty())
        ret = "/tmp";

    ret += "/" + name;
    name = ret;

    return std::move(std::ofstream(ret));
}

TEST_CASE("test car.dxi load/save/load via file", "[model]")
{
    int ret = ::chdir(EXAMPLES_DIR);
    REQUIRE(ret == 0);

    efyj::dexi car;
    std::string outputfile("CarXXXXXXXX.dxi");

    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(efyj::read(is, car));

        std::ofstream os(make_temporary(outputfile));
        REQUIRE(os.is_open());
        REQUIRE_NOTHROW(efyj::write(os, car));;
    }

    efyj::dexi car2;

    {
        std::ifstream is(outputfile);
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(efyj::read(is, car2));
    }

    REQUIRE(car == car2);
}
#endif

TEST_CASE("test Car.dxi", "[model]")
{
    int ret = ::chdir(EXAMPLES_DIR);
    REQUIRE(ret == 0);

    efyj::dexi car;

    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(efyj::read(is, car));
    }

    REQUIRE(car.attributes.size() == 10u);
    REQUIRE(car.child);
    REQUIRE(car.child->name == "CAR");
    REQUIRE(car.child->children.size() == 2u);
    REQUIRE(car.child->children[0]->name == "PRICE");
    REQUIRE(car.child->children[0]->children.size() == 2u);
    REQUIRE(car.child->children[0]->children[0]->name == "BUY.PRICE");
    REQUIRE(car.child->children[0]->children[0]->children.empty() == true);
    REQUIRE(car.child->children[0]->children[1]->name == "MAINT.PRICE");
    REQUIRE(car.child->children[0]->children[1]->children.empty() == true);
    REQUIRE(car.child->children[1]->name == "TECH.CHAR.");
    REQUIRE(car.child->children[1]->children.size() == 2u);
    REQUIRE(car.child->children[1]->children[0]->name == "COMFORT");
    REQUIRE(car.child->children[1]->children[0]->children.size() == 3u);
    REQUIRE(car.child->children[1]->children[0]->children[0]->name == "#PERS");
    REQUIRE(car.child->children[1]->children[0]->children[0]->children.empty() == true);
    REQUIRE(car.child->children[1]->children[0]->children[1]->name == "#DOORS");
    REQUIRE(car.child->children[1]->children[0]->children[1]->children.empty() == true);
    REQUIRE(car.child->children[1]->children[0]->children[2]->name == "LUGGAGE");
    REQUIRE(car.child->children[1]->children[0]->children[2]->children.empty() == true);
    REQUIRE(car.child->children[1]->children[1]->name == "SAFETY");
    REQUIRE(car.child->children[1]->children[1]->children.empty() == true);
}

TEST_CASE("test multiple Car.dexi", "[model]")
{
    int ret = ::chdir(EXAMPLES_DIR);
    REQUIRE(ret == 0);

    efyj::dexi src, dst;

    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(efyj::read(is, src));
    }

    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(efyj::read(is, dst));
    }

    REQUIRE(src == dst);

    efyj::dexi dst2;
    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(efyj::read(is, dst2));
    }

    REQUIRE(dst2 == dst);
    REQUIRE(dst2.child);
    dst2.child->name = "change";

    REQUIRE(dst2 != dst);
}

#endif
