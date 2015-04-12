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

#ifndef INRA_EFYj_SOLVER_STACK_HPP
#define INRA_EFYj_SOLVER_STACK_HPP

#include <efyj/model.hpp>
#include <efyj/exception.hpp>

#include <algorithm>
#include <numeric>

namespace efyj {

namespace solver_stack_details {

class aggregate_attribute
{
public:
    aggregate_attribute(const efyj::attribute &att)
        : stack_size(0)
    {
        std::transform(att.children.cbegin(),
                       att.children.cend(),
                       std::back_inserter(scale_size),
                       [](const efyj::attribute * child) {
                       return child->scale_size();
                       });

        coeffs = Vector::Zero(scale_size.size());
        coeffs(scale_size.size() - 1) = 1;

        for (long int i = (long int)scale_size.size() - 2; i >= 0; --i) // TODO FIXME
            coeffs(i) = scale_size[i + 1] * coeffs(i + 1);

        std::transform(att.functions.low.cbegin(),
                       att.functions.low.cend(),
                       std::back_inserter(functions),
                       [](const char id) {
                       return id - '0';
                       });

        stack = Vector::Zero(scale_size.size());
        stack_size = 0u;
    }

    inline std::size_t option_size() const
    {
        return coeffs.size();
    }

    inline void push(int i)
    {
        if (stack_size < 0)
            throw solver_error("too many attribute in function's stack");

        stack(stack_size--) = i;
    }

    inline void clear()
    {
        stack_size = coeffs.size() - 1;
    }

    inline int result() const
    {
        for (long i = 0; i < coeffs.size(); ++i)
            if (stack(i) >= (int)scale_size[i])
                throw solver_error("too big scale size");

        if (stack_size >= 0)
            throw solver_error("not enough attribute in function's stack to"
                               " get a result");

        return functions[coeffs.dot(stack)];
    }

private:
    Vector coeffs;
    std::vector <scale_id> functions;
    std::vector <std::size_t> scale_size;
    Vector stack;
    int stack_size;
};

struct Block {
    enum class BlockType {BLOCK_VALUE, BLOCK_ATTRIBUTE} type;

    Block(int value)
        : type(BlockType::BLOCK_VALUE), value(value)
    {}

    Block(solver_stack_details::aggregate_attribute *att)
        : type(BlockType::BLOCK_ATTRIBUTE), att(att)
    {}

    bool is_value() const { return type == BlockType::BLOCK_VALUE; }

    union {
        int value;
        solver_stack_details::aggregate_attribute *att;
    };
};

} // namespace solver_stack_details

class solver_stack
{
public:
    solver_stack(Model &model)
    {
        atts.reserve(model.attributes.size());

        int value_id = 0;
        recursive_fill(*model.child, value_id);
    }

    ~solver_stack()
    {}

    template <typename V>
        scale_id solve(const V &options)
        {
            result.clear();

            for (auto &block : function) {
                if (block.is_value()) {
                    result.emplace_back(options(block.value));
                } else {
                    block.att->clear();

                    for (std::size_t i = 0; i != block.att->option_size(); ++i) {
                        block.att->push(result.back());
                        result.pop_back();
                    }

                    result.emplace_back(block.att->result());
                }
            }

            if (result.size() != 1)
                throw solver_error("internal error in solver stack");

            return result[0];
        }

private:
    std::vector <solver_stack_details::aggregate_attribute> atts;

    // @e function is a Reverse Polish notation.
    std::vector <solver_stack_details::Block> function;

    // To avoid reallocation each solve(), we store the stack into the solver.
    std::vector <int> result;

    void recursive_fill(const efyj::attribute& att, int &value_id)
    {
        if (att.is_basic()) {
            function.emplace_back(value_id++);
        } else {
            for (auto& child : att.children)
                recursive_fill(*child, value_id);

            atts.emplace_back(att);
            function.emplace_back(&atts.back());
        }
    }
};

} // namespace efyj

#endif
