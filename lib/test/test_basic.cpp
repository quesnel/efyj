/* Copyright (C) 2015, 2016 INRA
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
#include "model.hpp"
#include "solver-stack.hpp"
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <random>
#if defined(__unix__)
#include <unistd.h>
#elif defined(__WIN32__)
#include <io.h>
#include <stdio.h>
#endif

#include "unit-test.hpp"

void test_empty_object_equality()
{
    efyj::Model x1;
    efyj::Model x2;
    Ensures(x1 == x2);
}

void test_empty_object_read_write()
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
    Ensures(is_equal == true);
}

static inline void change_pwd()
{
#if defined(__unix__)
    int ret = ::chdir(EXAMPLES_DIR);
#else
    int ret = ::_chdir(EXAMPLES_DIR);
#endif
    Ensures(ret == 0);
}

void test_classic_Model_file()
{
    change_pwd();

    std::vector<std::string> filepaths = {"Car.dxi",
                                          "Employ.dxi",
                                          "Enterprise.dxi",
                                          "IPSIM_PV_simulation1-1.dxi",
                                          "Nursery.dxi",
                                          "Shuttle.dxi"};

    for (const auto &filepath : filepaths) {
        efyj::Model dex1, dex2;
        std::string output("/tmp/");
        output += filepath;
        {
            std::ifstream is(filepath);
            Ensures(is.is_open());
            EnsuresNotThrow(dex1.read(is), std::exception);
            std::ofstream os(output);
            dex1.write(os);
        }
        {
            std::ifstream is(filepath);
            Ensures(is.is_open());
            EnsuresNotThrow(dex2.read(is), std::exception);
        }
        Ensures(dex1 == dex2);
    }
}

void test_car_dxi_load_save_load_via_sstream()
{
    change_pwd();

    efyj::Model car;
    std::stringstream ss;
    {
        std::ifstream is("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(car.read(is), std::exception);
        EnsuresNotThrow(car.write(ss), std::exception);
    }
    efyj::Model car2;
    EnsuresNotThrow(car2.read(ss), std::exception);
    Ensures(car == car2);
}

std::string make_temporary(const std::string &name)
{
    static const char *names[] = {"TMPDIR", "TMP", "TEMP"};
    static const int names_size = sizeof(names) / sizeof(names[0]);
    std::string ret;

    for (int i = 0; i != names_size and ret.empty(); ++i)
        if (::getenv(names[i]))
            ret = ::getenv(names[i]);

    if (ret.empty())
        ret = "./";
    else
        ret += '/';

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, 26);

    std::transform(name.begin(),
                   name.end(),
                   std::back_inserter(ret),
                   [&generator, &distribution](int character) {
                       if (character == 'X')
                           return 'A' + distribution(generator);

                       return character;
                   });
    return ret;
}

void test_car_dxi_load_save_load_via_file()
{
    change_pwd();

    efyj::Model car;
    std::string outputfile("CarXXXXXXXX.dxi");
    auto tmp = make_temporary(outputfile);

    {
        std::ifstream is("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(car.read(is), std::exception);
        std::ofstream os(tmp);
        Ensures(os.is_open());
        EnsuresNotThrow(car.write(os), std::exception);
    }

    efyj::Model car2;

    {
        std::ifstream is(tmp);
        Ensures(is.is_open());
        EnsuresNotThrow(car2.read(is), std::exception);
    }

#if defined(__unix__)
    unlink(tmp.c_str());
#else
    _unlink(tmp.c_str());
#endif

    Ensures(car == car2);
}

void test_Car_dxi()
{
    change_pwd();

    efyj::Model car;
    {
        std::ifstream is("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(car.read(is), std::exception);
    }

    Ensures(car.attributes.size() == 10u);
    Ensures(car.attributes[0].name == "CAR");
    Ensures(car.attributes[0].children.size() == 2u);
    Ensures(car.attributes[0].children == std::vector<std::size_t>({1, 4}));
    Ensures(car.attributes[1].name == "PRICE");
    Ensures(car.attributes[1].children.size() == 2u);
    Ensures(car.attributes[1].children == std::vector<std::size_t>({2, 3}));
    Ensures(car.attributes[2].name == "BUY.PRICE");
    Ensures(car.attributes[2].children.empty() == true);
    Ensures(car.attributes[3].name == "MAINT.PRICE");
    Ensures(car.attributes[3].children.empty() == true);
    Ensures(car.attributes[4].name == "TECH.CHAR.");
    Ensures(car.attributes[4].children.size() == 2u);
    Ensures(car.attributes[4].children == std::vector<std::size_t>({5, 9}));
    Ensures(car.attributes[5].name == "COMFORT");
    Ensures(car.attributes[5].children.size() == 3u);
    Ensures(car.attributes[5].children == std::vector<std::size_t>({6, 7, 8}));
    Ensures(car.attributes[6].name == "#PERS");
    Ensures(car.attributes[6].children.empty() == true);
    Ensures(car.attributes[7].name == "#DOORS");
    Ensures(car.attributes[7].children.empty() == true);
    Ensures(car.attributes[8].name == "LUGGAGE");
    Ensures(car.attributes[8].children.empty() == true);
    Ensures(car.attributes[9].name == "SAFETY");
    Ensures(car.attributes[9].children.empty() == true);
}

void test_multiple_Car_Model()
{
    change_pwd();

    efyj::Model src, dst;
    {
        std::ifstream is("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(src.read(is), std::exception);
    }
    {
        std::ifstream is("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(dst.read(is), std::exception);
    }
    Ensures(src == dst);
    efyj::Model dst2;
    {
        std::ifstream is("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(dst2.read(is), std::exception);
    }
    Ensures(dst2 == dst);
    dst2.attributes[0].name = "change";
    Ensures(dst2 != dst);
}

void test_solver_Car()
{
    change_pwd();

    efyj::Model model;
    {
        std::ifstream is("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(model.read(is), std::exception);
    }
    std::size_t problem_size = 1;
    std::size_t basic_scale_number = 0;
    std::size_t scale_number = 0;
    std::size_t scalevalue_number = 0;

    for (const auto &att : model.attributes) {
        if (att.is_basic()) {
            basic_scale_number++;
            scalevalue_number += att.scale.size();
            problem_size *= att.scale.size();
        }

        scale_number++;
    }

    Ensures(basic_scale_number == 6u);
    Ensures(scale_number == 10u);
    Ensures(scalevalue_number == 19u);
    Ensures(problem_size == 972u);
}

void test_basic_solver_for_Car()
{
    change_pwd();

    efyj::Model model;
    {
        std::ifstream is("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(model.read(is), std::exception);
    }

    efyj::Vector opt_v3(6);
    opt_v3 << 1, 2, 2, 2, 2, 2;
    efyj::Vector opt_v2(6);
    opt_v2 << 1, 1, 2, 2, 2, 1;
    efyj::Vector opt_v4(6);
    opt_v4 << 2, 2, 2, 3, 2, 2;
    efyj::Vector opt_v5(6);
    opt_v5 << 0, 0, 0, 0, 0, 0;

    {
        efyj::solver_stack s(model);
        Ensures(s.solve(opt_v3) == 3);
        Ensures(s.solve(opt_v2) == 2);
        Ensures(s.solve(opt_v4) == 3);
        Ensures(s.solve(opt_v5) == 0);
    }
}

void test_basic_solver_for_Enterprise()
{
    change_pwd();

    efyj::Model model;
    {
        std::ifstream is("Enterprise.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(model.read(is), std::exception);
    }
    efyj::Vector opt_v(12);
    opt_v << 2, 0, 0, 0, 2, 0, 0, 0, 1, 1, 1, 1;
    efyj::solver_stack ss(model);
    Ensures(ss.solve(opt_v) == 1);
}

void test_basic_solver_for_IPSIM_PV_simulation1_1()
{
    change_pwd();

    efyj::Model *model = new efyj::Model;
    {
        std::ifstream is("IPSIM_PV_simulation1-1.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(model->read(is), std::exception);
    }
    {
        efyj::Vector opt_v(14);
        opt_v << 2, 0, 0, 1, 0, 1, 1, 0, 0, 0, 2, 0, 0, 1;
        efyj::solver_stack ss(*model);
        Ensures(ss.solve(opt_v) == 6);
    }
    efyj::Model copy1(*model);
    delete model;
    {
        efyj::Vector opt_v(14);
        opt_v << 2, 0, 0, 1, 0, 1, 1, 0, 0, 0, 2, 0, 0, 1;
        efyj::solver_stack ss(copy1);
        Ensures(ss.solve(opt_v) == 6);
    }
}

void test_problem_Model_file()
{
    change_pwd();

    std::vector<std::string> filepaths = {"Car.dxi",
                                          "Employ.dxi",
                                          "Enterprise.dxi",
                                          "IPSIM_PV_simulation1-1.dxi"};

    for (const auto &filepath : filepaths) {
        std::cout << "run " << filepath << "\n";

        {
            efyj::Model model;

            std::ifstream ifs(filepath);
            model.read(ifs);

            std::ofstream ofs("/tmp/toto.csv");
            model.write_options(ofs);
        }

        efyj::efyj e(filepath, "/tmp/toto.csv");
        auto ret = e.compute_kappa();

        printf("===> %f\n", ret.kappa);

        Ensures(ret.kappa == 1.0);
        Ensures(ret.kappa_computed == 1);
    }
}

void check_the_options_set_function()
{
    change_pwd();

    efyj::Model model;

    {
        std::ifstream ifs("Car.dxi");
        model.read(ifs);

        std::ofstream ofs("/tmp/Car.csv");
        model.write_options(ofs);
    }

    std::vector<std::string> simulations_old;
    std::vector<std::string> places_old;
    std::vector<int> departments_old;
    std::vector<int> years_old;
    std::vector<int> observed_old;
    std::vector<int> options_old;

    {
        efyj::efyj e("Car.dxi", "/tmp/Car.csv");

        e.extract_options(simulations_old,
                          places_old,
                          departments_old,
                          years_old,
                          observed_old,
                          options_old);
    }

    efyj::Options options;

    {
        std::ifstream ifs("/tmp/Car.csv");
        options.read(std::make_shared<efyj::Context>(), ifs, model);
    }

    efyj::Array array_options_old = options.options;

    options.set(simulations_old,
                places_old,
                departments_old,
                years_old,
                observed_old,
                options_old);

    Ensures(options.options.rows() == array_options_old.rows());
    Ensures(options.options.cols() == array_options_old.cols());

    for (long int row = 0; row != options.options.rows(); ++row)
        for (long int col = 0; col != options.options.cols(); ++col)
            Ensures(options.options(row, col) == array_options_old(row, col));
}

void check_the_efyj_set_function()
{
    change_pwd();

    {
        efyj::Model model;
        std::ifstream ifs("Car.dxi");
        model.read(ifs);
        std::ofstream ofs("/tmp/Car.csv");
        model.write_options(ofs);
    }

    efyj::efyj e("Car.dxi", "/tmp/Car.csv");

    std::vector<std::string> simulations_old;
    std::vector<std::string> places_old;
    std::vector<int> departments_old;
    std::vector<int> years_old;
    std::vector<int> observed_old;
    std::vector<int> options_old;

    e.extract_options(simulations_old,
                      places_old,
                      departments_old,
                      years_old,
                      observed_old,
                      options_old);

    e.set_options(simulations_old,
                  places_old,
                  departments_old,
                  years_old,
                  observed_old,
                  options_old);

    std::vector<std::string> simulations;
    std::vector<std::string> places;
    std::vector<int> departments;
    std::vector<int> years;
    std::vector<int> observed;
    std::vector<int> options;

    e.extract_options(
        simulations, places, departments, years, observed, options);

    Ensures(simulations_old == simulations);
    Ensures(places_old == places);
    Ensures(departments_old == departments);
    Ensures(years_old == years);
    Ensures(observed_old == observed);
    Ensures(options_old == options);
}

void test_adjustment_solver_for_Car()
{
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

    efyj::efyj e("Car.dxi", "/tmp/Car.csv");

    {
        auto ret = e.compute_kappa();
        Ensures(ret.kappa == 1);
    }

    std::vector<std::string> simulations;
    std::vector<std::string> places;
    std::vector<int> departments;
    std::vector<int> years;
    std::vector<int> observed;
    std::vector<int> options;

    e.extract_options(
        simulations, places, departments, years, observed, options);

    Ensures(simulations.size() < options.size());
    Ensures(simulations.size() > 0);

    years[0] = 2000;
    years[1] = 2000;
    years[2] = 2001;
    years[3] = 2001;
    years[4] = 2002;
    years[5] = 2002;

    departments[0] = 59;
    departments[1] = 62;
    departments[2] = 59;
    departments[3] = 62;
    departments[4] = 59;
    departments[5] = 62;

    places[0] = "a";
    places[1] = "b";
    places[2] = "c";
    places[3] = "d";
    places[4] = "e";
    places[5] = "f";

    Ensures(model.attributes[0].scale.size() == 4);
    observed = {2, 1, 0, 0, 2, 2};
    e.set_options(simulations, places, departments, years, observed, options);
    {
        auto ret = e.compute_adjustment(4, -1, 1);
        Ensures(ret.size() == 5);

        EnsuresApproximatelyEqual(ret[0].kappa, 0.78, 0.1);
        EnsuresApproximatelyEqual(ret[1].kappa, 0.84, 0.1);
        EnsuresApproximatelyEqual(ret[2].kappa, 0.91, 0.1);
        EnsuresApproximatelyEqual(ret[3].kappa, 0.91, 0.1);
        EnsuresApproximatelyEqual(ret[4].kappa, 1, 0.1);
    }
}

void test_prediction_solver_for_Car()
{
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

    efyj::efyj e("Car.dxi", "/tmp/Car.csv");

    {
        auto ret = e.compute_kappa();
        Ensures(ret.kappa == 1);
    }

    std::vector<std::string> simulations;
    std::vector<std::string> places;
    std::vector<int> departments;
    std::vector<int> years;
    std::vector<int> observed;
    std::vector<int> options;

    e.extract_options(
        simulations, places, departments, years, observed, options);

    Ensures(simulations.size() < options.size());
    Ensures(simulations.size() > 0);

    years[0] = 2000;
    years[1] = 2000;
    years[2] = 2001;
    years[3] = 2001;
    years[4] = 2002;
    years[5] = 2002;

    departments[0] = 59;
    departments[1] = 62;
    departments[2] = 59;
    departments[3] = 62;
    departments[4] = 59;
    departments[5] = 62;

    places[0] = "a";
    places[1] = "b";
    places[2] = "c";
    places[3] = "d";
    places[4] = "e";
    places[5] = "f";

    Ensures(model.attributes[0].scale.size() == 4);
    observed = {3, 2, 0, 0, 3, 3};

    e.set_options(simulations, places, departments, years, observed, options);

    {
        auto ret = e.compute_prediction(1, -1, 1);

        Ensures(ret.size() == 2);
        Ensures(ret.front().kappa == 1);
        Ensures(ret.back().kappa == 1);
    }

    observed = {3, 2, 0, 0, 3, 2};
    e.set_options(simulations, places, departments, years, observed, options);
    {
        auto ret = e.compute_prediction(1, -1, 1);

        Ensures(ret.size() == 2);
        EnsuresApproximatelyEqual(ret.front().kappa, 0.95, 0.1);
        EnsuresApproximatelyEqual(ret.back().kappa, 0.89, 0.1);
    }
}

int main()
{
    test_empty_object_equality();
    test_empty_object_read_write();
    test_classic_Model_file();
    test_car_dxi_load_save_load_via_sstream();
    test_car_dxi_load_save_load_via_file();
    test_Car_dxi();
    test_multiple_Car_Model();
    test_solver_Car();
    test_basic_solver_for_Car();
    test_basic_solver_for_Enterprise();
    test_basic_solver_for_IPSIM_PV_simulation1_1();
    test_problem_Model_file();
    check_the_options_set_function();
    check_the_efyj_set_function();
    test_adjustment_solver_for_Car();
    test_prediction_solver_for_Car();

    return unit_test::report_errors();
}
