include $(top_srcdir)/buildutils/config.mk

PROG_ROOT = ../..

AM_CFLAGS = -g -DTEST_FUNCTION -I${PROG_ROOT}/ -I${PROG_ROOT}/${PBS_MACH} --coverage

lib_LTLIBRARIES = libMomStatusMessage.la

AM_LDFLAGS = @CHECK_LIBS@ ${lib_LTLIBRARIES} ${TRANSPORTLIBS}

check_PROGRAMS = test_MomStatusMessage

libMomStatusMessage_la_SOURCES = scaffolding.c ${PROG_ROOT}/MomStatusMessage.cpp ${PROG_ROOT}/Message.cpp
libMomStatusMessage_la_LDFLAGS = @CHECK_LIBS@ -shared -lgcov

test_MomStatusMessage_SOURCES = test_MomStatusMessage.cpp

check_SCRIPTS = ${PROG_ROOT}/../../test/coverage_run.sh

TESTS = ${check_PROGRAMS} ${check_SCRIPTS}

CLEANFILES = *.gcno *.gcda *.gcov core *.lo
