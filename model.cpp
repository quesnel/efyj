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

#include "model.hpp"
#include "log.hpp"
#include "print.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <gmpxx.h>
#include <cmath>

#pragma GCC diagnostic ignored "-Wdeprecated-register"
#include <Eigen/Core>
#include <ctime>

namespace efyj {

    void attribute::fill_utility_function()
    {
        std::vector <scale_id> child_max_value(children.size(), 0u);
        std::transform(children.begin(), children.end(),
                       child_max_value.begin(),
                       std::mem_fn(&attribute::scale_size));

        std::vector <std::size_t> iter(children.size(), 0u);
        bool end = false;

        std::size_t id = 0;
        do {
            std::size_t key = 0;
            for (std::size_t i = 0, e = iter.size(); i != e; ++i) {
                key *= 10;
                key += iter[i];
            }
            utility_function.emplace(key, functions.low[id] - '0');

            std::size_t current = child_max_value.size() - 1;

            do {
                iter[current]++;
                if (iter[current] >= child_max_value[current]) {
                    iter[current] = 0;

                    if (current == 0) {
                        end = true;
                        break;
                    } else {
                        --current;
                    }
                } else
                    break;
            } while (not end);
            ++id;
        } while (not end);
    }

    void dexi::init()
    {
        for (auto& att : attributes)
            if (not att.is_basic())
                att.fill_utility_function();
    }

    bool operator==(const attribute& lhs, const attribute& rhs)
    {
        return lhs.name == rhs.name;
    }

    bool operator==(const dexi& lhs, const dexi& rhs)
    {
        if (not (lhs.options == rhs.options &&
                 lhs.group == rhs.group &&
                 lhs.attributes == rhs.attributes))
            return false;

        if (lhs.child and rhs.child)
            return (*lhs.child) == (*rhs.child);

        if (lhs.child == nullptr and rhs.child == nullptr)
            return true;

        return false;
    }

    bool operator!=(const dexi& lhs, const dexi& rhs)
    {
        return not (lhs == rhs);
    }

    struct utility_function
    {
        std::string value;
        scale_id scale_value_size;

        utility_function(const std::string& value,
                         unsigned int scale_value_size)
            : value(value.size(), '0')
              , scale_value_size(scale_value_size)
        {
        }

        int next()
        {
            std::size_t current = value.size() - 1;

            for (;;) {
                value[current]++;
                if ((unsigned int)(value[current]) >= '0' + scale_value_size) {
                    value[current] = '0';

                    if (current == 0) {
                        return 0;
                    } else {
                        current--;
                    }
                } else
                    return -1;
            }
        }

        void init()
        {
            value.replace(0, value.size(), value.size(), '0');
        }
    };

    // struct flat_utility_function
    // {
    //     std::vector <mpz_class> uf_scale_size;
    //     std::vector <std::uint8_t> uf_attribute_size;
    //     mpz_class value;
    //     mpz_class min;
    //     mpz_class max;
    //     std::size_t fu_size;
    //     int processor, id;

    //     flat_utility_function(const std::vector <std::uint8_t>& scale_sizes,
    //                           const std::vector <std::uint8_t>& attribute_size,
    //                           int processor, int id)
    //         : fu_size(std::accumulate(attribute_size.begin(),
    //                                   attribute_size.end(), 0))
    //           , processor(processor)
    //           , id(id)
    //     {
    //         max = 1;
    //         mpz_class tmp;
    //         for (std::size_t i = 0; i < scale_sizes.size(); ++i) {
    //             mpz_ui_pow_ui(tmp.get_mpz_t(), scale_sizes[i], attribute_size[i]);
    //             max *= tmp;
    //         }

    //         if (processor == 1) {
    //             min = 0;
    //         } else {
    //             mpz_class lenght = max / processor;
    //             min = lenght * id;
    //             value = min;
    //             max = (lenght * (id + 1)) - 1;
    //         }

    //         std::clog << "\n"
    //             << "- flat_utility_function fu_size: " << fu_size << "\n"
    //             << " \\- id=" << id << " processor=" << processor << "\n"
    //             << "   - start calculation at =" << min.get_str() << "\n"
    //             << "   - end   calculation at =" << max.get_str() << "\n";

    //         std::uintmax_t modelsz = 1;
    //         for (std::size_t i = 0; i < attribute_size.size(); ++i) {
    //             std::uintmax_t sz = std::pow(scale_sizes[i], attribute_size[i]);
    //             modelsz *= sz;
    //             std::clog << (std::uintmax_t)scale_sizes[i] << "^"
    //                 << (std::uintmax_t)attribute_size[i] << " * ";
    //         }

    //         std::clog << "\nproblem size: " << modelsz << "\n";
    //         uf_scale_size.resize(fu_size, 0);

    //         auto it = uf_scale_size.begin();
    //         for (std::size_t i = 0; i < attribute_size.size(); ++i)
    //             it = std::fill_n(it, attribute_size[i], scale_sizes[i]);

    //         for (size_t i = 0; i < uf_scale_size.size(); ++i)
    //             std::clog << uf_scale_size[i].get_ui();

    //         std::clog << "\n";
    //     }

    //     void init()
    //     {
    //         value = min;
    //     }

    //     bool next()
    //     {
    //         return ++value < max;
    //     }

    //     std::string to_string() const
    //     {
    //         std::string ret(fu_size, '0');

    //         mpz_class tmp = value;
    //         mpz_class quo, rest;

    //         int i = fu_size - 1;

    //         while (tmp) {
    //             mpz_fdiv_qr(quo.get_mpz_t(), rest.get_mpz_t(), tmp.get_mpz_t(),
    //                         uf_scale_size[i].get_mpz_t());

    //             ret[i] += rest.get_ui();
    //             tmp = quo;
    //             --i;
    //         }

    //         return std::move(ret);
    //     }
    // };

    // int compute_flat_model(dexi& dex, int processor, int id)
    // {
    //     std::vector <std::uint8_t> fu_ss, fu_as;
    //     fu_ss.reserve(dex.attributes.size());
    //     fu_as.reserve(dex.attributes.size());

    //     for (auto& att : dex.attributes) {
    //         if (not att.is_basic()) {
    //             fu_ss.emplace_back(att.scale_size());
    //             fu_as.emplace_back(att.functions.low.size());
    //         }
    //     }

    //     flat_utility_function fu(fu_ss, fu_as, processor, id);
    //     std::size_t max = 0;

    //     std::chrono::time_point <std::chrono::system_clock> start =
    //         std::chrono::system_clock::now();

    //     while (fu.next()) {
    //         max++;

    //         if ((max % 123454321) == 0) {
    //             std::chrono::duration <double> duration =
    //                 std::chrono::system_clock::now() - start;

    //             mpz_class doing = (fu.value - fu.min);
    //             mpz_class remaining = (fu.max - fu.value);
    //             mpz_class pc = (doing * 100.0) / remaining;
    //             mpz_class estimated = (remaining * duration.count()) / doing;

    //             std::clog << "- " << pc.get_str() << "%. Time elapsed: "
    //                 << duration.count() << "s. Time remaining estimated: "
    //                 << estimated.get_str() << "s.\n";
    //             max = 0;
    //         }
    //     }

    //     return 0;
    // }

//     struct hierarchical_fu
//     {
//         hierarchical_fu() {}
//         virtual ~hierarchical_fu() {}

//         virtual void init(const mpz_class& min, const mpz_class& max) = 0;
//         virtual void init() = 0;
//         virtual bool next() = 0;
//         virtual std::string value() const = 0;
//         virtual mpz_class size() const = 0;
//     };

//     struct hierarchical_no_consistency_fu : hierarchical_fu
//     {
//         mpz_class current_value, min_value, max_value;
//         mpz_class scale_value_size;
//         mpz_class fu_size;

//         hierarchical_no_consistency_fu(const std::string& value,
//                                        unsigned int scale_value_size)
//             : hierarchical_fu()
//               , current_value(0)
//               , min_value(0)
//               , max_value(0)
//               , scale_value_size(scale_value_size)
//               , fu_size(value.size())
//         {}

//         virtual ~hierarchical_no_consistency_fu() {}

//         virtual void init(const mpz_class& min, const mpz_class& max)
//         {
//             min_value = min;
//             max_value = max;
//             current_value = min_value;
//         }

//         virtual void init()
//         {
//             current_value = min_value;
//         }

//         virtual bool next()
//         {
//             return ++current_value < max_value;
//         }

//         virtual std::string value() const
//         {
//             std::string ret(fu_size.get_ui(), '0');

//             mpz_class tmp = current_value;
//             mpz_class quo, rest;

//             int i = fu_size.get_ui() - 1;

//             while (tmp) {
//                 mpz_fdiv_qr(quo.get_mpz_t(), rest.get_mpz_t(), tmp.get_mpz_t(),
//                             scale_value_size.get_mpz_t());

//                 ret[i] += rest.get_ui();
//                 tmp = quo;
//                 --i;
//             }

//             return std::move(ret);
//         }

//         virtual mpz_class size() const
//         {
//             mpz_class ret;

//             mpz_ui_pow_ui(ret.get_mpz_t(), scale_value_size.get_ui(), fu_size.get_ui());

//             return std::move(ret);
//         }
//     };

//     struct hierarchical_consistency_fu : hierarchical_fu
//     {
//         Eigen::MatrixXi fu;
//         const int max_value;
//         int idx;
//         int idx_min, idx_max;

//         hierarchical_consistency_fu(const std::string& value,
//                                     unsigned int scale_value_size)
//             : hierarchical_fu()
//               , fu(Eigen::MatrixXi::Zero(1, value.size()))
//               , max_value(scale_value_size - 1)
//               , idx(0)
//               , idx_min(0)
//               , idx_max(0)
//         {
//             int col = 0;
//             int row = 0;

//             while (fu(row, 0) < max_value) {
//                 fu.conservativeResize(fu.rows() + 1, Eigen::NoChange_t());
//                 fu.row(fu.rows() - 1) = fu.row(fu.rows() - 2);
//                 row = fu.rows() - 1;
//                 col = fu.cols() - 1;

//                 fu(row, col)++;

//                 if (fu(row, col) > max_value) {
//                     do {
//                         col--;

//                         if (col < 0)
//                             return;

//                         fu(row, col)++;
//                     } while (fu(row, col) > max_value);

//                     for (int i = col + 1; i < fu.cols(); ++i)
//                         fu(row, i) = fu(row, col);
//                 }
//             }

// #ifndef NDEBUG
//             std::clog << "hierarchical_consistency_fu: "
//                 << fu.rows() << "x" << fu.cols()
//                 << "(" << scale_value_size << "**" << value.size() << ")"
//                 << "\n";
// #endif
//         }

//         virtual ~hierarchical_consistency_fu() {}

//         virtual void init(const mpz_class& min, const mpz_class& max)
//         {
//             idx_min = min.get_ui();
//             idx_max = max.get_ui();
//             idx = idx_min;
//         }

//         virtual void init()
//         {
//             idx = idx_min;
//         }

//         virtual bool next()
//         {
//             return ++idx < idx_max;
//         }

//         virtual std::string value() const
//         {
//             std::string ret(fu.cols(), '0');

//             for (int i = 0, e = fu.cols(); i != e; ++i)
//                 ret[i] += fu(idx, i);

//             return ret;
//         }

//         virtual mpz_class size() const
//         {
//             mpz_class ret;

//             ret = fu.rows();

//             return std::move(ret);
//         }
//     };

    // int compute_hierarchical_model(dexi& dex, int processor, int id)
    // {
    //     (void)processor;
    //     (void)id;

    //     std::vector <std::unique_ptr<hierarchical_fu>> solvers;

    //     for (auto& att : dex.attributes) {
    //         if (not att.is_basic()) {
    //             if (att.functions.consist == "False")
    //                 solvers.emplace_back(
    //                     new hierarchical_no_consistency_fu(
    //                         att.functions.low, att.scale_size()));
    //             else
    //                 solvers.emplace_back(
    //                     new hierarchical_consistency_fu(
    //                         att.functions.low, att.scale_size()));
    //         }
    //     }

    //     //
    //     // TODO bad implementation: needs MPI mode
    //     //

    //     // compute start et end for the first solver

    //     for (int i = 0, e = solvers.size(); i != e; ++i) {
    //         mpz_class max = solvers[i]->size();
    //         mpz_class min = 0;

    //         if (i == 0) {
    //             mpz_class proc = processor;
    //             if (proc == 1) {
    //                 min = 0;
    //             } else {
    //                 if (max < proc)
    //                     proc= max;

    //                 if (id >= proc)
    //                     return -2;

    //                 mpz_class lenght = max / proc;
    //                 min = lenght * id;
    //                 max = (lenght * (id + 1)) - 1;
    //             }

    //             std::clog << "\n"
    //                 << "- first solver id=" << id << " processor=" << processor << "\n"
    //                 << "  - start calculation at " << min.get_str() << "\n"
    //                 << "  - end calculation at " << max.get_str() << "\n";
    //         }

    //         solvers[i]->init(min, max);
    //     }

    //     std::size_t imax = 0;
    //     for (;;) {
    //         imax++;

    //         if ((imax % 123456789) == 0) {
    //             imax = 0;

    //             for (auto& solver : solvers)
    //                 std::clog << solver->value() << " ";
    //             std::clog << "\n";
    //         }

    //         std::size_t current = solvers.size() - 1;

    //         for (;;) {
    //             if (not solvers[current]->next()) {
    //                 solvers[current]->init();

    //                 if (current == 0) {
    //                     return -1;
    //                 } else {
    //                     current--;
    //                 }
    //             } else {
    //                 break;
    //             }
    //         }
    //     }

    //     return -1;
    // }

    // int compute_model(dexi& dex, int processor, int id)
    // {
    //     int consistency_fu_nb = 0;
    //     int fu_nb = 0;

    //     std::clog << "Compute all models\n";
    //     for (auto& att : dex.attributes) {
    //         if (not att.is_basic()) {
    //             bool consistency = att.functions.consist != "False";

    //             if (consistency)
    //                 consistency_fu_nb++;
    //             fu_nb++;

    //             std::clog << "- Attribute " << att.name << " scale size "
    //                 << att.scale_size() << " childrens : " << att.children.size()
    //                 << "\n";

    //             std::size_t m = 1;
    //             for (const auto& child : att.children) {
    //                 std::clog << "  - " << child->name << " scale size "
    //                     << child->scale_size() << "\n";
    //                 m *= child->scale_size();
    //             }
    //             std::clog << " Problem size: " << att.scale_size() << "**" << m <<
    //                 " (" << std::pow(att.scale_size(), m) << ")\n";

    //             std::clog << " Consistency " << std::boolalpha << consistency
    //                 << "\n";
    //         }
    //     }

    //     if (fu_nb > 0) {
    //         std::clog << "We can use the monotone kernel\n";
    //         return compute_hierarchical_model(dex, processor, id);
    //     } else {
    //         std::clog << "Inconsistency model: need too long time\n";
    //         return compute_flat_model(dex, processor, id);
    //     }
    // }
}
