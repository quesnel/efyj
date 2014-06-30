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

#ifndef INRA_EFYj_LOG_HPP
#define INRA_EFYj_LOG_HPP

#include <fstream>
#include <string>
#include <iostream>

namespace efyj {

    struct log
    {
        log(const std::string& filepath, int id)
            : old_clog_rdbuf(nullptr)
        {
            std::string logfile(filepath);
            logfile += '-' + std::to_string(id) + ".log";

            ofs.open(logfile);
            if (ofs) {
                old_clog_rdbuf = std::clog.rdbuf();
                std::clog.rdbuf(ofs.rdbuf());
            }
        }

        operator bool() const
        {
            return ofs.good();
        }

        ~log()
        {
            if (old_clog_rdbuf)
                std::clog.rdbuf(old_clog_rdbuf);
        }

    private:
        std::ofstream ofs;
        std::streambuf *old_clog_rdbuf;
    };

}

#endif
