#include $(top_srcdir)/buildutils/config.mk

PROG_ROOT = ../..

AM_CFLAGS = -g -DTEST_FUNCTION -I${PROG_ROOT}/ -I${PROG_ROOT}/${PBS_MACH} --coverage
AM_CXXFLAGS = ${AM_CFLAGS}

lib_LTLIBRARIES = libmom_zstatus.la

AM_LDFLAGS = @CHECK_LIBS@ ${lib_LTLIBRARIES} ${TRANSPORTLIBS}

check_PROGRAMS = test_mom_zstatus

libmom_zstatus_la_SOURCES = scaffolding.c ${PROG_ROOT}/mom_zstatus.cpp $(PROG_ROOT)/../lib/Libutils/u_mom_hierarchy.c $(PROG_ROOT)/../lib/Libutils/u_resizable_array.c
libmom_zstatus_la_LDFLAGS = @CHECK_LIBS@ -shared -lgcov

test_mom_zstatus_SOURCES = test_mom_zstatus.cpp

check_SCRIPTS = ${PROG_ROOT}/../test/coverage_run.sh

TESTS = ${check_PROGRAMS} ${check_SCRIPTS}

CLEANFILES = *.gcno *.gcda *.gcov core *.lo
