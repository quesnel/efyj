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

#include <efyj/efyj.hpp>

#include "model.hpp"
#include "options.hpp"
#include "post.hpp"
#include "solver-stack.hpp"
#include "utils.hpp"

#include <iostream>
#include <random>

#include <cstdio>
#include <cstdlib>
#include <ctime>

#if defined(__unix__)
#include <unistd.h>
#elif defined(__WIN32__)
#include <direct.h>
#include <io.h>
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

    return new uint8_t[size];
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
#ifndef NDEBUG
    // fprintf(stderr,
    //         "%zu (alignment: %zu offset: %zu) in %s (flags: %d debug flags:
    //         "
    //         "%u) from file %s:%d\n",
    //         size,
    //         alignment,
    //         alignmentOffset,
    //         pName,
    //         flags,
    //         debugFlags,
    //         file,
    //         line);
#endif

    if ((alignmentOffset % alignment) == 0)
        return new uint8_t[size];

    return new uint8_t[size];
}

#include "unit-test.hpp"

static inline bool
change_pwd()
{
#if defined(__unix__)
    int ret = ::chdir(EXAMPLES_DIR);
#else
    int ret = _chdir(EXAMPLES_DIR);
#endif

    return ret == 0;
}

eastl::shared_ptr<efyj::context>
make_context()
{
    auto ret = efyj::make_context(7);

    return ret;
}

eastl::string
make_temporary(eastl::string name)
{
    static const char* names[] = { "TMPDIR", "TMP", "TEMP" };
    static const int names_size = sizeof(names) / sizeof(names[0]);
    eastl::string ret;

    for (int i = 0; i != names_size && ret.empty(); ++i)
        if (::getenv(names[i]))
            ret = ::getenv(names[i]);

    if (ret.empty())
#ifdef __unix__
        ret = "/tmp/";
#else
        ret = "./";
#endif
    else
        ret += '/';

    std::minstd_rand generator(static_cast<unsigned int>(time(nullptr)));
    std::uniform_int_distribution<int> distribution(0, 25);

    for (auto c : name) {
        if (c == 'X')
            ret += 'a' + distribution(generator);
        else
            ret += c;
    }

    return ret;
}

void
test_tokenize()
{
    eastl::vector<eastl::string> output;
    eastl::string s1 =
      "simulation;place;department;year;BUY.PRICE;MAINT.PRICE;#"
      "PERS;#DOORS;LUGGAGE;SAFETY;CAR";

    efyj::tokenize(s1, output, ";", false);
    Ensures(output.size() == 11);

    eastl::string s2 = "Car1../;-;0;0;medium;low;more;4;big;high;exc";
    efyj::tokenize(s1, output, ";", false);

    Ensures(output.size() == 11);
}

void
test_matrix()
{
    efyj::matrix<double> m(2, 2);
    m(0, 0) = 3;
    m(1, 0) = 2;
    m(0, 1) = -1;
    m(1, 1) = m(1, 0) + m(0, 1);

    Ensures(m(0, 0) == 3);
    Ensures(m(0, 1) == -1);
    Ensures(m(1, 0) == 2);
    Ensures(m(1, 1) == 1);

    Ensures(*m.begin() == 3);
    Ensures(*(m.begin() + 1) == -1);
    Ensures(*(m.begin() + 2) == 2);
    Ensures(*(m.begin() + 3) == 1);
}

void
test_matrix_multiplcation()
{
    efyj::matrix<double> m1(2, 2);
    m1.assign({ 1, 2, 3, 4 });

    efyj::matrix<double> m2(2, 2);
    m2.assign({ 5, 6, 7, 8 });

    efyj::matrix<double> product(m1.rows(), m2.columns(), 0.0);

    for (size_t row = 0; row != m1.rows(); ++row)
        for (size_t col = 0; col != m2.columns(); ++col)
            for (size_t i = 0; i != m1.columns(); ++i)
                product(row, col) += m1(row, i) * m2(i, col);

    Ensures(product(0, 0) == 19);
    Ensures(product(0, 1) == 22);
    Ensures(product(1, 0) == 43);
    Ensures(product(1, 1) == 50);

    Ensures(mult_and_sum(m1, m2) == 70);
}

void
test_matrix_multiplcation_2()
{
    efyj::matrix<double> m1(3, 3);
    m1.assign({ 1, 2, 3, 2, 3, 1, 3, 1, 2 });

    efyj::matrix<double> m2(3, 3);
    m2.assign({ 1, 2, 3, 2, 3, 1, 3, 1, 2 });

    efyj::matrix<double> product(m1.rows(), m2.columns(), 0.0);

    for (size_t row = 0; row != m1.rows(); ++row)
        for (size_t col = 0; col != m2.columns(); ++col)
            for (size_t i = 0; i != m1.columns(); ++i)
                product(row, col) += m1(row, i) * m2(i, col);

    Ensures(product(0, 0) == 14);
    Ensures(product(0, 1) == 11);
    Ensures(product(0, 2) == 11);
    Ensures(product(1, 0) == 11);
    Ensures(product(1, 1) == 14);
    Ensures(product(1, 2) == 11);
    Ensures(product(2, 0) == 11);
    Ensures(product(2, 1) == 11);
    Ensures(product(2, 2) == 14);

    Ensures(mult_and_sum(m1, m2) == 39);
}

void
test_empty_object_equality()
{
    efyj::Model x1;
    efyj::Model x2;

    Ensures(x1 == x2);
}

void
test_empty_object_read_write()
{
#ifdef __unix__
    efyj::Model x1, x2;

    {
        char* ptr{ nullptr };
        size_t size{ 0 };

        efyj::scope_exit se_ptr([ptr]() { free(ptr); });

        {
            {
                auto* out = open_memstream(&ptr, &size);
                Ensures(out);
                efyj::scope_exit se_out([out]() { fclose(out); });

                x1.write(out);
            }

            Ensures(ptr);
        }

        {
            auto* in = fmemopen(ptr, strlen(ptr), "r");

            Ensures(in);
            efyj::scope_exit se_in([in]() { fclose(in); });

            x2.read(in);
        }
    }

    bool is_equal = x1 == x2;
    Ensures(is_equal == true);
#endif
}

void
test_classic_Model_file()
{
    change_pwd();

    eastl::vector<eastl::string> filepaths = {
        "Car.dxi",        "Employ.dxi",
        "Enterprise.dxi", "IPSIM_PV_simulation1-1.dxi",
        "Nursery.dxi",    "Shuttle.dxi"
    };

    eastl::string output;

    for (const auto& filepath : filepaths) {
        efyj::Model dex1, dex2;

        output = "XXXXXX" + filepath;
        output = make_temporary(output);

        {
            auto* is = fopen(filepath.c_str(), "r");
            Ensures(is);

            efyj::scope_exit se_is([is]() { fclose(is); });

            EnsuresNotThrow(dex1.read(is), std::exception);

            auto* os = fopen(output.c_str(), "w");
            Ensures(os);

            efyj::scope_exit se_os([os]() { fclose(os); });
            dex1.write(os);
        }

        {
            auto* is = fopen(output.c_str(), "r");
            Ensures(is);
            efyj::scope_exit se_is([is]() { fclose(is); });

            EnsuresNotThrow(dex2.read(is), std::exception);
        }

        Ensures(dex1 == dex2);
    }
}

void
test_car_dxi_load_save_load_via_sstream()
{
#ifdef __unix__
    change_pwd();

    efyj::Model car;
    char* ptr{ nullptr };
    size_t size{ 0 };

    {
        auto* is = fopen("Car.dxi", "r");
        Ensures(is);
        efyj::scope_exit se_is([is]() { fclose(is); });

        EnsuresNotThrow(car.read(is), std::exception);

        {
            auto* out = open_memstream(&ptr, &size);
            Ensures(out);
            efyj::scope_exit se_out([out]() { fclose(out); });

            EnsuresNotThrow(car.write(out), std::exception);
        }
    }

    efyj::Model car2;

    Ensures(ptr);

    auto* is = fmemopen(ptr, size, "r");
    Ensures(is);
    EnsuresNotThrow(car2.read(is), std::exception);
    free(ptr);

    Ensures(car == car2);
#endif
}

void
test_car_dxi_load_save_load_via_file()
{
    change_pwd();

    efyj::Model car;
    eastl::string outputfile("CarXXXXXXXX.dxi");
    auto tmp = make_temporary(outputfile);

    {
        auto* is = fopen("Car.dxi", "r");
        Ensures(is);
        efyj::scope_exit se_is([is]() { fclose(is); });

        EnsuresNotThrow(car.read(is), std::exception);

        auto* os = fopen(tmp.c_str(), "w");
        Ensures(os);
        efyj::scope_exit se_os([os]() { fclose(os); });

        EnsuresNotThrow(car.write(os), std::exception);
    }

    efyj::Model car2;

    {
        auto* is = fopen(tmp.c_str(), "r");
        Ensures(is);
        efyj::scope_exit se_is([is]() { fclose(is); });
        EnsuresNotThrow(car2.read(is), std::exception);
    }

#if defined(__unix__)
    unlink(tmp.c_str());
#else
    _unlink(tmp.c_str());
#endif

    Ensures(car == car2);
}

void
test_Car_dxi()
{
    change_pwd();

    efyj::Model car;
    {
        auto* is = fopen("Car.dxi", "r");
        Ensures(is);
        efyj::scope_exit se_is([is]() { fclose(is); });

        EnsuresNotThrow(car.read(is), std::exception);
    }

    Ensures(car.attributes.size() == 10u);
    Ensures(car.attributes[0].name == "CAR");
    Ensures(car.attributes[0].children.size() == 2u);
    Ensures(car.attributes[0].children ==
            eastl::vector<std::size_t>({ 1, 4 }));
    Ensures(car.attributes[1].name == "PRICE");
    Ensures(car.attributes[1].children.size() == 2u);
    Ensures(car.attributes[1].children ==
            eastl::vector<std::size_t>({ 2, 3 }));
    Ensures(car.attributes[2].name == "BUY.PRICE");
    Ensures(car.attributes[2].children.empty() == true);
    Ensures(car.attributes[3].name == "MAINT.PRICE");
    Ensures(car.attributes[3].children.empty() == true);
    Ensures(car.attributes[4].name == "TECH.CHAR.");
    Ensures(car.attributes[4].children.size() == 2u);
    Ensures(car.attributes[4].children ==
            eastl::vector<std::size_t>({ 5, 9 }));
    Ensures(car.attributes[5].name == "COMFORT");
    Ensures(car.attributes[5].children.size() == 3u);
    Ensures(car.attributes[5].children ==
            eastl::vector<std::size_t>({ 6, 7, 8 }));
    Ensures(car.attributes[6].name == "#PERS");
    Ensures(car.attributes[6].children.empty() == true);
    Ensures(car.attributes[7].name == "#DOORS");
    Ensures(car.attributes[7].children.empty() == true);
    Ensures(car.attributes[8].name == "LUGGAGE");
    Ensures(car.attributes[8].children.empty() == true);
    Ensures(car.attributes[9].name == "SAFETY");
    Ensures(car.attributes[9].children.empty() == true);
}

void
test_multiple_Car_Model()
{
    change_pwd();

    efyj::Model src, dst;
    {
        auto* is = fopen("Car.dxi", "r");
        Ensures(is);
        efyj::scope_exit se_is([is]() { fclose(is); });
        EnsuresNotThrow(src.read(is), std::exception);
    }
    {
        auto* is = fopen("Car.dxi", "r");
        Ensures(is);
        efyj::scope_exit se_is([is]() { fclose(is); });
        EnsuresNotThrow(dst.read(is), std::exception);
    }
    Ensures(src == dst);
    efyj::Model dst2;
    {
        auto* is = fopen("Car.dxi", "r");
        Ensures(is);
        efyj::scope_exit se_is([is]() { fclose(is); });
        EnsuresNotThrow(dst2.read(is), std::exception);
    }
    Ensures(dst2 == dst);
    dst2.attributes[0].name = "change";
    Ensures(dst2 != dst);
}

void
test_solver_Car()
{
    change_pwd();

    efyj::Model model;
    {
        auto* is = fopen("Car.dxi", "r");
        Ensures(is);
        efyj::scope_exit se_is([is]() { fclose(is); });
        EnsuresNotThrow(model.read(is), std::exception);
    }
    std::size_t problem_size = 1;
    std::size_t basic_scale_number = 0;
    std::size_t scale_number = 0;
    std::size_t scalevalue_number = 0;

    for (const auto& att : model.attributes) {
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

void
test_basic_solver_for_Car()
{
    change_pwd();

    efyj::Model model;
    {
        auto* is = fopen("Car.dxi", "r");
        Ensures(is);
        efyj::scope_exit se_is([is]() { fclose(is); });
        EnsuresNotThrow(model.read(is), std::exception);
    }

    eastl::vector<int> opt_v3{ 1, 2, 2, 2, 2, 2 };
    eastl::vector<int> opt_v2{ 1, 1, 2, 2, 2, 1 };
    eastl::vector<int> opt_v4{ 2, 2, 2, 3, 2, 2 };
    eastl::vector<int> opt_v5{ 0, 0, 0, 0, 0, 0 };

    {
        efyj::solver_stack s(model);
        Ensures(s.solve(opt_v3) == 3);
        Ensures(s.solve(opt_v2) == 2);
        Ensures(s.solve(opt_v4) == 3);
        Ensures(s.solve(opt_v5) == 0);
    }
}

void
test_basic_solver_for_Enterprise()
{
    change_pwd();

    efyj::Model model;
    {
        auto* is = fopen("Enterprise.dxi", "r");
        Ensures(is);
        efyj::scope_exit se_is([is]() { fclose(is); });
        EnsuresNotThrow(model.read(is), std::exception);
    }
    eastl::vector<int> opt_v{ 2, 0, 0, 0, 2, 0, 0, 0, 1, 1, 1, 1 };
    efyj::solver_stack ss(model);
    Ensures(ss.solve(opt_v) == 1);
}

void
test_basic_solver_for_IPSIM_PV_simulation1_1()
{
    change_pwd();

    efyj::Model* model = new efyj::Model;
    {
        auto* is = fopen("IPSIM_PV_simulation1-1.dxi", "r");
        Ensures(is);
        efyj::scope_exit se_is([is]() { fclose(is); });
        EnsuresNotThrow(model->read(is), std::exception);
    }
    {
        eastl::vector<int> opt_v{ 2, 0, 0, 1, 0, 1, 1, 0, 0, 0, 2, 0, 0, 1 };
        efyj::solver_stack ss(*model);
        Ensures(ss.solve(opt_v) == 6);
    }

    efyj::Model copy1(*model);
    delete model;

    {
        eastl::vector<int> opt_v{ 2, 0, 0, 1, 0, 1, 1, 0, 0, 0, 2, 0, 0, 1 };
        efyj::solver_stack ss(copy1);
        Ensures(ss.solve(opt_v) == 6);
    }
}

void
test_problem_Model_file()
{
    change_pwd();

    eastl::vector<eastl::string> filepaths = {
        "Car.dxi", "Employ.dxi", "Enterprise.dxi", "IPSIM_PV_simulation1-1.dxi"
    };

    eastl::string outputfilename("outputXXXXX.csv");
    auto output = make_temporary(outputfilename);

    for (const auto& filepath : filepaths) {

        {
            printf("read %s\n", filepath.c_str());
            efyj::Model model;

            auto* ifs = fopen(filepath.c_str(), "r");
            Ensures(ifs);
            efyj::scope_exit se_ifs([ifs]() { fclose(ifs); });

            model.read(ifs);

            printf("write %s\n", output.c_str());
            auto* ofs = fopen(output.c_str(), "w");
            Ensures(ofs);

            model.write_options(ofs);
            efyj::scope_exit se_ofs([ofs]() { fclose(ofs); });
        }

        printf("%s [%s]\n", filepath.c_str(), output.c_str());

        auto results = efyj::evaluate(make_context(), filepath, output);

        printf("squared: %g linear: %g\n",
               results.squared_weighted_kappa,
               results.linear_weighted_kappa);

        Ensures(results.linear_weighted_kappa == 1.0);
        Ensures(results.squared_weighted_kappa == 1.0);
    }
}

void
check_the_options_set_function()
{
    change_pwd();

    auto output = make_temporary("CarXXXXXXXX.dxi");

    {
        efyj::Model model;
        auto* ifs = fopen("Car.dxi", "r");
        Ensures(ifs);
        efyj::scope_exit se_ifs([ifs]() { fclose(ifs); });

        model.read(ifs);

        auto* ofs = fopen(output.c_str(), "w");
        Ensures(ofs);
        efyj::scope_exit se_ofs([ofs]() { fclose(ofs); });

        model.write_options(ofs);
    }

    efyj::options_data model_file, options_file, options_;

    auto opt1 = efyj::extract_options(make_context(), "Car.dxi");
    auto opt2 = efyj::extract_options(make_context(), "Car.dxi", output);

    Ensures(opt1.options.rows() == opt2.options.rows());
    Ensures(opt1.options.columns() == opt2.options.columns());

    Ensures(opt1.simulations == opt2.simulations);
    Ensures(opt1.places == opt2.places);
    Ensures(opt1.departments == opt2.departments);
    Ensures(opt1.years == opt2.years);
    Ensures(opt1.observed == opt2.observed);

    for (std::size_t row = 0; row != opt1.options.rows(); ++row)
        for (std::size_t col = 0; col != opt1.options.columns(); ++col)
            Ensures(opt1.options(row, col) == opt2.options(row, col));
}

void
check_the_efyj_set_function()
{
    // change_pwd();

    // auto output = make_temporary("CarXXXXXXXX.dxi");

    // {
    //     efyj::Model model;
    //     std::ifstream ifs("Car.dxi");
    //     model.read(ifs);
    //     std::ofstream ofs(output);
    //     model.write_options(ofs);
    // }

    // efyj::efyj e("Car.dxi", output);

    // eastl::vector<eastl::string> simulations_old;
    // eastl::vector<eastl::string> places_old;
    // eastl::vector<int> departments_old;
    // eastl::vector<int> years_old;
    // eastl::vector<int> observed_old;
    // eastl::vector<int> options_old;

    // e.extract_options(simulations_old,
    //                   places_old,
    //                   departments_old,
    //                   years_old,
    //                   observed_old,
    //                   options_old);

    // e.set_options(simulations_old,
    //               places_old,
    //               departments_old,
    //               years_old,
    //               observed_old,
    //               options_old);

    // eastl::vector<eastl::string> simulations;
    // eastl::vector<eastl::string> places;
    // eastl::vector<int> departments;
    // eastl::vector<int> years;
    // eastl::vector<int> observed;
    // eastl::vector<int> options;

    // e.extract_options(
    //     simulations, places, departments, years, observed, options);

    // Ensures(simulations_old == simulations);
    // Ensures(places_old == places);
    // Ensures(departments_old == departments);
    // Ensures(years_old == years);
    // Ensures(observed_old == observed);
    // Ensures(options_old == options);
}

void
test_adjustment_solver_for_Car()
{
    // change_pwd();

    // efyj::Model model;

    // {
    //     std::ifstream ifs("Car.dxi");
    //     model.read(ifs);
    // }

    // eastl::string output = make_temporary("CarXXXXXXXX.csv");

    // {
    //     std::ofstream ofs(output);
    //     model.write_options(ofs);
    // }

    // efyj::efyj e("Car.dxi", output);

    // {
    //     auto ret = e.compute_kappa();
    //     Ensures(ret.kappa == 1);
    // }

    // eastl::vector<eastl::string> simulations;
    // eastl::vector<eastl::string> places;
    // eastl::vector<int> departments;
    // eastl::vector<int> years;
    // eastl::vector<int> observed;
    // eastl::vector<int> options;

    // e.extract_options(
    //     simulations, places, departments, years, observed, options);

    // Ensures(simulations.size() < options.size());
    // Ensures(simulations.size() > 0);

    // years[0] = 2000;
    // years[1] = 2000;
    // years[2] = 2001;
    // years[3] = 2001;
    // years[4] = 2002;
    // years[5] = 2002;

    // departments[0] = 59;
    // departments[1] = 62;
    // departments[2] = 59;
    // departments[3] = 62;
    // departments[4] = 59;
    // departments[5] = 62;

    // places[0] = "a";
    // places[1] = "b";
    // places[2] = "c";
    // places[3] = "d";
    // places[4] = "e";
    // places[5] = "f";

    // Ensures(model.attributes[0].scale.size() == 4);
    // observed = {2, 1, 0, 0, 2, 2};
    // e.set_options(simulations, places, departments, years, observed,
    // options);
    // {
    //     auto ret = e.compute_adjustment(4, -1, 1);
    //     Ensures(ret.size() == 5);

    //     EnsuresApproximatelyEqual(ret[0].kappa, 0.78, 0.1);
    //     EnsuresApproximatelyEqual(ret[1].kappa, 0.84, 0.1);
    //     EnsuresApproximatelyEqual(ret[2].kappa, 0.91, 0.1);
    //     EnsuresApproximatelyEqual(ret[3].kappa, 0.91, 0.1);
    //     EnsuresApproximatelyEqual(ret[4].kappa, 1, 0.1);
    // }
}

void
test_prediction_solver_for_Car()
{
    // change_pwd();

    // efyj::Model model;

    // {
    //     std::ifstream ifs("Car.dxi");
    //     Ensures(ifs.is_open());

    //     model.read(ifs);
    // }

    // eastl::string output = make_temporary("CarXXXXXXXX.csv");

    // {
    //     std::ofstream ofs(output);
    //     Ensures(ofs.is_open());

    //     model.write_options(ofs);
    // }

    // efyj::efyj e("Car.dxi", output);

    // {
    //     auto ret = e.compute_kappa();
    //     Ensures(ret.kappa == 1);
    // }

    // eastl::vector<eastl::string> simulations;
    // eastl::vector<eastl::string> places;
    // eastl::vector<int> departments;
    // eastl::vector<int> years;
    // eastl::vector<int> observed;
    // eastl::vector<int> options;

    // e.extract_options(
    //     simulations, places, departments, years, observed, options);

    // Ensures(simulations.size() < options.size());
    // Ensures(simulations.size() > 0);

    // years[0] = 2000;
    // years[1] = 2000;
    // years[2] = 2001;
    // years[3] = 2001;
    // years[4] = 2002;
    // years[5] = 2002;

    // departments[0] = 59;
    // departments[1] = 62;
    // departments[2] = 59;
    // departments[3] = 62;
    // departments[4] = 59;
    // departments[5] = 62;

    // places[0] = "a";
    // places[1] = "b";
    // places[2] = "c";
    // places[3] = "d";
    // places[4] = "e";
    // places[5] = "f";

    // Ensures(model.attributes[0].scale.size() == 4);
    // observed = {3, 2, 0, 0, 3, 3};

    // e.set_options(simulations, places, departments, years, observed,
    // options);

    // {
    //     auto ret = e.compute_prediction(1, -1, 1);

    //     Ensures(ret.size() == 2);
    //     Ensures(ret.front().kappa == 1);
    //     Ensures(ret.back().kappa == 1);
    // }

    // observed = {3, 2, 0, 0, 3, 2};
    // e.set_options(simulations, places, departments, years, observed,
    // options);
    // {
    //     auto ret = e.compute_prediction(1, -1, 1);

    //     Ensures(ret.size() == 2);
    //     EnsuresApproximatelyEqual(ret.front().kappa, 0.95, 0.1);
    //     EnsuresApproximatelyEqual(ret.back().kappa, 0.89, 0.1);
    // }
}

int
main()
{
    test_tokenize();
    test_matrix();
    test_matrix_multiplcation();
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
