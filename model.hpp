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
        scalevalue(const std::string& name)
            : name(name)
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
    };

    struct attribute
    {
        attribute(const std::string& name)
            : name(name)
        {}

        std::string name;
        std::string description;
        std::vector <scalevalue> scale;
        function functions;
        std::vector <std::string> options;
        std::vector <std::deque <attribute>::pointer> children;
    };

    typedef std::deque <attribute>::pointer attribute_id;

    struct dexi
    {
        std::vector <std::string> options;
        group_set group;
        std::deque <attribute> attributes;
        attribute_id child;
    };

    bool operator==(const dexi& lhs, const dexi& rhs);
}

#endif
