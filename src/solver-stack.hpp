/* Copyright (C) 2015-2016 INRA
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

#include "model.hpp"
#include "exception.hpp"
#include "types.hpp"
#include <algorithm>
#include <numeric>

namespace efyj {
namespace solver_details {

struct aggregate_attribute
{
    inline
    aggregate_attribute(const Model& model, std::size_t att, int id_)
        : stack_size(0)
        , id(id_)
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
                       [](const char id)
                       {
                           return id - '0';
                       });

        saved_functions = functions;

        coeffs = Vector::Zero(m_scale_size.size());
        coeffs(m_scale_size.size() - 1) = 1;

        assert(m_scale_size.size() < std::numeric_limits<int>::max());
	assert(m_scale_size.size() >= 1);

	if (m_scale_size.size() >= 2) {
            for (int i = (int)m_scale_size.size() - 2; i >= 0; --i)
	        coeffs(i) = m_scale_size[i + 1] * coeffs(i + 1);
	}

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
    inline void
    reduce(std::set<int>& whitelist)
    {
#ifndef NDEBUG
        for (long i = 0; i < coeffs.size(); ++i) {
            assert(stack(i) < (int)m_scale_size[i] && "too big scale size");
        }
#endif

        struct walker {
            walker(int column_, int current_, int max_)
                : column(column_), current(current_), max(max_)
            {}

            int column, current, max;
        };

        std::vector<walker> walker;
        for (std::size_t i = 0, e = stack.size(); i != e; ++i) {
            if (stack[i] == -1) {
                walker.emplace_back(static_cast<int>(i), 0, m_scale_size[i]);
                stack[i] = 0;
            }
        }

        if (walker.empty()) {
            whitelist.emplace(coeffs.dot(stack));
            return;
        }

        bool end = false;
        do {
            std::size_t i = walker.size() - 1;
            do {
                whitelist.emplace(coeffs.dot(stack));

                walker[i].current++;
                stack[walker[i].column] = walker[i].current;

                if (walker[i].current >= walker[i].max) {
                    walker[i].current = 0;
                    stack[walker[i].column] = 0;

                    if (i == 0) {
                        end = true;
                        break;
                    } else {
                        i--;
                    }
                } else {
                    break;
                }
            } while (not end);
        } while (not end);
    }

    inline void
    function_restore() noexcept
    {
        functions = saved_functions;
    }

    Vector coeffs;
    std::vector <scale_id> functions;
    std::vector <scale_id> saved_functions;
    std::vector <std::size_t> m_scale_size;
    Vector stack;
    scale_id scale;
    int stack_size;
    int id; /* Reference in the solver_stack atts attribute. */
};

inline cstream& operator<<(cstream& os, const aggregate_attribute& att)
{
    os << "f:";
    for (const auto& x : att.functions)
        os << x;

    return os << " sz:" << att.scale;
}

inline cstream& operator<<(cstream& os,
                    const std::vector <aggregate_attribute>& atts)
{
    for (const auto& x : atts)
        os << x << "\n";

    return os;
}

// TODO perhaps replace Block with a boost::variant<int,
// solver_stack_details::aggregate_attribute*>.
struct Block
{
    inline constexpr
    Block(int value) noexcept
        : value(value)
        , type(BlockType::BLOCK_VALUE)
    {}

    inline constexpr
    Block(aggregate_attribute *att) noexcept
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
        aggregate_attribute *att;
    };
    enum class BlockType {BLOCK_VALUE, BLOCK_ATTRIBUTE} type;
};

struct line_updater {
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

inline bool
operator==(const line_updater& lhs, const line_updater& rhs) noexcept
{
    return lhs.attribute == rhs.attribute and lhs.line == rhs.line;
}

template<typename Solver>
class for_each_model_solver;
cstream& operator<<(cstream& os, const line_updater& line);
cstream& operator<<(cstream& os,
                         const std::vector<line_updater>& updaters);
template<typename Solver>
cstream&
operator<<(cstream& os,
           const for_each_model_solver<Solver>& solver);

struct solver_stack
{
    inline
    solver_stack(const Model &model)
    {
        atts.reserve(model.attributes.size());

        int value_id = 0;
        recursive_fill(model, 0, value_id);
    }

    inline void
    reinit()
    {
        for (auto& att : atts)
            att.function_restore();
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

    template <typename V>
    void reduce(const V& options,
                std::vector<std::set<int>>& whitelist)
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

                block.att->reduce(whitelist[block.att->id]);
                result.emplace_back(-1); // -1 means computed value.
            }
        }

        assert(result.size() == 1 && "internal error in solver stack");
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

        atts[attribute].functions[line] = atts[attribute].saved_functions[line];
    }

    inline void
    value_increase(int attribute, int line) noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 and attribute < static_cast <int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);
        assert(line >= 0 and
               line < static_cast <int>(atts[attribute].functions.size()));

        atts[attribute].functions[line] += 1;

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

            atts.emplace_back(model, att, static_cast<int>(atts.size()));
            function.emplace_back(&atts.back());
        }
    }

private:
    std::vector <aggregate_attribute> atts;

    // @e function is a Reverse Polish notation.
    std::vector <Block> function;

    // To avoid reallocation each solve(), we store the stack into the solver.
    std::vector <int> result;
};

inline cstream&
operator<<(cstream& os, const std::vector<scale_id>& v)
{
    for (auto x : v)
        os << x;

    return os;
}

template <typename Solver>
class for_each_model_solver
{
    Solver m_solver;
    std::vector <line_updater> m_updaters;
    std::vector<std::vector<int>> m_whitelist;
    int m_walker_number;
    bool m_with_reduce;

    /** @e reduce is used to reduce the size of the problem. It removes
     * from the solver, all lines from the solver based on options.
     */
    void reduce(const Options& options)
    {
        out() << out().red() << "Reducing problem size\n" << out().def();

        m_whitelist.clear();
        m_whitelist.resize(m_solver.attribute_size());

        std::vector<std::set<int>> whitelist;
        whitelist.resize(m_solver.attribute_size());

        for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
            m_solver.reduce(options.options.row(i), whitelist);

        for (auto i = 0ul, e = whitelist.size(); i != e; ++i) {
            out() << " whitelist ";
            for (const auto v : whitelist[i])
                out() << v << ' ';

            out() << '(' << m_solver.function_size(i) << ")\n";
        }

        /* convert the set into vector of vector. */
        for (int i = 0ul, e = whitelist.size(); i != e; ++i) {
            m_whitelist[i].resize(whitelist[i].size());

            std::copy(whitelist[i].begin(), whitelist[i].end(),
                      m_whitelist[i].begin());
        }
    }

    void full()
    {
        out() << out().red() << "Full problem size\n" << out().def();

        m_whitelist.clear();
        m_whitelist.resize(m_solver.attribute_size());

        for (int i = 0ul, e = m_whitelist.size(); i != e; ++i)
            for (int j = 0ul, endj = m_solver.function_size(i); j != endj; ++j)
                m_whitelist[i].emplace_back(j);
    }

public:
    for_each_model_solver(const Model& model, const Options& options,
                          bool with_reduce = true)
        : m_solver(model)
        , m_with_reduce(with_reduce)
    {
        if (m_with_reduce)
            reduce(options);
        else
            full();

        init(1);
    }

    for_each_model_solver(const Model& model, const Options& options,
                          int walker_number, bool with_reduce = true)

        : m_solver(model)
        , m_with_reduce(with_reduce)
    {
        if (m_with_reduce)
            reduce(options);
        else
            full();

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

        assert(not m_whitelist[0].empty());

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

            // if (line >= m_solver.function_size(attribute)) {
            if (line >= m_whitelist[attribute].size()) {
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
        /* if mode with reduce, we recompute attributes/lines otherwise,
         * we can return m_updaters directly.
         */

        std::vector<line_updater> ret;

        for (const auto& lu : m_updaters)
            ret.emplace_back(lu.attribute, m_whitelist[lu.attribute][lu.line]);

        return ret;
    }

    inline
    int get_max_updaters() const noexcept
    {
        std::size_t sz = 1ul;

        for (int i = 0, e = m_solver.attribute_size(); i != e; ++i)
            sz *= m_solver.function_size(i);

        assert(sz < INT_MAX);

        return static_cast<int>(sz);
    }

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

        if (m_solver.value(attribute, m_whitelist[attribute][line]) + 1 >=
            m_solver.scale_size(attribute)) {
            m_solver.value_restore(attribute, m_whitelist[attribute][line]);

            ++line;

            if (line >= m_whitelist[attribute].size()) {
                line = 0;
                ++attribute;

                if (attribute >= m_solver.attribute_size()) {
                    attribute = -1;
                    line = -1;
                    return false;
                }
            }

            m_solver.value_clear(attribute, m_whitelist[attribute][line]);
        } else {
            m_solver.value_increase(attribute, m_whitelist[attribute][line]);
        }

        return true;
    }
};

inline cstream&
operator<<(cstream& os, const line_updater& updater)
{
    return os << "[" << updater.attribute << "," << updater.line << "]";
}

inline cstream&
operator<<(cstream& os,
           const std::vector<line_updater>& updaters)
{
    for (const auto& x : updaters)
        os << x << " ";

    return os;
}

template <typename Solver>
cstream&
operator<<(cstream& os,
           const for_each_model_solver<Solver>& solver)
{
    return os << "walker(s): " << solver.walker_number()
              << " states: " << solver.updaters()
              << '\n';
}

}} // namespace efyj

#endif