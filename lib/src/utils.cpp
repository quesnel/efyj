/* Copyright (C) 2016 INRA
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

#include <thread>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

namespace {

inline unsigned
hardware_concurrency_from_std() noexcept
{
    unsigned ret = std::thread::hardware_concurrency();

    return ret == 0 ? 1 : ret;
}

} // anonymous namespace

namespace efyj {

unsigned get_hardware_concurrency() noexcept
{
#ifdef HAVE_UNISTD_H
    long nb_procs = ::sysconf(_SC_NPROCESSORS_ONLN);
    if (nb_procs == -1)
        return ::hardware_concurrency_from_std();

    return static_cast<unsigned>(nb_procs);
#else
    return ::hardware_concurrency_from_std();
#endif
}

} // namespace efyj
