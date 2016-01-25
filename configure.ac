AC_PREREQ(2.60)
AC_INIT([efyj],
        [1.0],
        [gauthier.quesnel@toulouse.inra.fr],
        [efyj],
        [http://www.vle-project.org/efyj/])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([
	check-news
	foreign
	1.11
	-Wall
	-Wno-portability
	silent-rules
	tar-pax
	no-dist-gzip
	dist-xz
	subdir-objects
])
AC_PROG_CXX
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AC1_SYS_LARGEFILE
AC_CONFIG_MACRO_DIR([m4])
AM_SILENT_RULES([yes])
LT_INIT([
	disable-static
	pic-only
])

AC_LANG(C++)
AX_CHECK_COMPILE_FLAG([-std=c++14], [CXX="$CXX -std=c++14"],
AX_CHECK_COMPILE_FLAG([-std=c++1y], [CXX="$CXX -std=c++1y"],
AX_CHECK_COMPILE_FLAG([-std=c++11], [CXX="$CXX -std=c++1y"],
AX_CHECK_COMPILE_FLAG([-std=c++0x], [CXX="$CXX -std=c++1y"],
AC_MSG_ERROR([Compiler does not support C++11])))))

AX_CHECK_COMPILE_FLAG([-Wall], [CXXFLAGS="$CXXFLAGS -Wall"])
AX_CHECK_COMPILE_FLAG([-Wextra], [CXXFLAGS="$CXXFLAGS -Wextra"])
AX_CHECK_COMPILE_FLAG([-pedantic], [CXXFLAGS="$CXXFLAGS -pedantic"])
AX_CHECK_COMPILE_FLAG([-Wbool-conversion], [CXXFLAGS="$CXXFLAGS -Wbool-conversion"])
AX_CHECK_COMPILE_FLAG([-Wheader-guard], [CXXFLAGS="$CXXFLAGS -Wheader-guard"])
AX_CHECK_COMPILE_FLAG([-Wimplicit-fallthrough], [CXXFLAGS="$CXXFLAGS -Wimplicit-fallthrough"])
AX_CHECK_COMPILE_FLAG([-Wlogical-not-parentheses], [CXXFLAGS="$CXXFLAGS -Wlogical-not-parentheses"])
AX_CHECK_COMPILE_FLAG([-Wloop-analysis], [CXXFLAGS="$CXXFLAGS -Wloop-analysis"])
AX_CHECK_COMPILE_FLAG([-Woverloaded-shift-op-parentheses], [CXXFLAGS="$CXXFLAGS -Woverloaded-shift-op-parentheses"])
AX_CHECK_COMPILE_FLAG([-Wsizeof-array-argument], [CXXFLAGS="$CXXFLAGS -Wsizeof-array-argument"])
AX_CHECK_COMPILE_FLAG([-Wstring-conversion], [CXXFLAGS="$CXXFLAGS -Wstring-conversion"])
AX_CHECK_COMPILE_FLAG([-Wunique-enum], [CXXFLAGS="$CXXFLAGS -Wunique-enum"])
AX_CHECK_COMPILE_FLAG([-Wformat-security], [CXXFLAGS="$CXXFLAGS -Wformat-security"])

# check whether to build tesseract with -fvisibility=hidden
# -fvisibility-inlines-hidden http://gcc.gnu.org/wiki/Visibility
AC_MSG_CHECKING(--enable-visibility argument)
AC_ARG_ENABLE([visibility],
  [AC_HELP_STRING([--enable-visibility],
                  [enable build with fvisibility=hidden (default=yes)])],
  [enable_visibility=$enableval],
  [enable_visibility="yes"])

AC_MSG_RESULT($enable_visibility)
AM_CONDITIONAL([VISIBILITY], [test "$enable_visibility" = "yes"])

AC_PREFIX_DEFAULT([/usr])

AC_CHECK_HEADERS([unistd.h])
AC_LANG_PUSH(C++)
AC_CHECK_HEADER([catch.hpp], [], [AC_MSG_ERROR([Could not find catch.hpp. Please download and install it from https://github.com/philsquared/Catch])])
AC_LANG_POP

AC_PROG_SED
AC_PROG_MKDIR_P

AX_PTHREAD([],[AC_MSG_ERROR([Please install libdw library : libdw-dev])])

AX_LIB_EXPAT([2.1])
enableoptional=yes
CONFIGURE_EIGEN([],[yes])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
        Makefile
])

AC_OUTPUT
AC_MSG_RESULT([
        $PACKAGE $VERSION
        =====

        prefix:                 ${prefix}
        sysconfdir:             ${sysconfdir}
        libdir:                 ${libdir}
        includedir:             ${includedir}

        compiler:               ${CC}
        compiler:               ${CXX}
        cflags:                 ${CFLAGS}
        cxxflags:               ${CXXFLAGS}
        ldflags:                ${LDFLAGS}
])
