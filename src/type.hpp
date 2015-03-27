#ifndef INRA_EFYj_TYPE_HPP
#define INRA_EFYj_TYPE_HPP

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#endif

#include <Eigen/src/Core/util/DisableStupidWarnings.h>
#include <Eigen/Core>

namespace efyj {
typedef Eigen::ArrayXXi Array;
typedef Array::RowXpr VectorRef;
typedef Eigen::VectorXi Vector;
}
#endif
