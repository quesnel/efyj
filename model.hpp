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
#include <set>
#include <string>
#include <vector>
#include <cstdint>

namespace efyj {

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
    };

    struct attribute
    {
        attribute(const std::string& name)
            : name(name)
        {}

        std::string name;
        std::string description;
        scales scale;
        function functions;
        std::vector <std::string> options;
        std::vector <attribute*> children;
    };

    struct dexi
    {
        dexi()
            : child(nullptr)
        {}

        dexi(const dexi& other) = delete;
        dexi(dexi&& other) = delete;
        dexi& operator=(const dexi& other) = delete;
        dexi& operator=(dexi&& other) = delete;

        std::string name;
        std::string description;
        std::vector <std::string> options;
        group_set group;
        std::deque <attribute> attributes;
        attribute *child;
    };

    bool operator==(const dexi& lhs, const dexi& rhs);
    bool operator!=(const dexi& lhs, const dexi& rhs);
}

#endif
