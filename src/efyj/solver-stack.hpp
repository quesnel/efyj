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

#ifndef INRA_EFYj_SOLVER_STACK_HPP
#define INRA_EFYj_SOLVER_STACK_HPP

#include <efyj/model.hpp>
#include <efyj/exception.hpp>
#include <efyj/types.hpp>

#include <algorithm>
#include <numeric>

namespace efyj {

namespace solver_stack_details {

struct aggregate_attribute
{
    aggregate_attribute(const Model& model, std::size_t att)
        : stack_size(0)
    {
        std::transform(model.attributes[att].children.cbegin(),
                       model.attributes[att].children.cend(),
                       std::back_inserter(m_scale_size),
                       [&model](std::size_t child)
                       {
                           return model.attributes[child].scale_size();
                       });

        std::transform(model.attributes[att].functions.low.cbegin(),
                       model.attributes[att].functions.low.cend(),
                       std::back_inserter(functions),
                       [](const char id) {
                           return id - '0';
                       });

        coeffs = Vector::Zero(m_scale_size.size());
        coeffs(m_scale_size.size() - 1) = 1;

        assert(m_scale_size.size() < std::numeric_limits<int>::max());
        assert(m_scale_size.size() >= 2);

        for (int i = (int)m_scale_size.size() - 2; i >= 0; --i)
            coeffs(i) = m_scale_size[i + 1] * coeffs(i + 1);

        stack = Vector::Zero(m_scale_size.size());
        stack_size = 0;

        scale = model.attributes[att].scale_size();
    }

    inline scale_id
    scale_size() const noexcept
    {
        return scale;
    }

    inline std::size_t
    option_size() const
    {
        return coeffs.size();
    }

    inline void
    push(int i)
    {
        assert(stack_size >= 0 && "too many attribute in function's stack");

        stack(stack_size--) = i;
    }

    inline void
    clear()
    {
        stack_size = coeffs.size() - 1;
    }

    inline int
    result() const
    {
#ifndef NDEBUG
        for (long i = 0; i < coeffs.size(); ++i) {
            assert(stack(i) < (int)m_scale_size[i] && "too big scale size");
        }
#endif

        assert(stack_size < 0 &&
               "not enough attribute in function's stack to get a result");

        return functions[coeffs.dot(stack)];
    }

    Vector coeffs;
    std::vector <scale_id> functions;
    std::vector <std::size_t> m_scale_size;
    Vector stack;
    scale_id scale;
    int stack_size;
};

struct Block
{
    inline constexpr
    Block(int value) noexcept
        : value(value)
        , type(BlockType::BLOCK_VALUE)
    {}

    inline constexpr
    Block(solver_stack_details::aggregate_attribute *att) noexcept
        : att(att)
        , type(BlockType::BLOCK_ATTRIBUTE)
    {}

    inline constexpr
    bool
    is_value() const noexcept
    {
        return type == BlockType::BLOCK_VALUE;
    }

    union {
        int value;
        solver_stack_details::aggregate_attribute *att;
    };
    enum class BlockType {BLOCK_VALUE, BLOCK_ATTRIBUTE} type;
};

} // namespace solver_stack_details

struct line_updater
{
    inline constexpr
    line_updater() noexcept
        : attribute(0)
        , line(0)
    {}

    int attribute;
    int line;
};

inline bool
operator==(const line_updater& lhs, const line_updater& rhs) noexcept
{
    return lhs.attribute == rhs.attribute and lhs.line == rhs.line;
}

class for_each_model_solver_stack;
std::ostream& operator<<(std::ostream& os, const line_updater& line);
std::ostream& operator<<(std::ostream& os,
                         const std::vector<line_updater>& updaters);
std::ostream& operator<<(std::ostream& os,
                         const for_each_model_solver_stack& solver);

struct solver_stack
{
    solver_stack(const Model &model)
    {
        atts.reserve(model.attributes.size());

        int value_id = 0;
        recursive_fill(model, 0, value_id);

        atts_copy = atts;
    }

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

        assert(result.size() == 1 && "internal error in solver stack");

        return result[0];
    }

    inline void
    functions_restore() noexcept
    {
        for (std::size_t i = 0, e = atts.size(); i != e; ++i)
            std::copy(atts_copy[i].functions.cbegin(),
                      atts_copy[i].functions.cend(),
                      atts[i].functions.begin());
    }

    inline int
    attribute_size() const noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);

        return static_cast <int>(atts.size());
    }

    inline int
    function_size(int attribute) const noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 and attribute < static_cast <int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);

        return static_cast <int>(atts[attribute].functions.size());
    }

    inline int
    scale_size(int attribute) const noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 and attribute < static_cast <int>(atts.size()));

        return static_cast <int>(atts[attribute].scale_size());
    }

    inline int
    value(int attribute, int line) const noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 and attribute < static_cast <int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);
        assert(line >= 0 and
               line < static_cast <int>(atts[attribute].functions.size()));

        return static_cast <int>(atts[attribute].functions[line]);
    }

    inline void
    value_restore(int attribute, int line) noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 and attribute < static_cast <int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);
        assert(line >= 0 and
               line < static_cast <int>(atts[attribute].functions.size()));

        atts[attribute].functions[line] = atts_copy[attribute].functions[line];
    }

    inline void
    value_increase(int attribute, int line) noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 and attribute < static_cast <int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);
        assert(line >= 0 and
               line < static_cast <int>(atts[attribute].functions.size()));

        ++atts[attribute].functions[line];

        assert(atts[attribute].functions[line] < atts[attribute].scale_size());
    }

    inline void
    value_clear(int attribute, int line) noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 and attribute < static_cast <int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);
        assert(line >= 0 and
               line < static_cast <int>(atts[attribute].functions.size()));

        atts[attribute].functions[line] = 0;
    }

    void
    recursive_fill(const efyj::Model& model, std::size_t att, int &value_id)
    {
        if (model.attributes[att].is_basic()) {
            function.emplace_back(value_id++);
        } else {
            for (auto& child : model.attributes[att].children)
                recursive_fill(model, child, value_id);

            atts.emplace_back(model, att);
            function.emplace_back(&atts.back());
        }
    }

    std::vector <solver_stack_details::aggregate_attribute> atts;
    std::vector <solver_stack_details::aggregate_attribute> atts_copy;

    // @e function is a Reverse Polish notation.
    std::vector <solver_stack_details::Block> function;

    // To avoid reallocation each solve(), we store the stack into the solver.
    std::vector <int> result;
};

class for_each_model_solver_stack
{
public:
    for_each_model_solver_stack(const Model& model)
        : m_solver(model)
    {
        init(1);
    }

    for_each_model_solver_stack(const Model& model, int walker_number)
        : m_solver(model)
    {
        init(walker_number);
    }

    void init(int walker_number)
    {
        m_updaters.resize(walker_number);
        m_walker_number = walker_number;

        {
            std::size_t sz = 1ul;

            for (int i = 0, e = m_solver.attribute_size(); i != e; ++i)
                sz *= m_solver.function_size(i);

            if (walker_number <= 0 or
                static_cast <std::size_t>(walker_number) >= sz)
                throw solver_error(
                    "solver: param greater than sum of lines in all functions");
        }

        m_updaters.back().attribute = 0;
        m_updaters.back().line = 0;

        init_walker_from(m_updaters.size() - 1);
    }

    /** Initialize walkers from walker @id - 1 to 0.
     *
     * For example, if i = 0 and attribute,line = 0, 0: move 0..n walkers
     * to the attribute 0, line 0...n.  move n+1..m walkers to the
     * attribute 1, line 0...m. If the attribute,line of walker is too
     * big, return false.
     */
    bool
    init_walker_from(int id)
    {
        assert(m_updaters.size() < INT_MAX
               and id < (int)m_updaters.size()
               and id >= 0);

        int attribute = m_updaters[id].attribute;
        int line = m_updaters[id].line;

        for (int i = id - 1; i >= 0; --i) {
            m_updaters[i].attribute = attribute;
            m_updaters[i].line = ++line;

            if (line >= m_solver.function_size(attribute)) {
                m_updaters[i].line = 0;
                m_updaters[i].attribute = ++attribute;
                line = -1;

                if (attribute >= m_solver.attribute_size())
                    return false;
            }
        }

        return true;
    }

    /** Advance walkers to the next value, line or attribute.
     *
     * If the first walker reaches the last attribute, then, second walker
     * walks to the next value, line or attribute. If the nth walker
     * reaches the last attribute the the function returns false.
     */
    bool next()
    {
        int i = 0;

        /* A strange behaviour because we need to advance walker and if it
         * reaches the last item, we advance second walker and
         * reinitialize previous first walker and ... and advance 3rd
         * walker and perhaps the advance of the first and second walker
         * may be impossible with second override first etc.
         */

        // TODO: Really need a best algorithm. Perhaps be rewriting the
        // init_walker_from function.

        while (next(i) == false) {
            ++i;
            if (i >= m_walker_number)
                return false;
        }

        if  (i > 0) {
            while (init_walker_from(i) == false) {
                ++i;

                if (i >= m_walker_number)
                    return false;

                while (next(i) == false) {
                    ++i;

                    if (i >= m_walker_number)
                        return false;
                }
            }
        }

        // static int max = 1;
        // static int current = 0;
        // current++;

        // if (current % max == 0) {
        //     current = 0;
        //     std::cout << "next(): ";
        //     for (int i = 0, e = (int)m_updaters.size(); i != e; ++i) {
        //         if (m_updaters[i].attribute == -1 or
        //             m_updaters[i].line == -1)
        //             std::cout << "[" << m_updaters[i].attribute
        //                       << "," << m_updaters[i].line
        //                       << "," << -1
        //                       << "] ";
        //         else
        //             std::cout << "[" << m_updaters[i].attribute
        //                       << "," << m_updaters[i].line
        //                       << "," << m_solver.value(m_updaters[i].attribute,
        //                                                m_updaters[i].line)
        //                       << "] ";
        //     }
        //     std::cout << "\n";
        // }

        return i >= 0;
    }

    template <typename V>
    scale_id solve(const V &options)
    {
        return m_solver.solve(options);
    }

    inline int
    walker_number() const noexcept
    {
        return m_walker_number;
    }

    inline
    std::vector <line_updater>
    updaters() const noexcept
    {
        return m_updaters;
    }

private:
    solver_stack m_solver;
    std::vector <line_updater> m_updaters;
    int m_walker_number;

    /** Advance the @e i walker.
     *
     * If we reach the max of the value for (attribute,line), we restore
     * the default value for (attribute, line) and try to jump to the next
     * (attribute,line++) or (attribute++,line).
     */
    bool next(int i)
    {
        int& attribute = m_updaters[i].attribute;
        int& line = m_updaters[i].line;

        if (attribute == -1 or line == -1)
            return false;

        if (m_solver.value(attribute, line) + 1 >=
            m_solver.scale_size(attribute)) {
            m_solver.value_restore(attribute, line);
            ++line;

            if (line >= m_solver.function_size(attribute)) {
                line = 0;
                ++attribute;

                if (attribute >= m_solver.attribute_size()) {
                    attribute = -1;
                    line = -1;
                    return false;
                }
            }

            m_solver.value_clear(attribute, line);
        } else {
            m_solver.value_increase(attribute, line);
        }

        return true;
    }
};

std::ostream& operator<<(std::ostream& os, const line_updater& updater)
{
    return os << "[" << updater.attribute << "," << updater.line << "]";
}

std::ostream& operator<<(std::ostream& os,
                         const std::vector<line_updater>& updaters)
{
    std::copy(updaters.cbegin(),
              updaters.cend(),
              std::ostream_iterator<line_updater>(os, " "));

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const for_each_model_solver_stack& solver)
{
    return os << "walker(s): " << solver.walker_number()
              << " states: " << solver.updaters()
              << '\n';
}

} // namespace efyj

#endif
