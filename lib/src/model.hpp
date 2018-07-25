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

#ifndef ORG_VLEPROJECT_EFYJ_MODEL_HPP
#define ORG_VLEPROJECT_EFYJ_MODEL_HPP

#include <EASTL/algorithm.h>
#include <EASTL/deque.h>
#include <EASTL/initializer_list.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/optional.h>
#include <EASTL/stack.h>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

#include <efyj/efyj.hpp>

#include "model.hpp"

#include <cassert>
#include <cstring>

namespace efyj {

using scale_id = int;

template<typename T>
void
is_valid_scale_id(T n) noexcept
{
    assert(n >= 0 and n <= 127);
}

constexpr scale_id
scale_id_unknown() noexcept
{
    return eastl::numeric_limits<scale_id>::max();
}

struct scalevalue
{
    scalevalue(const eastl::string& name_)
      : name(name_)
      , group(-1)
    {}

    eastl::string name;
    eastl::string description;
    int group;
};

struct function
{
    eastl::string low;
    eastl::string entered;
    eastl::string consist;

    bool empty() const noexcept
    {
        return low.empty() && entered.empty() && consist.empty();
    }
};

struct scales
{
    scales()
      : order(true)
    {}

    bool order;
    eastl::vector<scalevalue> scale;

    eastl::optional<scale_id> find_scale_value(const eastl::string& name) const
    {
        for (size_t i = 0, e = scale.size(); i != e; ++i) {
            if (scale[i].name == name) {
                is_valid_scale_id(i);

                return static_cast<int>(i);
            }
        }

        return {};
    }

    scale_id size() const
    {
        is_valid_scale_id(scale.size());

        return static_cast<int>(scale.size());
    }
};

struct attribute
{
    attribute(const eastl::string& name_)
      : name(name_)
    {}

    size_t children_size() const noexcept
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

    void push_back(size_t child)
    {
        children.emplace_back(child);
    }

    eastl::string name;
    eastl::string description;
    scales scale;
    function functions;
    eastl::vector<int> options;
    eastl::vector<size_t> children;
};

struct Model
{
    eastl::string name;
    eastl::string version;
    eastl::string created;
    eastl::string reports;
    eastl::vector<eastl::string> description;
    eastl::vector<eastl::string> options;
    eastl::vector<scale_id> basic_attribute_scale_size;
    eastl::vector<eastl::string> group;
    eastl::deque<attribute> attributes;

    void read(FILE* is);

    void write(FILE* os);

    /** Release all dynamically allocated memory. */
    void clear();

    bool empty() const noexcept
    {
        return attributes.empty();
    }

    int group_id(const eastl::string& name) const
    {
        auto it = eastl::find(group.cbegin(), group.cend(), name);

        if (it == group.cend())
            return -1;

        return static_cast<int>(eastl::distance(group.cbegin(), it));
    }

    void write_options(FILE* os) const;
    options_data write_options() const;
};

bool
operator<(const Model& lhs, const Model& rhs);
bool
operator==(const Model& lhs, const Model& rhs);
bool
operator!=(const Model& lhs, const Model& rhs);

struct str_compare
{
    inline bool operator()(const char* lhs, const char* rhs) const noexcept
    {
        return strcmp(lhs, rhs) == 0;
    }
};

struct str_hash
{
    inline size_t operator()(const char* str) const noexcept
    {
        size_t hash = 0;
        int c;

        while ((c = *str++) != '\0')
            hash = c + (hash << 6) + (hash << 16) - hash;

        return hash;
    }
};

inline bool
operator==(const scalevalue& lhs, const scalevalue& rhs)
{
    return lhs.name == rhs.name && lhs.description == rhs.description &&
           lhs.group == rhs.group;
}

inline bool
operator==(const scales& lhs, const scales& rhs)
{
    return lhs.order == rhs.order && lhs.scale == rhs.scale;
}

inline bool
operator==(const function& lhs, const function& rhs)
{
    return lhs.low == rhs.low && lhs.entered == rhs.entered &&
           lhs.consist == rhs.consist;
}

inline bool
operator==(const attribute& lhs, const attribute& rhs)
{
    return lhs.name == rhs.name && lhs.description == rhs.description &&
           lhs.scale == rhs.scale && lhs.functions == rhs.functions &&
           lhs.children == rhs.children;
}

inline bool
operator<(const Model& lhs, const Model& rhs)
{
    return lhs.name < rhs.name;
}

inline bool
operator==(const Model& lhs, const Model& rhs)
{
    return lhs.name == rhs.name && lhs.version == rhs.version &&
           lhs.created == rhs.created && lhs.description == rhs.description &&
           lhs.options == rhs.options && lhs.reports == rhs.reports &&
           lhs.basic_attribute_scale_size == rhs.basic_attribute_scale_size &&
           lhs.group == rhs.group && lhs.attributes == rhs.attributes;
}

inline bool
operator!=(const Model& lhs, const Model& rhs)
{
    return !(lhs == rhs);
}

inline bool
operator<(const attribute& lhs, const attribute& rhs)
{
    return lhs.name < rhs.name;
}

} // namespace efyj

#endif
