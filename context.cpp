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

#include <efyj/context.hpp>
#include <iostream>
#include <vector>
#include <cstdio>

namespace efyj {

void default_log_fct(const ContextImpl& ctx, int priority, const char *file,
                     int line, const char *fn, const char *format,
                     va_list args)
{
    (void)ctx;

    switch (priority) {
    case efyj::LOG_OPTION_DEBUG:
        std::cout << "Debug " << file << ":" << line  << " " << fn << " ";
        break;
    case efyj::LOG_OPTION_INFO:
        break;
    case efyj::LOG_OPTION_ERR:
        std::cout << "Error " << file << ":" << line  << " " << fn << " ";
        break;
    }

    std::vector <char> buffer(1024, '\0');

    for (;;) {
        int sz = std::vsnprintf(buffer.data(), buffer.size() - 1, format, args);

        if (sz < 0) {
            break;
        } else if (static_cast <std::size_t>(sz) < buffer.size()) {
            buffer[sz] = '\0';
            std::cout << buffer.data();
        } else {
            buffer.resize(sz + 1);
        }
    }

    std::cout << '\n';
}


struct ContextImpl::impl
{
    impl()
        : fct(&default_log_fct)
        , priority(LOG_OPTION_DEBUG)
    {
    }

    log_function fct;
    LogOption priority;
};

Context ContextImpl::create()
{
    return std::make_shared <ContextImpl>();
}

ContextImpl::ContextImpl()
    : m_impl(new ContextImpl::impl)
{
}

ContextImpl::~ContextImpl()
{
    delete m_impl;
}

void ContextImpl::set_log_function(log_function fct)
{
    m_impl->fct = fct;
}

void ContextImpl::log(int priority, const char *file,
                      int line, const char *fn,
                      const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    try {
        m_impl->fct(*this, priority, file, line, fn, format, ap);
    } catch (...) {
    }

    va_end(ap);
}

LogOption ContextImpl::log_priority() const
{
    return m_impl->priority;
}

void ContextImpl::set_log_priority(LogOption priority)
{
    m_impl->priority = priority;
}

}
