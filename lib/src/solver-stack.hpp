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

#include "context.hpp"
#include "model.hpp"
#include "exception.hpp"
#include "types.hpp"
#include <algorithm>
#include <numeric>

namespace efyj {

struct aggregate_attribute
{
    inline
    aggregate_attribute(const Model& model, std::size_t att_, int id_)
        : stack_size(0)
        , att(att_)
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

        struct walker
        {
            inline walker(int column_, int current_, int max_) noexcept
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
    int att;
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

    friend cstream&
    operator<<(cstream& os, const line_updater& updater) noexcept
    {
        return os << '[' << updater.attribute
                  << ',' << updater.line
                  << ']';
    }

    int attribute;
    int line;
};

struct solver_stack
{
    inline
    solver_stack(const Model &model)
    {
        atts.reserve(model.attributes.size());

        int value_id = 0;
        recursive_fill(model, 0, value_id);
    }

    /** Restores the default function (e.g. read from model file) for each
     * aggregate attributes.
     */
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

    inline int
    default_value(int attribute, int line) const noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 and attribute < static_cast <int>(atts.size()));
        assert(atts[attribute].saved_functions.size() < INT_MAX);
        assert(line >= 0 and line
               < static_cast<int>(atts[attribute].saved_functions.size()));

        return static_cast <int>(atts[attribute].saved_functions[line]);
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
    value_set(int attribute, int line, int scale_value) noexcept
    {
        assert(atts.size() > 0 && atts.size() < INT_MAX);
        assert(attribute >= 0 and attribute < static_cast <int>(atts.size()));
        assert(atts[attribute].functions.size() < INT_MAX);
        assert(line >= 0 and
               line < static_cast <int>(atts[attribute].functions.size()));

        atts[attribute].functions[line] = scale_value;
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

    void set_functions(const std::vector<std::vector<scale_id>>& functions)
    {
        assert(functions.size() == atts.size()
               && "incoherent: internal error");

        for (std::size_t i = 0, e = atts.size(); i != e; ++i) {
            assert(atts[i].functions.size() == functions[i].size()
                   && "incoherent: internal error");

            atts[i].functions = functions[i];
            atts[i].saved_functions = functions[i];
        }
    }

    void get_functions(std::vector<std::vector<scale_id>>& functions)
    {
        functions.resize(atts.size());

        std::transform(atts.cbegin(), atts.cend(), functions.begin(),
                       [](const aggregate_attribute& att)
                       {
                           return att.functions;
                       });
    }

    std::string string_functions() const
    {
        std::string ret;

        for (const auto& att : atts)
            for (auto id : att.functions)
                ret += id + '0';

        return ret;
    }

    friend std::ostream& operator<<(std::ostream& os, const solver_stack& s)
    {
        for (const auto& att : s.atts)
            for (auto id : att.functions)
                os << id;

        return os;
    }

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

class for_each_model_solver
{
public:
    std::shared_ptr<Context> m_context;
    solver_stack m_solver;
    std::vector <line_updater> m_updaters;
    std::vector<std::vector<int>> m_whitelist;
    int m_walker_number;

    /** @e full is used to enable all lines for all aggregate
     * attributes. It's the opposite of the @e reduce function.
     */
    void full()
    {
        m_context->info() << m_context->info().cyanb()
                          << "[Full problem size]"
                          << m_context->info().def()
                          << '\n';

        m_whitelist.clear();
        m_whitelist.resize(m_solver.attribute_size());

        for (int i = 0ul, e = m_whitelist.size(); i != e; ++i)
            for (int j = 0ul, endj = m_solver.function_size(i); j != endj; ++j)
                m_whitelist[i].emplace_back(j);
    }

    void detect_missing_scale_value()
    {
        m_context->info() << m_context->info().cyanb()
                          << "[Number of models available]"
                          << m_context->info().def()
                          << '\n';

        long double model_number {1};
        for (auto i = 0ul, e = m_whitelist.size(); i != e; ++i) {
            m_context->info() << m_solver.scale_size(i) << '^'
                              << m_whitelist[i].size();
            if (i + 1 != e)
                m_context->info() << " * ";

            model_number *= std::pow(m_solver.scale_size(i),
                                     m_whitelist[i].size());
        }

        m_context->info() << " = " << model_number << '\n';

        m_context->info() << m_context->info().cyanb()
                          << "[Detect unused scale value]"
                          << m_context->info().def()
                          << '\n';

        for (int i = 0ul, e = m_whitelist.size(); i != e; ++i) {
            int sv = m_solver.scale_size(i);

            m_context->info() << "Attribute " << i
                  << "\n- scale size........ : " << sv
                  << "\n- used rows......... : ";
            for (std::size_t x = 0, endx = m_whitelist[i].size(); x != endx; ++x)
                m_context->info() << m_whitelist[i][x] << ' ';

            m_context->info() << "\n- function.......... : ";
            for (std::size_t x = 0, endx = m_solver.function_size(i);
                 x != endx; ++x)
                m_context->info() << m_solver.value(i, x) << ' ';

            m_context->info() << "\n- unused scale value : ";
            for (int j = 0, endj = sv; j != endj; ++j) {
                std::size_t x, endx;

                for (x = 0, endx = m_whitelist[i].size(); x != endx; ++x) {
                    if (m_solver.value(i, m_whitelist[i][x]) == j)
                        break;
                }

                if (x == endx)
                    m_context->info() << j << ' ';
            }
            m_context->info() << "\n";
        }
    }

public:
    for_each_model_solver(std::shared_ptr<Context> context, const Model& model)
        : m_context(context)
        , m_solver(model)
    {
        full();
        init_walkers(1);

        detect_missing_scale_value();

        context->info() << context->info().cyanb()
                        << "[internal attribute id -> real attribute]"
                        << context->info().def()
                        << '\n';

        for (std::size_t i = 0, e = m_solver.atts.size(); i != e; ++i)
            context->info().indent(2)
                << i
                << ' '
                << model.attributes[m_solver.atts[i].att].name
                << '\n';
    }

    for_each_model_solver(std::shared_ptr<Context> context, const Model& model,
                          int walker_number)
        : m_context(context)
        , m_solver(model)
    {
        full();
        init_walkers(walker_number);

        detect_missing_scale_value();

        for (std::size_t i = 0, e = m_solver.atts.size(); i != e; ++i)
            context->info().indent(2)
                << i
                << ' '
                << model.attributes[m_solver.atts[i].att].name
                << '\n';
    }

    /** @e reduce is used to reduce the size of the problem. It removes
     * from the solver, all lines from the solver based on options.
     */
    void reduce(const Options& options)
    {
        m_context->info() << m_context->info().cyanb()
                          << "[Reducing problem size]"
                          << m_context->info().def()
                          << '\n';

        m_whitelist.clear();
        m_whitelist.resize(m_solver.attribute_size());

        std::vector<std::set<int>> whitelist;
        whitelist.resize(m_solver.attribute_size());

        for (std::size_t i = 0, e = options.options.rows(); i != e; ++i)
            m_solver.reduce(options.options.row(i), whitelist);

        for (auto i = 0ul, e = whitelist.size(); i != e; ++i) {
            m_context->info() << "  Whitelist ";
            for (const auto v : whitelist[i])
                m_context->info() << v << ' ';

            m_context->info() << '(' << m_solver.function_size(i) << ")\n";
        }

        /* convert the set into vector of vector. */
        for (int i = 0ul, e = whitelist.size(); i != e; ++i) {
            m_whitelist[i].resize(whitelist[i].size());

            std::copy(whitelist[i].begin(), whitelist[i].end(),
                      m_whitelist[i].begin());
        }
    }

    /** @e reduce is used to reduce the size of the problem. It removes
     * from the solver, all lines from the solver based on part (@e ids)
     * of the options.
     */
    void reduce(const Options& options, const std::vector<int>& ids)
    {
        m_context->info() << "Reducing problem size\n";

        m_whitelist.clear();
        m_whitelist.resize(m_solver.attribute_size());

        std::vector<std::set<int>> whitelist;
        whitelist.resize(m_solver.attribute_size());

        for (std::size_t i = 0, e = ids.size(); i != e; ++i)
            for (auto x : options.ordered[ids[i]])
                m_solver.reduce(options.options.row(x), whitelist);

        for (auto i = 0ul, e = whitelist.size(); i != e; ++i) {
            m_context->info() << " whitelist ";
            for (const auto v : whitelist[i])
                m_context->info() << v << ' ';

            m_context->info() << '(' << m_solver.function_size(i) << ")\n";
        }

        /* convert the set into vector of vector. */
        for (int i = 0ul, e = whitelist.size(); i != e; ++i) {
            m_whitelist[i].resize(whitelist[i].size());

            std::copy(whitelist[i].begin(), whitelist[i].end(),
                      m_whitelist[i].begin());
        }
    }

    void init_next_value()
    {
        m_solver.reinit();

        for (std::size_t i = 0, e = m_updaters.size(); i != e; ++i) {
            const int attribute = m_updaters[i].attribute;
            const int line = m_whitelist[attribute][m_updaters[i].line];

            m_solver.value_clear(attribute, line);
        }
    }

    bool next_value()
    {
        assert(not m_updaters.empty() and m_updaters.size() < INT_MAX);

        std::size_t i = m_updaters.size() - 1;

        for (;;) {
            int attribute = m_updaters[i].attribute;
            int line = m_whitelist[attribute][m_updaters[i].line];

            if (m_solver.value(attribute, line) + 1 <
                m_solver.scale_size(attribute)) {

                m_solver.value_increase(attribute, line);
                return true;
            } else {
                if (i == 0)
                    return false;

                m_solver.value_clear(attribute, line);
                --i;
            }
        }
    }

    bool init_walkers(std::size_t walker_numbers)
    {
        assert(walker_numbers > 0);

        m_updaters.resize(walker_numbers);

        std::size_t i = 0;
        int attribute = m_updaters[i].attribute = 0;
        int line = m_updaters[i].line = 0;

        for (auto j = i + 1; j < m_updaters.size(); ++j) {
            m_updaters[j].attribute = attribute;
            m_updaters[j].line = ++line;

            if (static_cast<std::size_t>(m_updaters[j].line) >=
                m_whitelist[m_updaters[j].attribute].size()) {
                line = 0;
                ++attribute;

                m_updaters[j].attribute = attribute;
                m_updaters[j].line = line;

                if (attribute >= m_solver.attribute_size())
                    return false;
            }
        }

        return true;
    }

    bool next_line()
    {
        assert(not m_updaters.empty() and m_updaters.size() < INT_MAX);

        for (;;) {
            std::size_t i = m_updaters.size() - 1;

            for (;;) {
                if (static_cast<std::size_t>(m_updaters[i].line + 1) <
                    m_whitelist[m_updaters[i].attribute].size()) {
                    ++m_updaters[i].line;

                    int attribute = m_updaters[i].attribute;
                    int line = m_updaters[i].line;
                    for (auto j = i + 1; j < m_updaters.size(); ++j) {
                        m_updaters[j].attribute = attribute;
                        m_updaters[j].line = ++line;

                        if (static_cast<std::size_t>(m_updaters[j].line) >=
                            m_whitelist[m_updaters[j].attribute].size()) {
                            line = 0;
                            ++attribute;

                            m_updaters[j].attribute = attribute;
                            m_updaters[j].line = line;

                            if (attribute >= m_solver.attribute_size())
                                return false;
                        }
                    }

                    return true;
                } else {
                    if (m_updaters[i].attribute + 1 < m_solver.attribute_size()) {
                        ++m_updaters[i].attribute;
                        m_updaters[i].line = 0;

                        int attribute = m_updaters[i].attribute;
                        int line = m_updaters[i].line;
                        for (auto j = i + 1; j < m_updaters.size(); ++j) {
                            m_updaters[j].attribute = attribute;
                            m_updaters[j].line = ++line;

                            if (static_cast<std::size_t>(m_updaters[j].line) >=
                                m_whitelist[m_updaters[j].attribute].size()) {
                                line = 0;
                                ++attribute;

                                m_updaters[j].attribute = attribute;
                                m_updaters[j].line = line;

                                if (attribute >= m_solver.attribute_size())
                                    return false;
                            }
                        }

                        return true;
                    } else {
                        if (i == 0)
                            return false;

                        --i;
                    }
                }
            }
        }
    }

    template <typename V>
    scale_id solve(const V &options)
    {
        return m_solver.solve(options);
    }

    inline
    void
    set_functions(const std::vector<std::vector<scale_id>>& functions)
    {
        return m_solver.set_functions(functions);
    }

    inline
    void
    get_functions(std::vector<std::vector<scale_id>>& functions)
    {
        return m_solver.get_functions(functions);
    }

    inline
    std::vector <std::tuple<int, int, int>>
        updaters() const
    {
        /* if mode with reduce, we recompute attributes/lines otherwise,
         * we can return m_updaters directly.
         */

        std::vector<std::tuple<int, int, int>> ret;
        ret.reserve(m_updaters.size());

        for (std::size_t i = 0, e = m_updaters.size(); i != e; ++i) {
            const int attribute = m_updaters[i].attribute;
            const int line = m_whitelist[attribute][m_updaters[i].line];

            ret.emplace_back(attribute,
                             line,
                             m_solver.value(attribute, line));
        }

        return ret;
    }

    inline
    std::size_t get_attribute_line_tuple_limit() const
    {
        std::size_t ret = 0;

        for (const auto& att : m_whitelist)
            ret += att.size();

        return ret;
    }

    std::string string_functions() const
    {
        return m_solver.string_functions();
    }
};

inline cstream&
operator<<(cstream& os, const std::vector<line_updater>& updaters)
{
    for (const auto& x : updaters)
        os << x << ' ';

    return os;
}

inline
cstream& operator<<(cstream& cs, const std::tuple<int, int, int>& att)
{
    return cs << '[' << std::get<0>(att)
              << ',' << std::get<1>(att)
              << ',' << std::get<2>(att)
              << ']';
}

inline
cstream& operator<<(cstream& cs, const std::vector<
                    std::tuple<int, int, int>>& atts)
{
    for (std::size_t i = 0, e = atts.size(); i != e; ++i) {
        cs << atts[i];
        if (i + 1 != e)
            cs << ' ';
    }

    return cs;
}

} // namespace efyj

#endif
