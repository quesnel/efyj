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

#ifndef INRA_EFYj_VISIBILITY_HPP
#define INRA_EFYj_VISIBILITY_HPP

/*
 * See. http://gcc.gnu.org/wiki/Visibility
 */

#if defined _WIN32 || defined __CYGWIN__
#  define EFYJ_HELPER_DLL_IMPORT __declspec(dllimport)
#  define EFYJ_HELPER_DLL_EXPORT __declspec(dllexport)
#  define EFYJ_HELPER_DLL_LOCAL
#else
#  if __GNUC__ >= 4
#    define EFYJ_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
#    define EFYJ_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
#    define EFYJ_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#    define EFYJ_HELPER_DLL_IMPORT
#    define EFYJ_HELPER_DLL_EXPORT
#    define EFYJ_HELPER_DLL_LOCAL
#  endif
#endif

/*
 * Now we use the generic helper definitions above to define EFYJ_API
 * and EFYJ_LOCAL. EFYJ_API is used for the public API symbols. It
 * either DLL imports or DLL exports (or does nothing for static
 * build) EFYJ_LOCAL is used for non-api symbols.
 */

#ifdef efyj_DLL
#  ifdef efyj_EXPORTS
#    define EFYJ_API EFYJ_HELPER_DLL_EXPORT
#  else
#    define EFYJ_API EFYJ_HELPER_DLL_IMPORT
#  endif
#  define EFYJ_LOCAL EFYJ_HELPER_DLL_LOCAL
#  define EFYJ_MODULE EFYJ_HELPER_DLL_EXPORT
#else
#  define EFYJ_API
#  define EFYJ_LOCAL
#  define EFYJ_MODULE EFYJ_HELPER_DLL_EXPORT
#endif

#endif

