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

#ifndef INRA_EFYj_MODEL_HPP
#define INRA_EFYj_MODEL_HPP

#include <efyj/efyj.hpp>
#include <efyj/exception.hpp>
#include <efyj/cstream.hpp>
#include <algorithm>
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
//typedef std::int_fast8_t scale_id;
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

struct EFYJ_API scalevalue
{
    scalevalue(const std::string& name)
        : name(name), group(-1)
    {}

    std::string name;
    std::string description;
    int group;
};

struct EFYJ_API function
{
    std::string low;
    std::string entered;
    std::string consist;

    bool empty() const noexcept
    {
        return low.empty() and entered.empty() and consist.empty();
    }
};

struct EFYJ_API scales
{
    scales()
        : order(true)
    {}

    bool order;
    std::vector <scalevalue> scale;

    scale_id find_scale_value(const std::string& name) const
    {
        for (std::size_t i = 0, e = scale.size(); i != e; ++i) {
            if (scale[i].name == name) {
                if (not is_valid_scale_id(i)) {
                    throw efyj_error("bad scale definition");
                }
                return static_cast <int>(i);
            }
        }

        throw efyj_error("scale not found");
    }

    scale_id size() const noexcept
    {
        if (not is_valid_scale_id(scale.size()))
            throw efyj_error("bad scale definition");

        return static_cast <int>(scale.size());
    }
};

struct EFYJ_API attribute
{
    attribute(const std::string& name)
        : name(name)
    {}

    std::size_t children_size() const noexcept
    {
        return children.size();
    }

    scale_id scale_size() const noexcept
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
    std::vector <int> options;
    std::vector <std::size_t> children;
};

struct EFYJ_API Model
{
    std::string name;
    std::string version;
    std::string created;
    std::string reports;
    std::vector <std::string> description;
    std::vector <std::string> options;
    std::vector <scale_id> basic_attribute_scale_size;
    std::vector <std::string> group;
    std::deque <attribute> attributes;

    void read(std::istream& is);

    void write(std::ostream& os);

    /** Release all dynamically allocated memory. */
    void clear();

    int group_id(const std::string& name) const
    {
        auto it = std::find(group.cbegin(), group.cend(), name);

        if (it == group.cend())
            return -1;

        return it - group.cbegin();
    }

    void write_options(std::ostream& os) const;
};

EFYJ_API
bool operator<(const Model& lhs, const Model& rhs);
EFYJ_API
bool operator==(const Model& lhs, const Model& rhs);

EFYJ_API
cstream& operator<<(cstream& os, const Model& Model_data) noexcept;

}

#endif
