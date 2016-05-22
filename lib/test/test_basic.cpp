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

#include "context.hpp"
#include "problem.hpp"
#include "model.hpp"
#include "solver.hpp"
#include <sstream>
#include <cstdlib>

#if defined(__unix__)
    #include <unistd.h>
#elif defined(__WIN32__)
    #include <io.h>
    #include <stdio.h>
#endif

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("test empty object equality", "[model]")
{
    efyj::Model x1;
    efyj::Model x2;
    REQUIRE(x1 == x2);
}

TEST_CASE("test empty object read/write", "[model]")
{
    efyj::Model x1, x2;
    {
        std::string result;
        {
            std::ostringstream os;
            x1.write(os);
            result = os.str();
        }
        {
            std::istringstream is(result);
            x2.read(is);
        }
    }
    bool is_equal = x1 == x2;
    REQUIRE(is_equal == true);
}

#include <unistd.h>

static inline void change_pwd()
{
#ifdef EXAMPLES_DIR
    int ret = ::chdir(EXAMPLES_DIR);
    REQUIRE(ret == 0);
#else
    int ret = ::chdir("/home/goth/devel/efyj/efyj/tests");
    REQUIRE(ret == 0);
#endif
}

TEST_CASE("test classic Model file", "[model]")
{
    change_pwd();

    std::vector <std::string> filepaths = { "Car.dxi", "Employ.dxi",
                                            "Enterprise.dxi", "IPSIM_PV_simulation1-1.dxi", "Nursery.dxi",
                                            "Shuttle.dxi"
                                          };

    for (const auto &filepath : filepaths) {
        efyj::Model dex1, dex2;
        std::string output("/tmp/");
        output += filepath;
        {
            std::ifstream is(filepath);
            REQUIRE(is.is_open());
            REQUIRE_NOTHROW(dex1.read(is));
            std::ofstream os(output);
            dex1.write(os);
        }
        {
            std::ifstream is(filepath);
            REQUIRE(is.is_open());
            REQUIRE_NOTHROW(dex2.read(is));
        }
        REQUIRE(dex1 == dex2);
    }
}

TEST_CASE("test car.dxi load/save/load via sstream", "[model]")
{
    change_pwd();

    efyj::Model car;
    std::stringstream ss;
    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(car.read(is));
        REQUIRE_NOTHROW(car.write(ss));
    }
    efyj::Model car2;
    REQUIRE_NOTHROW(car2.read(ss));
    REQUIRE(car == car2);
}

#if (GCC_VERSION >= 40900) or (defined __clang__)
std::ofstream make_temporary(std::string &name, bool remove = true)
{
    static const char *names[] = { "TMPDIR", "TMP", "TEMP" };
    static const int names_size = sizeof(names) / sizeof(names[0]);
    std::string filename;

    // TODO replace X with random bits
    // transform_if(...);

    for (int i = 0; i != names_size and filename.empty(); ++i)
        if (::getenv(names[i]))
            filename = ::getenv(names[i]);

    if (filename.empty())
        filename = "/tmp";

    filename += "/" + name;
    name = filename;
    std::ofstream os(filename);

    if (os && remove) {
#if defined(__unix__)
        ::unlink(filename.c_str());
#elif defined(__WIN32__)
        ::_unlink(filename.c_str());
#endif
    }

    return std::move(os);
}

TEST_CASE("test car.dxi load/save/load via file", "[model]")
{
    int ret = ::chdir(EXAMPLES_DIR);
    REQUIRE(ret == 0);
    efyj::Model car;
    std::string outputfile("CarXXXXXXXX.dxi");
    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(is >> car);
        std::ofstream os(make_temporary(outputfile, false));
        REQUIRE(os.is_open());
        REQUIRE_NOTHROW(os << car);
    }
    efyj::Model car2;
    {
        std::ifstream is(outputfile);
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(is >> car2);
    }
    REQUIRE(car == car2);
}
#endif

TEST_CASE("test Car.dxi", "[model]")
{
    change_pwd();

    efyj::Model car;
    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(car.read(is));
    }
    REQUIRE(car.attributes.size() == 10u);
    REQUIRE(car.attributes[0].name == "CAR");
    REQUIRE(car.attributes[0].children.size() == 2u);
    REQUIRE(car.attributes[0].children == std::vector <std::size_t>({1, 4}));
    REQUIRE(car.attributes[1].name == "PRICE");
    REQUIRE(car.attributes[1].children.size() == 2u);
    REQUIRE(car.attributes[1].children == std::vector <std::size_t>({ 2, 3 }));
    REQUIRE(car.attributes[2].name == "BUY.PRICE");
    REQUIRE(car.attributes[2].children.empty() == true);
    REQUIRE(car.attributes[3].name == "MAINT.PRICE");
    REQUIRE(car.attributes[3].children.empty() == true);
    REQUIRE(car.attributes[4].name == "TECH.CHAR.");
    REQUIRE(car.attributes[4].children.size() == 2u);
    REQUIRE(car.attributes[4].children == std::vector <std::size_t>({5, 9}));
    REQUIRE(car.attributes[5].name == "COMFORT");
    REQUIRE(car.attributes[5].children.size() == 3u);
    REQUIRE(car.attributes[5].children == std::vector <std::size_t>({6, 7, 8}));
    REQUIRE(car.attributes[6].name == "#PERS");
    REQUIRE(car.attributes[6].children.empty() == true);
    REQUIRE(car.attributes[7].name == "#DOORS");
    REQUIRE(car.attributes[7].children.empty() == true);
    REQUIRE(car.attributes[8].name == "LUGGAGE");
    REQUIRE(car.attributes[8].children.empty() == true);
    REQUIRE(car.attributes[9].name == "SAFETY");
    REQUIRE(car.attributes[9].children.empty() == true);
}

TEST_CASE("test multiple Car.Model", "[model]")
{
    change_pwd();

    efyj::Model src, dst;
    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(src.read(is));
    }
    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(dst.read(is));
    }
    REQUIRE(src == dst);
    efyj::Model dst2;
    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(dst2.read(is));
    }
    REQUIRE(dst2 == dst);
    dst2.attributes[0].name = "change";
    REQUIRE(dst2 != dst);
}

TEST_CASE("test solver Car", "[model]")
{
    change_pwd();

    efyj::Model model;
    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(model.read(is));
    }
    std::size_t problem_size = 1;
    std::size_t basic_scale_number = 0;
    std::size_t scale_number = 0;
    std::size_t scalevalue_number = 0;

    for (auto att : model.attributes) {
        if (att.is_basic()) {
            basic_scale_number++;
            scalevalue_number += att.scale.size();
            problem_size *= att.scale.size();
        }

        scale_number++;
    }

    REQUIRE(basic_scale_number == 6u);
    REQUIRE(scale_number == 10u);
    REQUIRE(scalevalue_number == 19u);
    REQUIRE(problem_size == 972u);
}

TEST_CASE("test basic solver for Car", "[model]")
{
    change_pwd();

    efyj::Model model;
    {
        std::ifstream is("Car.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(model.read(is));
    }
    efyj::Vector opt_v3(6); opt_v3 << 1, 2, 2, 2, 2, 2;
    efyj::Vector opt_v2(6); opt_v2 << 1, 1, 2, 2, 2, 1;
    efyj::Vector opt_v4(6); opt_v4 << 2, 2, 2, 3, 2, 2;
    efyj::Vector opt_v5(6); opt_v5 << 0, 0, 0, 0, 0, 0;
    {
        efyj::Solver s(model);
        REQUIRE(s.solve(opt_v3) == 3);
        REQUIRE(s.solve(opt_v2) == 2);
        REQUIRE(s.solve(opt_v4) == 3);
        REQUIRE(s.solve(opt_v5) == 0);
    }
}

TEST_CASE("test basic solver for Enterprise", "[model]")
{
    change_pwd();

    efyj::Model model;
    {
        std::ifstream is("Enterprise.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(model.read(is));
    }
    efyj::Vector opt_v(12); opt_v << 2, 0, 0, 0, 2, 0, 0, 0, 1, 1, 1, 1;
    efyj::Solver ss(model);
    REQUIRE(ss.solve(opt_v) == 1);
}

TEST_CASE("test basic solver for IPSIM_PV_simulation1-1", "[model]")
{
    change_pwd();

    efyj::Model *model = new efyj::Model;
    {
        std::ifstream is("IPSIM_PV_simulation1-1.dxi");
        REQUIRE(is.is_open());
        REQUIRE_NOTHROW(model->read(is));
    }
    {
        efyj::Vector opt_v(14); opt_v << 2, 0, 0, 1, 0, 1, 1, 0, 0, 0, 2, 0, 0, 1;
        efyj::Solver ss(*model);
        REQUIRE(ss.solve(opt_v) == 6);
    }
    efyj::Model copy1(*model);
    delete model;
    {
        efyj::Vector opt_v(14); opt_v << 2, 0, 0, 1, 0, 1, 1, 0, 0, 0, 2, 0, 0, 1;
        efyj::Solver ss(copy1);
        REQUIRE(ss.solve(opt_v) == 6);
    }
}

TEST_CASE("test problem Model file", "[model]")
{
    change_pwd();

    std::vector <std::string> filepaths = {
        "Car.dxi", "Employ.dxi", "Enterprise.dxi",
        "IPSIM_PV_simulation1-1.dxi" };

    auto ctx = std::make_shared <efyj::Context>(efyj::LOG_OPTION_DEBUG);

    for (const auto &filepath : filepaths) {
        std::cout << "run " << filepath << "\n";

        efyj::Model model;

        {
            std::ifstream ifs(filepath);
            model.read(ifs);
        }

        {
            std::ofstream ofs("/tmp/toto.csv");
            model.write_options(ofs);
        }

        efyj::Options options;

        {
            std::ifstream ifs("/tmp/toto.csv");
            options.read(ctx, ifs, model);
        }

        efyj::Problem problem(ctx);
        double kappa = problem.compute0(model, options);
        REQUIRE(kappa == 1.0);
    }
}

TEST_CASE("test multiple solver for Car", "[model]")
{
    auto ctx = std::make_shared <efyj::Context>();

    change_pwd();

    efyj::Model model;

    {
        std::ifstream ifs("Car.dxi");
        model.read(ifs);
    }

    {
        std::ofstream ofs("/tmp/Car.csv");
        model.write_options(ofs);
    }

    efyj::Options options;

    {
        std::ifstream ifs("/tmp/Car.csv");
        options.read(ctx, ifs, model);

    }

     efyj::Problem problem(ctx);

     double kappa;
     kappa = problem.compute0(model, options);
     REQUIRE(kappa == 1);

     // We change the simulation result.

     std::cerr << "Options to change: " << options.options.cols() - 1 << '\n';
     options.options(options.options.cols() - 1, 0) = 0;

     kappa = problem.compute0(model, options);
     REQUIRE(kappa == Approx(0.6667).epsilon(0.1));

     {
         auto kappa_11 = problem.compute0(model, options);
         REQUIRE(kappa_11 == Approx(0.6667).epsilon(0.1));
     }

     {
         auto kappa_11 = problem.computen(model, options, 1);
         REQUIRE(kappa_11 == 1);
     }
}