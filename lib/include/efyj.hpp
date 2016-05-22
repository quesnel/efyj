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

#ifndef FR_INRA_EFYJ_EFYJ_HPP
#define FR_INRA_EFYJ_EFYJ_HPP

#if defined _WIN32 || defined __CYGWIN__
#define EFYJ_HELPER_DLL_IMPORT __declspec(dllimport)
#define EFYJ_HELPER_DLL_EXPORT __declspec(dllexport)
#define EFYJ_HELPER_DLL_LOCAL
#else
# if __GNUC__ >= 4
#  define EFYJ_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
#  define EFYJ_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
#  define EFYJ_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
# else
#  define EFYJ_HELPER_DLL_IMPORT
#  define EFYJ_HELPER_DLL_EXPORT
#  define EFYJ_HELPER_DLL_LOCAL
# endif
#endif

#ifdef EFYJ_BUILD_SHARED_LIBRARY
# ifdef libefyj_EXPORTS
#  define EFYJ_API EFYJ_HELPER_DLL_EXPORT
# else
#  define EFYJ_API EFYJ_HELPER_DLL_IMPORT
# endif
# define EFYJ_LOCAL EFYJ_HELPER_DLL_LOCAL
# define EFYJ_MODULE EFYJ_HELPER_DLL_EXPORT
#else
# define EFYJ_API
# define EFYJ_LOCAL
# define EFYJ_MODULE
#endif

#include <memory>
#include <vector>
#include <stdexcept>

/** Comments about efyj's API.
 */
namespace efyj {

struct modifier {
    int attribute;
    int line;
    int value;
};

struct result {
    std::vector <modifier> modifiers;
    double kappa;
    double time;
    unsigned long int kappa_computed;
    unsigned long int function_computed;
};

class EFYJ_API internal_error : std::logic_error
{
    std::string pp_file, pp_function;
    int pp_line;

public:
    internal_error(const std::string& msg);
    internal_error(const std::string& file,
                   const std::string& function,
                   int line,
                   const std::string& msg);
    virtual ~internal_error() noexcept = default;

    std::string file() const;
    std::string function() const;
    int line() const;
};

class EFYJ_API file_error : public std::runtime_error
{
    std::string pp_file;

public:
    file_error(const std::string& file);

    virtual ~file_error() noexcept = default;

    std::string file() const;
};

class EFYJ_API solver_error : public std::runtime_error
{
public:
    solver_error(const std::string &msg);

    virtual ~solver_error() noexcept = default;
};

class EFYJ_API dexi_parser_error : public std::runtime_error
{
public:
    dexi_parser_error(const std::string &msg);

    dexi_parser_error(const std::string &msg, const std::string &filepath);

    dexi_parser_error(const std::string &msg, int line, int column, int error);

    virtual ~dexi_parser_error() noexcept = default;

    int line() const { return m_line; }
    int column() const { return m_column; }
    int internal_error_code() const { return m_internal_error_code; }
    std::string filepath() const { return m_filepath; }
    std::string message() const { return m_message; }

private:
    int m_line, m_column;
    int m_internal_error_code;
    std::string m_filepath;
    std::string m_message;
};

class EFYJ_API csv_parser_error : public std::runtime_error
{
public:
    csv_parser_error(const std::string &msg);

    csv_parser_error(const std::string &filepath, const std::string &msg);

    csv_parser_error(std::size_t line, const std::string &filepath,
                     const std::string &msg);

    csv_parser_error(std::size_t line, std::size_t column,
                     const std::string &filepath,
                     const std::string &msg);

    virtual ~csv_parser_error() noexcept = default;

    std::size_t line() const { return m_line; }
    std::size_t column() const { return m_column; }
    std::string filepath() const { return m_filepath; }
    std::string message() const { return m_msg; }

private:
    std::size_t m_line;
    std::size_t m_column;
    std::string m_filepath;
    std::string m_msg;
};

class EFYJ_API efyj
{
    struct pimpl;
    std::unique_ptr<pimpl> pp_impl;

public:
    efyj();

    explicit efyj(const std::string& model_filepath);

    explicit efyj(const std::string& model_filepath,
                  const std::string& options_filepath);

    /** No copy constructor. */
    efyj(const efyj& other) = delete;

    /** No assigment operator. */
    efyj& operator=(const efyj& other) = delete;

    efyj(efyj&& other);
    efyj& operator=(efyj&& other);

    ~efyj() noexcept;

    void extract_options(const std::string& filepath) const;

    void extract_options(std::vector <std::string>& simulations,
                         std::vector <std::string>& places,
                         std::vector <int>& departments,
                         std::vector <int>& years,
                         std::vector <int>& observed,
                         std::vector <int>& options) const;

    void set_options(const std::vector <std::string>& simulations,
                     const std::vector <std::string>& places,
                     const std::vector <int>& departments,
                     const std::vector <int>& years,
                     const std::vector <int>& observed,
                     const std::vector <int>& options);

    int solve(const std::vector<int>& options);

    result compute_kappa() const;

    std::vector<result>
        compute_prediction(int line_limit,
                           double time_limit,
                           int reduce_mode) const;

    std::vector<result>
        compute_adjustment(int line_limit,
                           double time_limit,
                           int reduce_mode) const;

    void clear();
};

} // namespace efyj

#endif
