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

#include <filesystem>
#include <random>

#include "unit-test.hpp"

#include <cstdio>
#include <cstdlib>
#include <ctime>

#if defined(__unix__)
#include <unistd.h>
#elif defined(_WIN32)
#include <direct.h>
#include <io.h>
#include <tchar.h>
#endif

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

std::string
make_temporary(std::string name)
{
    static const char* names[] = { "TMPDIR", "TMP", "TEMP" };
    static const int names_size = sizeof(names) / sizeof(names[0]);
    std::string ret;

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
            ret += static_cast<char>('a' + distribution(generator));
        else
            ret += c;
    }

    return ret;
}

void
test_tokenize()
{
    std::vector<std::string> output;
    std::string s1 = "simulation;place;department;year;BUY.PRICE;MAINT.PRICE;#"
                     "PERS;#DOORS;LUGGAGE;SAFETY;CAR";

    efyj::tokenize(s1, output, ";", false);
    Ensures(output.size() == 11);

    std::string s2 = "Car1../;-;0;0;medium;low;more;4;big;high;exc";
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
    efyj::context ctx;
    efyj::Model x1, x2;

    {
        auto name = make_temporary("efyj-XXXXXXXX.dxi");
        auto file = std::filesystem::path(name);

        {
            efyj::output_file out(file.string().c_str());
            x1.write(ctx, out);
        }

        {
            efyj::input_file in(file.string().c_str());
            x2.read(ctx, in);
        }

        std::filesystem::remove(file);
    }

    bool is_equal = x1 == x2;

    Ensures(is_equal == true);
}

void
test_classic_Model_file()
{
    change_pwd();
    efyj::context ctx;

    std::vector<std::string> filepaths = {
        "Car.dxi",        "Employ.dxi",
        "Enterprise.dxi", "IPSIM_PV_simulation1-1.dxi",
        "Nursery.dxi",    "Shuttle.dxi"
    };

    std::string output;

    for (const auto& filepath : filepaths) {
        efyj::Model dex1, dex2;

        output = "XXXXXX" + filepath;
        output = make_temporary(output);

        {
            const auto is = efyj::input_file(filepath.c_str());
            Ensures(is.is_open());

            EnsuresNotThrow(dex1.read(ctx, is), std::exception);

            const auto os = efyj::output_file(output.c_str());
            Ensures(os.is_open());

            dex1.write(ctx, os);
        }

        {
            const auto is = efyj::input_file(output.c_str());
            Ensures(is.is_open());

            EnsuresNotThrow(dex2.read(ctx, is), std::exception);
        }

        Ensures(dex1 == dex2);
    }
}

void
test_car_dxi_load_save_load_via_sstream()
{
    change_pwd();
    efyj::context ctx;

    efyj::Model car;

    auto name = make_temporary("efyj-XXXXXXXX.dxi");
    auto file = std::filesystem::path(name);

    {
        efyj::input_file is("Car.dxi");
        EnsuresNotThrow(car.read(ctx, is), std::exception);

        efyj::output_file out(file.string().c_str());
        EnsuresNotThrow(car.write(ctx, out), std::exception);
    }

    efyj::Model car2;

    efyj::input_file is(file.string().c_str());
    EnsuresNotThrow(car2.read(ctx, is), std::exception);

    Ensures(car == car2);
}

void
test_car_dxi_load_save_load_via_file()
{
    change_pwd();
    efyj::context ctx;
    efyj::Model car;
    std::string outputfile("CarXXXXXXXX.dxi");
    auto tmp = make_temporary(outputfile);

    {
        const auto is = efyj::input_file("Car.dxi");
        Ensures(is.is_open());

        EnsuresNotThrow(car.read(ctx, is), std::exception);

        const auto os = efyj::output_file(tmp.c_str());
        Ensures(os.is_open());

        EnsuresNotThrow(car.write(ctx, os), std::exception);
    }

    efyj::Model car2;

    {
        const auto is = efyj::input_file(tmp.c_str());
        Ensures(is.is_open());
        EnsuresNotThrow(car2.read(ctx, is), std::exception);
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
    efyj::context ctx;

    efyj::Model car;
    {
        const auto is = efyj::input_file("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(car.read(ctx, is), std::exception);
    }

    Ensures(car.attributes.size() == 10u);
    Ensures(car.attributes[0].name == "CAR");
    Ensures(car.attributes[0].children.size() == 2u);
    Ensures(car.attributes[0].children == std::vector<std::size_t>({ 1, 4 }));
    Ensures(car.attributes[1].name == "PRICE");
    Ensures(car.attributes[1].children.size() == 2u);
    Ensures(car.attributes[1].children == std::vector<std::size_t>({ 2, 3 }));
    Ensures(car.attributes[2].name == "BUY.PRICE");
    Ensures(car.attributes[2].children.empty() == true);
    Ensures(car.attributes[3].name == "MAINT.PRICE");
    Ensures(car.attributes[3].children.empty() == true);
    Ensures(car.attributes[4].name == "TECH.CHAR.");
    Ensures(car.attributes[4].children.size() == 2u);
    Ensures(car.attributes[4].children == std::vector<std::size_t>({ 5, 9 }));
    Ensures(car.attributes[5].name == "COMFORT");
    Ensures(car.attributes[5].children.size() == 3u);
    Ensures(car.attributes[5].children ==
            std::vector<std::size_t>({ 6, 7, 8 }));
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
    efyj::context ctx;

    efyj::Model src, dst;
    {
        const auto is = efyj::input_file("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(src.read(ctx, is), std::exception);
    }
    {
        const auto is = efyj::input_file("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(dst.read(ctx, is), std::exception);
    }
    Ensures(src == dst);
    efyj::Model dst2;
    {
        const auto is = efyj::input_file("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(dst2.read(ctx, is), std::exception);
    }
    Ensures(dst2 == dst);
    dst2.attributes[0].name = "change";
    Ensures(dst2 != dst);
}

void
test_solver_Car()
{
    change_pwd();
    efyj::context ctx;
    efyj::Model model;

    {
        const auto is = efyj::input_file("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(model.read(ctx, is), std::exception);
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
    efyj::context ctx;

    efyj::Model model;
    {
        const auto is = efyj::input_file("Car.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(model.read(ctx, is), std::exception);
    }

    std::vector<int> opt_v3{ 1, 2, 2, 2, 2, 2 };
    std::vector<int> opt_v2{ 1, 1, 2, 2, 2, 1 };
    std::vector<int> opt_v4{ 2, 2, 2, 3, 2, 2 };
    std::vector<int> opt_v5{ 0, 0, 0, 0, 0, 0 };

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
    efyj::context ctx;

    efyj::Model model;
    {
        const auto is = efyj::input_file("Enterprise.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(model.read(ctx, is), std::exception);
    }
    std::vector<int> opt_v{ 2, 0, 0, 0, 2, 0, 0, 0, 1, 1, 1, 1 };
    efyj::solver_stack ss(model);
    Ensures(ss.solve(opt_v) == 1);
}

void
test_basic_solver_for_IPSIM_PV_simulation1_1()
{
    change_pwd();
    efyj::context ctx;

    efyj::Model* model = new efyj::Model;
    {
        const auto is = efyj::input_file("IPSIM_PV_simulation1-1.dxi");
        Ensures(is.is_open());
        EnsuresNotThrow(model->read(ctx, is), std::exception);
    }
    {
        std::vector<int> opt_v{ 2, 0, 0, 1, 0, 1, 1, 0, 0, 0, 2, 0, 0, 1 };
        efyj::solver_stack ss(*model);
        Ensures(ss.solve(opt_v) == 6);
    }

    efyj::Model copy1(*model);
    delete model;

    {
        std::vector<int> opt_v{ 2, 0, 0, 1, 0, 1, 1, 0, 0, 0, 2, 0, 0, 1 };
        efyj::solver_stack ss(copy1);
        Ensures(ss.solve(opt_v) == 6);
    }
}

void
test_problem_Model_file()
{
    change_pwd();
    efyj::context ctx;

    std::vector<std::string> filepaths = {
        "Car.dxi", "Employ.dxi", "Enterprise.dxi", "IPSIM_PV_simulation1-1.dxi"
    };

    std::string outputfilename("outputXXXXX.csv");

    for (const auto& filepath : filepaths) {
        efyj::status ret;
        auto output = make_temporary(outputfilename);

        ret = efyj::extract_options_to_file(ctx, filepath, output);
        Ensures(efyj::is_success(ret));

        efyj::evaluation_results eval;
        ret = efyj::evaluate(ctx, filepath, output, eval);
        Ensures(efyj::is_success(ret));

        Ensures(eval.linear_weighted_kappa == 1.0);
        Ensures(eval.squared_weighted_kappa == 1.0);
    }
}

void
check_the_options_set_function()
{
    change_pwd();
    efyj::context ctx;
    efyj::status ret;

    auto output = make_temporary("CarXXXXXXXX.dxi");

    ret = efyj::extract_options_to_file(ctx, "Car.dxi", output);
    Ensures(efyj::is_success(ret));

    efyj::data opt1;

    ret = efyj::extract_options(ctx, "Car.dxi", opt1);
    Ensures(efyj::is_success(ret));

    efyj::data opt2 = opt1;
    Ensures(opt1.rows() == opt2.rows());
    Ensures(opt1.cols() == opt2.cols());

    ret = efyj::merge_options(ctx, "Car.dxi", output, opt2);
    Ensures(efyj::is_success(ret));
    Ensures(opt1.simulations == opt2.simulations);
    Ensures(opt1.places == opt2.places);
    Ensures(opt1.departments == opt2.departments);
    Ensures(opt1.years == opt2.years);
    Ensures(opt1.observed == opt2.observed);
    Ensures(opt1.scale_values == opt2.scale_values);

    ret = efyj::extract_options(ctx, "Car.dxi", opt2);
    Ensures(efyj::is_success(ret));
    Ensures(opt1.simulations == opt2.simulations);
    Ensures(opt1.places == opt2.places);
    Ensures(opt1.departments == opt2.departments);
    Ensures(opt1.years == opt2.years);
    Ensures(opt1.observed == opt2.observed);
    Ensures(opt1.scale_values == opt2.scale_values);
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

    // std::vector<std::string> simulations_old;
    // std::vector<std::string> places_old;
    // std::vector<int> departments_old;
    // std::vector<int> years_old;
    // std::vector<int> observed_old;
    // std::vector<int> options_old;

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

    // std::vector<std::string> simulations;
    // std::vector<std::string> places;
    // std::vector<int> departments;
    // std::vector<int> years;
    // std::vector<int> observed;
    // std::vector<int> options;

    // e.extract_options(
    //   simulations, places, departments, years, observed, options);

    // Ensures(simulations_old == simulations);
    // Ensures(places_old == places);
    // Ensures(departments_old == departments);
    // Ensures(years_old == years);
    // Ensures(observed_old == observed);
    // Ensures(options_old == options);
}

static efyj::context
make_context() noexcept
{
    efyj::context ctx;

    ctx.out = nullptr;
    ctx.err = nullptr;
    ctx.line = 0;
    ctx.column = 0;
    ctx.size = 0;
    ctx.data_1.reserve(256u);
    ctx.status = efyj::status::success;
    ctx.log_priority = efyj::log_level::info;

    return ctx;
}

struct result_fn
{
private:
    std::vector<int>& all_modifiers;
    std::vector<double>& all_kappa;
    std::vector<double>& all_time;
    const int limit = 0;
    int current_limit = 0;

public:
    result_fn(std::vector<int>& all_modifiers_,
              std::vector<double>& all_kappa_,
              std::vector<double>& all_time_,
              const int limit_)
      : all_modifiers(all_modifiers_)
      , all_kappa(all_kappa_)
      , all_time(all_time_)
      , limit(limit_)
    {}

    bool operator()(const efyj::result& r)
    {
        try {
            for (const auto& elem : r.modifiers) {
                all_modifiers.emplace_back(elem.attribute);
                all_modifiers.emplace_back(elem.line);
                all_modifiers.emplace_back(elem.value);
            }

            all_kappa.emplace_back(r.kappa);
            all_time.emplace_back(r.time);

            ++current_limit;

            return current_limit < limit;
        } catch (...) {
            return false;
        }
    }
};

void
test_adjustment_solver_for_Car()
{
    auto ctx = make_context();

    efyj::data d;

    auto ret = efyj::extract_options(ctx, "Car.dxi", d);
    Ensures(is_success(ret));

    std::vector<int> all_modifiers;
    std::vector<double> all_kappa;
    std::vector<double> all_time;
    result_fn fn(all_modifiers, all_kappa, all_time, 4);

    ret = efyj::adjustment(ctx, "Car.dxi", d, fn, true, 4, 1u);
    Ensures(is_success(ret));

    const std::vector<int> to_compare = { 1, 0, 0, 1, 0, 0, 1, 4, 1,
                                          1, 0, 0, 1, 4, 1, 1, 5, 1 };

    Ensures(to_compare.size() == all_modifiers.size());
    for (size_t i = 0, e = to_compare.size(); i < e; ++i)
        Ensures(to_compare[i] == all_modifiers[i]);

    Ensures(all_kappa.size() == (size_t)4);
    Ensures(all_kappa[0] == 1.0);
    Ensures(all_kappa[1] == 1.0);
    Ensures(all_kappa[2] == 1.0);
    Ensures(all_kappa[3] == 1.0);
}

void
test_adjustment_solver_for_Car2()
{
    auto ctx = make_context();

    efyj::data d;

    auto ret = efyj::extract_options(ctx, "Car2.dxi", d);
    Ensures(is_success(ret));

    std::vector<int> all_modifiers;
    std::vector<double> all_kappa;
    std::vector<double> all_time;
    result_fn fn(all_modifiers, all_kappa, all_time, 2);

    ret = efyj::adjustment(ctx, "Car2.dxi", d, fn, true, 2, 1u);
    Ensures(is_success(ret));

    const std::vector<int> to_compare = { 4, 8, 3 };

    Ensures(to_compare.size() == all_modifiers.size());
    for (size_t i = 0, e = all_modifiers.size(); i < e; ++i)
        Ensures(to_compare[i] == all_modifiers[i]);

    Ensures(all_kappa.size() == (size_t)2);
    Ensures(all_kappa[0] < 1.0);
    Ensures(all_kappa[1] == 1.0);
}

void
test_prediction_solver_for_Car()
{
    auto ctx = make_context();

    efyj::data d;

    auto ret = efyj::extract_options(ctx, "Car.dxi", d);
    Ensures(is_success(ret));

    std::vector<int> all_modifiers;
    std::vector<double> all_kappa;
    std::vector<double> all_time;
    result_fn fn(all_modifiers, all_kappa, all_time, 4);

    d.years[0] = 1990;
    d.years[1] = 1990;
    d.departments[0] = 81;
    d.departments[1] = 81;
    d.places[0] = "Auzeville";
    d.places[1] = "Auzeville";

    ret = efyj::prediction(ctx, "Car.dxi", d, fn, true, 4, 1u);
    Ensures(is_success(ret));

    const std::vector<int> to_compare = { 1, 0, 0, 1, 0, 0, 1, 4, 1,
                                          1, 0, 0, 1, 4, 1, 1, 5, 1 };

    Ensures(to_compare.size() == all_modifiers.size());
    for (size_t i = 0, e = to_compare.size(); i < e; i += 3)
        Ensures(to_compare[i] == all_modifiers[i]);

    Ensures(all_kappa.size() == (size_t)4);
    Ensures(all_kappa[0] <= 1.0);
    Ensures(all_kappa[1] <= 1.0);
    Ensures(all_kappa[2] <= 1.0);
    Ensures(all_kappa[3] <= 1.0);
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
    test_adjustment_solver_for_Car2();
    test_prediction_solver_for_Car();

    return unit_test::report_errors();
}
