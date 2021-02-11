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

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include <efyj/efyj.hpp>
#include "utils.hpp"
#include "model.hpp"

#include <cassert>
#include <cstring>

namespace efyj {

using scale_id = int;

template<typename T>
void
is_valid_scale_id([[maybe_unused]] T n) noexcept
{
    assert(n >= 0 && n <= 127);
}

constexpr scale_id
scale_id_unknown() noexcept
{
    return std::numeric_limits<scale_id>::max();
}

struct scalevalue
{
    scalevalue(const std::string& name_)
      : name(name_)
      , group(-1)
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
        return low.empty() && entered.empty() && consist.empty();
    }
};

struct scales
{
    scales()
      : order(true)
    {}

    bool order;
    std::vector<scalevalue> scale;

    std::optional<scale_id> find_scale_value(const std::string& name) const
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
    attribute(const std::string& name_)
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

    std::string name;
    std::string description;
    scales scale;
    function functions;
    std::vector<int> options;
    std::vector<size_t> children;
};

struct Model
{
    std::string name;
    std::string version;
    std::string created;
    std::string reports;
    std::string optdatatype;
    std::string optlevels;
    std::vector<std::string> description;
    std::vector<std::string> options;
    std::vector<scale_id> basic_attribute_scale_size;
    std::vector<std::string> group;
    std::deque<attribute> attributes;

    void read(const context& ctx, const input_file& is);

    void write(const context& ctx, const output_file& os);

    /** Release all dynamically allocated memory. */
    void clear();

    bool empty() const noexcept
    {
        return attributes.empty();
    }

    int group_id(const std::string& name) const
    {
        auto it = std::find(group.cbegin(), group.cend(), name);

        if (it == group.cend())
            return -1;

        return static_cast<int>(std::distance(group.cbegin(), it));
    }

    std::vector<const attribute*> get_basic_attribute() const
    {
        std::vector<const attribute*> ret;
        ret.reserve(attributes.size());

        for (const auto& att : attributes)
            if (att.is_basic())
                ret.emplace_back(&att);

        return ret;
    }

    void write_options(const output_file& os) const;
    options_data write_options() const;
    void set_options(const options_data& options);
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
