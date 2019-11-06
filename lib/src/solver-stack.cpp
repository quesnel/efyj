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

#include <efyj/efyj.hpp>

#include "solver-stack.hpp"

#include <cassert>
#include <cmath>

namespace efyj {

aggregate_attribute::aggregate_attribute(const Model& model,
                                         size_t att_,
                                         int id_)
  : stack_size(0)
  , att(static_cast<int>(att_))
  , id(id_)
{
    std::transform(
      model.attributes[att].children.cbegin(),
      model.attributes[att].children.cend(),
      std::back_inserter(m_scale_size),
      [&model](size_t child) { return model.attributes[child].scale_size(); });

    std::transform(model.attributes[att].functions.low.cbegin(),
                   model.attributes[att].functions.low.cend(),
                   std::back_inserter(functions),
                   [](const char id) { return id - '0'; });

    saved_functions = functions;

    coeffs.resize(m_scale_size.size(), 0);
    coeffs[m_scale_size.size() - 1] = 1;

    assert(m_scale_size.size() <
           std::numeric_limits<decltype(m_scale_size)::value_type>::max());
    assert(m_scale_size.size() >= 1);

    if (m_scale_size.size() >= 2) {
        for (int i = (int)m_scale_size.size() - 2; i >= 0; --i)
            coeffs[i] = static_cast<int>(m_scale_size[i + 1]) * coeffs[i + 1];
    }

    stack.resize(m_scale_size.size(), 0);
    stack_size = 0;

    scale = model.attributes[att].scale_size();
}

int
aggregate_attribute::result() const
{
#ifndef NDEBUG
    for (size_t i = 0; i < coeffs.size(); ++i) {
        assert(stack[i] < (int)m_scale_size[i] && "too big scale size");
    }
#endif

    assert(stack_size < 0 &&
           "not enough attribute in function's stack to get a result");

    auto id = 0;
    for (size_t i = 0, e = coeffs.size(); i != e; ++i)
        id += coeffs[i] * stack[i];

    return functions[id];

    // return functions[coeffs.dot(stack)];
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
void
aggregate_attribute::reduce(std::set<int>& whitelist)
{
#ifndef NDEBUG
    for (size_t i = 0; i < coeffs.size(); ++i) {
        assert(stack[i] < (int)m_scale_size[i] && "too big scale size");
    }
#endif

    struct walker
    {
        inline walker(int column_, int current_, int max_) noexcept
          : column(column_)
          , current(current_)
          , max(max_)
        {}

        int column, current, max;
    };

    std::vector<walker> walker;
    for (size_t i = 0, e = stack.size(); i != e; ++i) {
        if (stack[i] == -1) {
            walker.emplace_back(static_cast<int>(i), 0, m_scale_size[i]);
            stack[i] = 0;
        }
    }

    if (walker.empty()) {
        auto id = 0;
        for (size_t i{ 0 }, e{ coeffs.size() }; i != e; ++i)
            id += coeffs[i] * stack[i];
        whitelist.emplace(id);

        // whitelist.emplace(coeffs.dot(stack));
        return;
    }

    bool end = false;
    do {
        size_t i = walker.size() - 1;
        do {
            auto id = 0;
            for (size_t i{ 0 }, e{ coeffs.size() }; i != e; ++i)
                id += coeffs[i] * stack[i];
            whitelist.emplace(id);

            // whitelist.emplace(coeffs.dot(stack));

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
        } while (!end);
    } while (!end);
}

solver_stack::solver_stack(const Model& model)
{
    atts.reserve(model.attributes.size());

    int value_id = 0;
    recursive_fill(model, 0, value_id);
}

void
solver_stack::reinit()
{
    for (auto& att : atts)
        att.function_restore();
}

void
solver_stack::recursive_fill(const Model& model, size_t att, int& value_id)
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

void
solver_stack::set_functions(
  const std::vector<std::vector<scale_id>>& functions)
{
    assert(functions.size() == atts.size() && "incoherent: internal error");

    for (size_t i = 0, e = atts.size(); i != e; ++i) {
        assert(atts[i].functions.size() == functions[i].size() &&
               "incoherent: internal error");

        atts[i].functions = functions[i];
        atts[i].saved_functions = functions[i];
    }
}

void
solver_stack::get_functions(std::vector<std::vector<scale_id>>& functions)
{
    functions.resize(atts.size());

    std::transform(
      atts.cbegin(),
      atts.cend(),
      functions.begin(),
      [](const aggregate_attribute& att) { return att.functions; });
}

std::string
solver_stack::string_functions() const
{
    std::string ret;

    for (const auto& att : atts)
        for (auto id : att.functions)
            ret += static_cast<char>(id + '0');

    return ret;
}

void
for_each_model_solver::full()
{
    info(m_context, "[Full problem size]\n");

    m_whitelist.clear();
    m_whitelist.resize(m_solver.attribute_size());

    for (std::size_t i = { 0 }, e = m_whitelist.size(); i != e; ++i)
        for (int j = { 0 }, endj = m_solver.function_size(static_cast<int>(i));
             j != endj;
             ++j)
            m_whitelist[i].emplace_back(j);
}

void
for_each_model_solver::detect_missing_scale_value()
{
    info(m_context, "[Number of models available]\n");

    long double model_number{ 1 };
    for (size_t i = 0, e = m_whitelist.size(); i != e; ++i) {
        info(m_context,
             "{} ^ {}\n",
             m_solver.scale_size(static_cast<int>(i)),
             m_whitelist[i].size());
        if (i + 1 != e)
            info(m_context, " * ");

        model_number *= std::pow(m_solver.scale_size(static_cast<int>(i)),
                                 m_whitelist[i].size());
    }

    info(m_context, " = {}\n", model_number);

    info(m_context, "[Detect unused scale value]\n");

    for (std::size_t i{ 0 }, e = m_whitelist.size(); i != e; ++i) {
        int sv = m_solver.scale_size(static_cast<int>(i));

        info(m_context,
             "Attribute {}\n"
             "\n- scale size........ : {}"
             "\n- used rows......... : ",
             i,
             sv);

        for (size_t x = 0, endx = m_whitelist[i].size(); x != endx; ++x)
            info(m_context, "{} ", m_whitelist[i][x]);

        info(m_context, "\n- function.......... : ");
        for (size_t x = 0, endx = m_solver.function_size(static_cast<int>(i));
             x != endx;
             ++x)
            info(m_context,
                 "{} ",
                 m_solver.value(static_cast<int>(i), static_cast<int>(x)));

        info(m_context, "\n- unused scale value : ");
        for (int j = 0, endj = sv; j != endj; ++j) {
            size_t x, endx;

            for (x = 0, endx = m_whitelist[i].size(); x != endx; ++x) {
                if (m_solver.value(static_cast<int>(i), m_whitelist[i][x]) ==
                    j)
                    break;
            }

            if (x == endx)
                info(m_context, "{} ", j);
        }
        info(m_context, "\n");
    }
}

for_each_model_solver::for_each_model_solver(std::shared_ptr<context> context,
                                             const Model& model)
  : m_context(context)
  , m_solver(model)
{
    full();
    init_walkers(1);

    detect_missing_scale_value();

    info(context, "[internal attribute id -> real attribute]\n");

    for (size_t i = 0, e = m_solver.atts.size(); i != e; ++i)
        info(context,
             "  {} {}\n",
             i,
             model.attributes[m_solver.atts[i].att].name.c_str());
}

for_each_model_solver::for_each_model_solver(std::shared_ptr<context> context,
                                             const Model& model,
                                             int walker_number)
  : m_context(context)
  , m_solver(model)
{
    full();
    init_walkers(walker_number);

    detect_missing_scale_value();

    for (size_t i = 0, e = m_solver.atts.size(); i != e; ++i)
        info(context,
             "  {} {}\n",
             i,
             model.attributes[m_solver.atts[i].att].name.c_str());
}

/** @e reduce is used to reduce the size of the problem. It removes
 * from the solver, all lines from the solver based on options.
 */
void
for_each_model_solver::reduce(const Options& options)
{
    info(m_context, "[Reducing problem size]");

    m_whitelist.clear();
    m_whitelist.resize(m_solver.attribute_size());

    std::vector<std::set<int>> whitelist;
    whitelist.resize(m_solver.attribute_size());

    for (size_t i = 0, e = options.options.rows(); i != e; ++i)
        m_solver.reduce(options.options.row(i), whitelist);

    for (size_t i = 0, e = whitelist.size(); i != e; ++i) {
        info(m_context, "  Whitelist ");
        for (const auto v : whitelist[i])
            info(m_context, "{} ", v);

        info(m_context, "({})\n", m_solver.function_size(static_cast<int>(i)));
    }

    /* convert the set into vector of vector. */
    for (size_t i = 0, e = whitelist.size(); i != e; ++i) {
        m_whitelist[i].resize(whitelist[i].size());

        std::copy(
          whitelist[i].begin(), whitelist[i].end(), m_whitelist[i].begin());
    }
}

void
for_each_model_solver::init_next_value()
{
    m_solver.reinit();

    for (size_t i = 0, e = m_updaters.size(); i != e; ++i) {
        const int attribute = m_updaters[i].attribute;
        const int line = m_whitelist[attribute][m_updaters[i].line];

        m_solver.value_clear(attribute, line);
    }
}

bool
for_each_model_solver::next_value()
{
    assert(!m_updaters.empty() && m_updaters.size() < INT_MAX);

    size_t i = m_updaters.size() - 1;

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

bool
for_each_model_solver::init_walkers(size_t walker_numbers)
{
    assert(walker_numbers > 0);

    m_updaters.resize(walker_numbers);

    size_t i = 0;
    int attribute = m_updaters[i].attribute = 0;
    int line = m_updaters[i].line = 0;

    for (auto j = i + 1; j < m_updaters.size(); ++j) {
        m_updaters[j].attribute = attribute;
        m_updaters[j].line = ++line;

        if (static_cast<size_t>(m_updaters[j].line) >=
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

bool
for_each_model_solver::next_line()
{
    assert(!m_updaters.empty() && m_updaters.size() < INT_MAX);

    for (;;) {
        size_t i = m_updaters.size() - 1;

        for (;;) {
            if (static_cast<size_t>(m_updaters[i].line + 1) <
                m_whitelist[m_updaters[i].attribute].size()) {
                ++m_updaters[i].line;

                int attribute = m_updaters[i].attribute;
                int line = m_updaters[i].line;
                for (auto j = i + 1; j < m_updaters.size(); ++j) {
                    m_updaters[j].attribute = attribute;
                    m_updaters[j].line = ++line;

                    if (static_cast<size_t>(m_updaters[j].line) >=
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

                        if (static_cast<size_t>(m_updaters[j].line) >=
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

std::vector<std::tuple<int, int, int>>
for_each_model_solver::updaters() const
{
    /* if mode with reduce, we recompute attributes/lines otherwise,
     * we can return m_updaters directly.
     */

    std::vector<std::tuple<int, int, int>> ret;
    ret.reserve(m_updaters.size());

    for (size_t i = 0, e = m_updaters.size(); i != e; ++i) {
        const int attribute = m_updaters[i].attribute;
        const int line = m_whitelist[attribute][m_updaters[i].line];

        ret.emplace_back(attribute, line, m_solver.value(attribute, line));
    }

    return ret;
}

size_t
for_each_model_solver::get_attribute_line_tuple_limit() const
{
    size_t ret = 0;

    for (const auto& att : m_whitelist)
        ret += att.size();

    return ret;
}

void
print(std::shared_ptr<context> ctx,
      const std::vector<std::tuple<int, int, int>>& updaters) noexcept
{
    for (const auto& elem : updaters)
        info(ctx,
             "[{} {} {}] ",
             std::get<0>(elem),
             std::get<1>(elem),
             std::get<2>(elem));
}

} // namespace efyj
