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

#ifndef INRA_EFYj_DBG_HPP
#define INRA_EFYj_DBG_HPP

#include <string>
#include <stdexcept>

namespace efyj {

    struct assert_error: std::logic_error
    {
        explicit assert_error(const std::string& condition,
                              const std::string& line,
                              const std::string& file)
            : std::logic_error(" Assertion `" + condition + "' failed in file "
                               + file + " at line " + line)
        {}
    };

}

#define dWHITE "\x1b[37;1m"
#define dRED "\x1b[31;1m"
#define dYELLOW "\x1b[33;1m"
#define dCYAN "\x1b[36;1m"
#define dNORMAL "\x1b[0;m"

#ifdef NDEBUG
#define dError(...)
#define dWarning(...)
#define dInfo(...)
#define dAssert(x)
#else
#define dError(...) do { efyj::dbg_print(efyj::DBG_LEVEL_ERROR, __VA_ARGS__); } while (0)
#define dWarning(...) do { efyj::dbg_print(efyj::DBG_LEVEL_WARNING, __VA_ARGS__); } while (0)
#define dInfo(...) do { efyj::dbg_print(efyj::DBG_LEVEL_INFO, __VA_ARGS__); } while (0)
#define dAssert(condition_) \
    do { \
        if (!(condition_)) { \
            dError("Assertion: ", dYELLOW, "`", #condition_, "'", dRED, \
                   " failed (value: ", dYELLOW, "`", (condition_), "')", dRED, \
                   " in file ", dYELLOW, __FILE__, dRED, " at line ", \
                   dYELLOW, __LINE__, dRED, "\n      in function: ", \
                   dYELLOW, __PRETTY_FUNCTION__); \
            throw efyj::assert_error(#condition_, std::to_string(__LINE__), \
                                     __FILE__); \
        } \
    } while (0)
#endif

#include <iostream>

namespace efyj {

    enum dbg_level
    {
        DBG_LEVEL_ERROR,
        DBG_LEVEL_WARNING,
        DBG_LEVEL_INFO
    };

    template <typename... ArgTypes>
        inline void print(const ArgTypes&... args)
        {
            using expand_variadic_pack = int[];

            (void)expand_variadic_pack{0, ((std::cerr << args), void(), 0)... };
        }

    template <typename... ArgTypes>
        inline void dbg_print(dbg_level level, ArgTypes... args)
        {
            switch (level) {
            case DBG_LEVEL_ERROR:
                std::cerr << dRED << "[ERR] ";
                break;
            case DBG_LEVEL_WARNING:
                std::cerr << dYELLOW << "[WRN] ";
                break;
            case DBG_LEVEL_INFO:
                std::cerr << dCYAN << "[INF] ";
                break;
            }

            print(args...);

            std::cerr << dNORMAL << std::endl;
        }

}

#endif
