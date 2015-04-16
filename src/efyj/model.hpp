/* Copyright (C) 2014-2015 INRA
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

struct scalevalue
{
    scalevalue(const std::string& name)
        : name(name), group(-1)
    {}

    std::string name;
    std::string description;
    int group;
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
        : name(name)
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

    void push_back(std::size_t child)
    {
        children.emplace_back(child);
    }

    std::string name;
    std::string description;
    scales scale;
    function functions;
    std::vector <std::string> options;
    std::vector <std::size_t> children;
};

struct Model
{
    std::string name;
    std::string description;
    std::vector <std::string> options;
    std::vector <scale_id> basic_attribute_scale_size;
    std::vector <std::string> group;
    std::deque <attribute> attributes;
    
    int group_id(const std::string& name) const
    {
        auto it = std::find(group.cbegin(), group.cend(), name);

        if (it == group.cend())
            return -1;

        return it - group.cbegin();
    }

    const attribute& child() const
    {
        return attributes[0];
    }
};

bool operator<(const Model& lhs, const Model& rhs);
bool operator==(const Model& lhs, const Model& rhs);

std::ostream& operator<<(std::ostream& os, const Model& Model_data);
std::istream& operator>>(std::istream& is, Model& Model_data);

}

#include "details/model-implementation.hpp"

#endif
