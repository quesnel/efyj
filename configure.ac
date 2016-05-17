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

AC_LANG_PUSH(C++)

## Check wheter to build efyj with MPICXX compiler
AC_ARG_ENABLE(mpi,
        [AC_HELP_STRING([--enable-mpi],[compile Efyj with MPI library])],
        enable_mpi=$enableval, enable_mpi=no)

if test "$enable_mpi" = "yes"; then
  AX_MPI([],[AC_MSG_ERROR([could not find mpi library for --enable-mpi])])
  AC_CHECK_PROG(MPIRUN, mpirun, mpirun)
  AC_SUBST(MPIRUN)
  CXX=$MPICXX
fi
AM_CONDITIONAL([EFYJ_HAVE_MPI], test "$enable_mpi" = "yes")

## Check whether to build tesseract with -fvisibility=hidden
## -fvisibility-inlines-hidden http://gcc.gnu.org/wiki/Visibility
AC_MSG_CHECKING(--enable-visibility argument)
AC_ARG_ENABLE([visibility],
  [AC_HELP_STRING([--enable-visibility],
                  [enable build with fvisibility=hidden (default=yes)])],
  [enable_visibility=$enableval],
  [enable_visibility="yes"])

AC_MSG_RESULT($enable_visibility)
AM_CONDITIONAL([VISIBILITY], [test "$enable_visibility" = "yes"])

## Try to append the C++14 flag
AX_CHECK_COMPILE_FLAG([-std=c++14], [CXX="$CXX -std=c++14"],
  AX_CHECK_COMPILE_FLAG([-std=c++1y], [CXX="$CXX -std=c++1y"],
    AX_CHECK_COMPILE_FLAG([-std=c++11], [CXX="$CXX -std=c++1y"],
      AX_CHECK_COMPILE_FLAG([-std=c++0x], [CXX="$CXX -std=c++1y"],
        AC_MSG_ERROR([Compiler does not support C++14])))))

AC_PREFIX_DEFAULT([/usr])

AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADER([catch.hpp],, [missing_catch=yes}])
AM_CONDITIONAL([HAVE_CATCH],[test -z "$missing_catch"])
AC_LANG_POP

AC_PROG_SED
AC_PROG_MKDIR_P

AX_PTHREAD
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

        mpilibs:                ${MPILIBS}
])
