EXTRA_DIST = autogen.sh
CLEANFILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\" \
	${EIGEN_INCLUDE} \
	-I${top_srcdir}/src/efyj \
	-I${top_srcdir}/src

AM_CFLAGS = ${my_CFLAGS} \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections

AM_LDFLAGS = \
	-Wl,--gc-sections \
	-Wl,--as-needed

SED_PROCESS = \
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(SED) \
	-e 's,@VERSION\@,$(VERSION),g' \
	-e 's,@prefix\@,$(prefix),g' \
	-e 's,@exec_prefix\@,$(exec_prefix),g' \
	-e 's,@libdir\@,$(libdir),g' \
	-e 's,@includedir\@,$(includedir),g' \
	< $< > $@ || rm $@

%.pc: %.pc.in Makefile
	$(SED_PROCESS)

LIBEFYJ_CURRENT=2
LIBEFYJ_REVISION=0
LIBEFYJ_AGE=2

pkginclude_HEADERS = src/efyj/types.hpp src/efyj/solver.hpp	\
		     src/efyj/problem.hpp src/efyj/options.hpp	\
		     src/efyj/model.hpp src/efyj/exception.hpp	\
		     src/efyj/efyj.hpp src/efyj/cstream.hpp	\
		     src/efyj/context.hpp

lib_LTLIBRARIES = src/libefyj.la

src_libefyj_la_SOURCES = src/context.cpp src/cstream.cpp		\
			 src/exception.cpp src/model.cpp		\
			 src/options.cpp src/post.hpp			\
			 src/prediction.hpp src/prediction.cpp		\
			 src/prediction-thread.cpp src/problem.cpp	\
			 src/solver.cpp src/solver-stack.hpp		\
			 src/utils.cpp src/utils.hpp

if VISIBILITY
src_libefyj_la_CPPFLAGS = $(AM_CPPFLAGS) -DEFYJ_DLL -Dlibefyj_EXPORTS \
    -fvisibility=hidden -fvisibility-inlines-hidden
endif

src_libefyj_la_LDFLAGS = $(AM_LDFLAGS) $(EXPAT_LDFLAGS) \
			 -version-info $(LIBEFYJ_CURRENT):$(LIBEFYJ_REVISION):$(LIBEFYJ_AGE)

src_libefyj_la_LIBADD = $(EXPAT_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = src/libefyj.pc
EXTRA_DIST += src/libefyj.pc.in
CLEANFILES += src/libefyj.pc

bin_PROGRAMS = efyj
efyj_SOURCES = src/main.cpp
efyj_LDFLAGS = $(AM_LDFLAGS) $(EXPAT_LDFLAGS) $(PTHREAD_CFLAGS)
efyj_LDADD = src/libefyj.la $(EXPAT_LIBS) $(PTHREAD_LIBS)

if HAVE_CATCH
TESTS = tests/test_basic
check_PROGRAMS = tests/test_basic
tests_test_basic_SOURCES = tests/test_basic.cpp
tests_test_basic_LDFLAGS = $(AM_LDFLAGS) $(EXPAT_LDFLAGS) $(PTHREAD_CFLAGS)
tests_test_basic_LDADD = src/libefyj.la $(EXPAT_LIBS) $(PTHREAD_LIBS)
endif
