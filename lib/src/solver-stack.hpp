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

#ifndef INRA_EFYj_SOLVER_STACK_HPP
#define INRA_EFYj_SOLVER_STACK_HPP

#include <EASTL/set.h>

#include "model.hpp"
#include "options.hpp"
#include "private.hpp"

#include <cassert>
#include <cmath>

namespace efyj {

using Vector = eastl::vector<int>;

struct aggregate_attribute
{
    aggregate_attribute(const Model& model, size_t att_, int id_);
    inline scale_id scale_size() const noexcept
    {
        return scale;
    }

    inline size_t option_size() const
    {
        return coeffs.size();
    }

    inline void push(int i)
    {
        assert(stack_size >= 0 && "too many attribute in function's stack");

        stack[stack_size--] = i;
    }

    inline void clear()
    {
        stack_size = static_cast<int>(coeffs.size()) - 1;
    }

    int result() const;

    /** The @e reduce function returns the list of authorized lines in the
     * utility function of the aggregate attribute.
     *
     * We replace the -1 in the stack(i) (scale value from another
     * aggregate attribute and not option) with the scale values for each
     * columns. It produces a list of all line authorized for the model
     * generator. For example:
     *
     * opt1 opt2 -1 opt3 -1
     * --------------------
     * opt1 opt2  0 opt3  0
     * opt1 opt2  0 opt3  1
     * opt1 opt2  0 opt3  2
     * opt1 opt2  1 opt3  0
     * opt1 opt2  1 opt3  1
     * opt1 opt2  1 opt3  2
     */
    void reduce(eastl::set<int>& whitelist);

    inline void function_restore() noexcept
    {
        functions = saved_functions;
    }

    Vector coeffs;
    eastl::vector<scale_id> functions;
    eastl::vector<scale_id> saved_functions;
    eastl::vector<size_t> m_scale_size;
    Vector stack;
    scale_id scale;
    int stack_size;
    int att;
    int id; /* Reference in the solver_stack atts attribute. */
};

struct Block
{
    inline constexpr Block(int value) noexcept
      : value(value)
      , type(BlockType::BLOCK_VALUE)
    {}

    inline constexpr Block(aggregate_attribute* att) noexcept
      : att(att)
      , type(BlockType::BLOCK_ATTRIBUTE)
    {}

    inline constexpr bool is_value() const noexcept
    {
        return type == BlockType::BLOCK_VALUE;
    }

    union
    {
        int value;
        aggregate_attribute* att;
    };

    enum class BlockType
    {
        BLOCK_VALUE,
        BLOCK_ATTRIBUTE
    } type;
};

struct line_updater
{
    constexpr line_updater() noexcept
      : attribute(0)
      , line(0)
    {}

    constexpr line_updater(int attribute_, int line_) noexcept
      : attribute(attribute_)
      , line(line_)
    {}

    int attribute;
    int line;
};

struct solver_stack
{
    solver_stack(const Model& model);

    /** Restores the default function (e.g. read from model file) for each
     * aggregate attributes.
     */
    void reinit();

    template<typename T>
    scale_id solve(const T& options)
    {
        result.clear();

        for (auto& block : function) {
            if (block.is_value()) {
                result.emplace_back(options[block.value]);
            } else {
                block.att->clear();

                for (size_t i = 0; i != block.att->option_size(); ++i) {
                    block.att->push(result.back());
                    result.pop_back();
                }

                result.emplace_back(block.att->result());
            }
        }

        assert(result.size() == 1 && "internal error in solver stack");

        return result[0];
    }

    template<typename V>
    void reduce(const V& options, eastl::vector<eastl::set<int>>& whitelist)
    {
        result.clear();

        for (auto& block : function) {
            if (block.is_value()) {
                result.emplace_back(options[block.value]);
            } else {
                block.att->clear();

                for (size_t i = 0; i != block.att->option_size(); ++i) {
                    block.att->push(result.back());
                    result.pop_back();
                }

                block.att->reduce(whitelist[block.att->id]);
                result.emplace_back(-1); // -1 means computed value.
            }
        }

        assert(result.size() == 1 && "internal error in solver stack");
    }

    inline int attribute_size() const noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);

        return static_cast<int>(atts.size());
    }

    inline int function_size(int attribute) const noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 && attribute < static_cast<int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);

        return static_cast<int>(atts[attribute].functions.size());
    }

    inline int scale_size(int attribute) const noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 && attribute < static_cast<int>(atts.size()));

        return static_cast<int>(atts[attribute].scale_size());
    }

    inline int value(int attribute, int line) const noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 && attribute < static_cast<int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);
        assert(line >= 0 &&
               line < static_cast<int>(atts[attribute].functions.size()));

        return static_cast<int>(atts[attribute].functions[line]);
    }

    inline int default_value(int attribute, int line) const noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 && attribute < static_cast<int>(atts.size()));
        assert(atts[attribute].saved_functions.size() < INT_MAX);
        assert(line >= 0 && line < static_cast<int>(
                                     atts[attribute].saved_functions.size()));

        return static_cast<int>(atts[attribute].saved_functions[line]);
    }

    inline void value_restore(int attribute, int line) noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 && attribute < static_cast<int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);
        assert(line >= 0 &&
               line < static_cast<int>(atts[attribute].functions.size()));

        atts[attribute].functions[line] =
          atts[attribute].saved_functions[line];
    }

    inline void value_set(int attribute, int line, int scale_value) noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 && attribute < static_cast<int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);
        assert(line >= 0 &&
               line < static_cast<int>(atts[attribute].functions.size()));

        atts[attribute].functions[line] = scale_value;
    }

    inline void value_increase(int attribute, int line) noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 && attribute < static_cast<int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);
        assert(line >= 0 &&
               line < static_cast<int>(atts[attribute].functions.size()));

        atts[attribute].functions[line] += 1;

        assert(atts[attribute].functions[line] < atts[attribute].scale_size());
    }

    inline void value_clear(int attribute, int line) noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 && attribute < static_cast<int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);
        assert(line >= 0 &&
               line < static_cast<int>(atts[attribute].functions.size()));

        atts[attribute].functions[line] = 0;
    }

    void recursive_fill(const Model& model, size_t att, int& value_id);

    void set_functions(
      const eastl::vector<eastl::vector<scale_id>>& functions);

    void get_functions(eastl::vector<eastl::vector<scale_id>>& functions);

    eastl::string string_functions() const;

    eastl::vector<aggregate_attribute> atts;

    // @e function is a Reverse Polish notation.
    eastl::vector<Block> function;

    // To avoid reallocation each solve(), we store the stack into the solver.
    eastl::vector<int> result;
};

class for_each_model_solver
{
public:
    eastl::shared_ptr<context> m_context;
    solver_stack m_solver;
    eastl::vector<line_updater> m_updaters;
    eastl::vector<eastl::vector<int>> m_whitelist;
    int m_walker_number;

    /** @e full is used to enable all lines for all aggregate
     * attributes. It's the opposite of the @e reduce function.
     */
    void full();

    void detect_missing_scale_value();

public:
    for_each_model_solver(eastl::shared_ptr<context> context,
                          const Model& model);

    for_each_model_solver(eastl::shared_ptr<context> context,
                          const Model& model,
                          int walker_number);

    /** @e reduce is used to reduce the size of the problem. It removes
     * from the solver, all lines from the solver based on options.
     */
    void reduce(const Options& options);

    void init_next_value();

    bool next_value();

    bool init_walkers(size_t walker_numbers);

    bool next_line();

    template<typename V>
    scale_id solve(const V& options)
    {
        return m_solver.solve(options);
    }

    void set_functions(const eastl::vector<eastl::vector<scale_id>>& functions)
    {
        return m_solver.set_functions(functions);
    }

    void get_functions(eastl::vector<eastl::vector<scale_id>>& functions)
    {
        return m_solver.get_functions(functions);
    }

    eastl::vector<eastl::tuple<int, int, int>> updaters() const;

    size_t get_attribute_line_tuple_limit() const;

    eastl::string string_functions() const
    {
        return m_solver.string_functions();
    }
};

void
print(eastl::shared_ptr<context> ctx,
      const eastl::vector<eastl::tuple<int, int, int>>& updaters) noexcept;

} // namespace efyj

#endif
