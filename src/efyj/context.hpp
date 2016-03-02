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

#ifndef INRA_EFYj_CONTEXT_HPP
#define INRA_EFYj_CONTEXT_HPP

#include <efyj/efyj.hpp>
#include <fstream>
#include <memory>

namespace efyj {

enum LogOption
{
    LOG_OPTION_NOTHING,
    LOG_OPTION_ERROR,
    LOG_OPTION_INFO,
    LOG_OPTION_DEBUG
};

/**
 * @attention Prefers to use a context object per thread to avoid the use
 * of mutex, conditions etc.
 *
 * @code
 * auto ctx = std::make_shared<efyj::Context>(efyj::LOG_OPTION_DEBUG);
 * ctx->set_log_file_stream("efyj.log");
 *
 * ctx->info() << "Context initialized.\n";
 * @endcode
 */
class EFYJ_API Context
{
public:
    Context(LogOption option = LOG_OPTION_DEBUG);

    Context(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&) = delete;

    ~Context();

    void set_log_file_stream(const std::string &filepath) noexcept;

    std::string get_log_filename() const noexcept;

    void set_console_log_stream();

    std::ostream& info() const noexcept;

    std::ostream& dbg() const noexcept;

    std::ostream& err() const noexcept;

    LogOption log_priority() const;

    void set_log_priority(LogOption priority);

    std::string log_dirname() const;

    std::string log_template() const;

private:
    std::shared_ptr <std::ostream> m_os;
    mutable std::ofstream m_null_os;
    std::string m_log_filename;
    LogOption m_log_priority;
};

}

#endif
