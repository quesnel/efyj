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

#ifndef INRA_EFYj_INTERNAL_PREDICTION_HPP
#define INRA_EFYj_INTERNAL_PREDICTION_HPP

namespace efyj {

void prediction_0(std::shared_ptr<Context> context,
                  const Model& model,
                  const Options& options);

void prediction_n(std::shared_ptr<Context> context,
                  const Model& model,
                  const Options& options,
                  unsigned int threads);

/** \e make_new_name is used to create new file path with a suffix composed with
 * an identifier.
 * \param filepath Original filepath to be updated.  \e filepath can be empty or
 * have and extension.
 * \param id Identifier to be attached to the origin filepath.
 * \return A new string represents modified \e filepath with the \e identifier.
 *
 * \example
 * \code
 * assert(make_new_name("example.dat", 0) == "example-0.dat");
 * assert(make_new_name("", 0) == "worker-0.dat");
 * assert(make_new_name("x.y.example.dat", 0) == "x.y.example-0.dat");
 * assert(make_new_name(".zozo", 0) == "worker-0.dat");
 * \endcode
 * \endexample
inline
std::string
make_new_name(const std::string& filepath, unsigned int id) noexcept
{
    if (filepath.empty()) {
        std::ostringstream os;
        os << "worker-" << id << ".log";
        return os.str();
    }

    auto dotposition = filepath.find_last_of('.');

    if (dotposition == 0u) {
        std::ostringstream os;
        os << "worker-" << id << ".log";
        return os.str();
    }

    if (dotposition == filepath.size() - 1) {
        std::ostringstream os;
        os << filepath.substr(0, dotposition)
           << '-' << id;

        return os.str();
    }

    std::ostringstream os;
    os << filepath.substr(0, dotposition)
       << '-' << id
       << filepath.substr(dotposition + 1);

    return os.str();
}

} // namespace efyj

#endif
