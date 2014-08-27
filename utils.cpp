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

#include "utils.hpp"
#include <vector>
#include <cstdio>
#include <cstdarg>

namespace efyj {

std::string stringf(const char* format, ...)
{
    std::vector <char> buffer(1024, '\0');
    int sz;
    va_list ap;

    for (;;) {
        va_start(ap, format);
        sz = std::vsnprintf(buffer.data(), buffer.size(), format, ap);
        va_end(ap);

        if (sz < 0)
            return std::move(std::string());
        else if (static_cast <std::size_t>(sz) < buffer.size())
            return std::move(std::string(buffer.data(), buffer.size()));
        else
            buffer.resize(sz + 1);
    }
}

}
