/* Copyright (C) 2015 INRA
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

#ifndef INRA_EFYj_CONTEXT_HPP
#define INRA_EFYj_CONTEXT_HPP

#include <memory>
#include <functional>
#include <cstdarg>

#if defined __GNUC__
#define EFYj_GCC_PRINTF(format__, args__)               \
    __attribute__ ((format (printf, format__, args__)))
#endif

#define DEBUG_MESSAGE efyj::LOG_OPTION_DEBUG, __FILE__, __LINE__, __PRETTY_FUNCTION__
#define INFO_MESSAGE efyj::LOG_OPTION_INFO, __FILE__, __LINE__, __PRETTY_FUNCTION__
#define ERR_MESSAGE efyj::LOG_OPTION_ERR, __FILE__, __LINE__, __PRETTY_FUNCTION__

namespace efyj {

class ContextImpl;

typedef std::shared_ptr <ContextImpl> Context;

enum LogOption { LOG_OPTION_DEBUG, LOG_OPTION_INFO, LOG_OPTION_ERR };

typedef std::function <void(const ContextImpl& ctx, int priority, const char *file,
                            int line, const char *fn, const char *format,
                            va_list args)> log_function;

class ContextImpl
{
public:
    ContextImpl();

    ~ContextImpl();

    Context create();

    void set_log_function(log_function fct);

    void log(int priority, const char *file,
             int line, const char *fn,
             const char *formats, ...) EFYj_GCC_PRINTF(6, 7);

    LogOption log_priority() const;

    void set_log_priority(LogOption priority);

private:
    struct impl;
    impl* m_impl;
};

}

#endif
