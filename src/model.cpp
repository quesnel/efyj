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

#include "model.hpp"
#include <algorithm>

namespace efyj {

void attribute::fill_utility_function()
{
    std::vector <scale_id> child_max_value(children.size(), 0u);
    std::transform(children.begin(), children.end(),
                   child_max_value.begin(),
                   [](const attribute* att)
                   {
                       return att->scale_size();
                   });

    std::vector <scale_id> iter(children.size(), 0u);
    bool end = false;

    std::size_t id = 0;
    do {
        std::size_t key = 0;
        for (std::size_t i = 0, e = iter.size(); i != e; ++i) {
            key *= 10;
            key += iter[i];
        }
        utility_function.emplace(key, functions.low[id] - '0');

        std::size_t current = child_max_value.size() - 1;

        do {
            iter[current]++;
            if (iter[current] >= child_max_value[current]) {
                iter[current] = 0;

                if (current == 0) {
                    end = true;
                    break;
                } else {
                    --current;
                }
            } else
                break;
        } while (not end);
        ++id;
    } while (not end);
}

void Model::init()
{
    for (auto& att : attributes)
        if (not att.is_basic())
            att.fill_utility_function();
}

bool operator==(const attribute& lhs, const attribute& rhs)
{
    return lhs.name == rhs.name;
}

bool operator==(const Model& lhs, const Model& rhs)
{
    if (not (lhs.options == rhs.options &&
             lhs.group == rhs.group &&
             lhs.attributes == rhs.attributes))
        return false;

    if (lhs.child and rhs.child)
        return (*lhs.child) == (*rhs.child);

    if (lhs.child == nullptr and rhs.child == nullptr)
        return true;

    return false;
}

bool operator!=(const Model& lhs, const Model& rhs)
{
    return not (lhs == rhs);
}

}
