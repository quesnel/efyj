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

#include <boost/format.hpp>
#include <memory>
#include <functional>

#define DEBUG_MESSAGE efyj::LOG_OPTION_DEBUG, __FILE__, __LINE__, __PRETTY_FUNCTION__
#define INFO_MESSAGE efyj::LOG_OPTION_INFO, __FILE__, __LINE__, __PRETTY_FUNCTION__
#define ERR_MESSAGE efyj::LOG_OPTION_ERR, __FILE__, __LINE__, __PRETTY_FUNCTION__

namespace efyj {

using fmt = boost::format;

class ContextImpl;

typedef std::shared_ptr <ContextImpl> Context;

enum LogOption { LOG_OPTION_DEBUG, LOG_OPTION_INFO, LOG_OPTION_ERR };

typedef std::function <void(const ContextImpl&, int, const char *,
                            int, const char *, const efyj::fmt& fmt)> log_function;

class ContextImpl
{
public:
    ContextImpl();

    ~ContextImpl();

    Context create();

    void set_log_stream(std::ostream* os);

    void set_error_stream(std::ostream* os);

    void set_log_function(log_function fct);

    void log(const efyj::fmt& fmt);

    void log(int priority, const char *file,
             int line, const char *fn,
             const efyj::fmt& fmt);

    LogOption log_priority() const;

    void set_log_priority(LogOption priority);

private:
    struct impl;
    impl* m_impl;
};

}

#endif
