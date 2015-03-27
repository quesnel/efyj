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

#ifndef INRA_EFYj_MODEL_HPP
#define INRA_EFYj_MODEL_HPP

#include <deque>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <vector>
#include <cstdint>

namespace efyj {

/**
 * The @e scale_id is used to represent possible value when solving
 * problem. The range of @e scale_id is [0..127].
 */
// typedef std::uint_fast8_t scale_id;
typedef int scale_id;

template <typename T>
constexpr typename std::enable_if <std::is_unsigned <T>::value, bool>::type
is_valid_scale_id(T n) noexcept
{
    return n <= 127;
}

template <typename T>
constexpr typename std::enable_if <!std::is_unsigned <T>::value, bool>::type
is_valid_scale_id(T n) noexcept
{
    return n >= 0 && n <= 127;
}

constexpr scale_id scale_id_unknown() noexcept
{
    return std::numeric_limits <scale_id>::max();
}

typedef std::set <std::string> group_set;

struct scalevalue
{
    scalevalue(const std::string& name, group_set::iterator group)
        : name(name), group(group)
    {}

    std::string name;
    std::string description;
    group_set::iterator group;
};

struct function
{
    std::string low;
    std::string entered;
    std::string consist;

    bool empty() const noexcept
    {
        return low.empty() and entered.empty() and consist.empty();
    }
};

struct scales
{
    scales()
        : order(true)
    {}

    bool order;
    std::vector <scalevalue> scale;

    int find_scale_value(const std::string& name) const
    {
        for (std::size_t i = 0, e = scale.size(); i != e; ++i)
            if (scale[i].name == name)
                return static_cast <int>(i);

        return -1;
    }

    scale_id size() const noexcept
    {
        return scale.size();
    }
};

struct attribute
{
    attribute(const std::string& name)
        : parent(nullptr)
        , position_in_parent(0)
        , value(0)
        , parent_is_solvable(0)
        , name(name)
    {}

    std::size_t children_size() const noexcept
    {
        return children.size();
    }

    std::size_t scale_size() const noexcept
    {
        return scale.size();
    }

    bool is_basic() const noexcept
    {
        return children.empty();
    }

    bool is_aggregate() const noexcept
    {
        return !children.empty();
    }

    void push_back(attribute *child)
    {
        children.emplace_back(child);
        child->position_in_parent = children.size();
        child->parent = this;
    }

    void fill_utility_function();

    attribute *parent;
    std::size_t position_in_parent;
    std::size_t value;
    std::size_t parent_is_solvable;
    std::string name;
    std::string description;
    scales scale;
    function functions;
    std::vector <std::string> options;
    std::vector <attribute*> children;
    std::map <std::uint32_t, scale_id> utility_function;
};

struct dexi
{
    dexi()
        : child(nullptr)
        , problem_size(1)
        , basic_scale_number(0)
        , scale_number(0)
        , scalevalue_number(0)
    {}

    dexi(const dexi& other) = delete;
    dexi(dexi&& other) = delete;
    dexi& operator=(const dexi& other) = delete;
    dexi& operator=(dexi&& other) = delete;

    void init();

    std::string name;
    std::string description;
    std::vector <std::string> options;
    std::vector <scale_id> basic_attribute_scale_size;
    group_set group;
    std::deque <attribute> attributes;
    attribute *child;

    std::size_t problem_size;
    std::size_t basic_scale_number;
    std::size_t scale_number;
    std::size_t scalevalue_number;
};

bool operator==(const dexi& lhs, const dexi& rhs);
bool operator!=(const dexi& lhs, const dexi& rhs);

std::ostream& operator<<(std::ostream& os, const dexi& dexi_data);
std::istream& operator>>(std::istream& is, dexi& dexi_data);

}

#endif
