/* Copyright (C) 2015 INRA
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

#ifndef INRA_EFYj_MESSAGE_HPP
#define INRA_EFYj_MESSAGE_HPP

#include <string>
#include <thread>
#include <ostream>
#include <boost/format.hpp>

namespace efyj {

enum LogOption { LOG_OPTION_DEBUG, LOG_OPTION_INFO, LOG_OPTION_ERR };

struct message {
    template <typename T>
    message(LogOption priority, const T &t);

    template <typename T>
    message(LogOption priority, const char *file, int line, const char *fn,
            const T &t);

    std::string msg;
    std::string file;
    std::string fn;
    std::thread::id thread_id;
    LogOption priority;
    unsigned long int pid;
    int line;
};

std::ostream &operator<<(std::ostream &os, const message &msg);

}

#include "details/message-implementation.hpp"

#endif
