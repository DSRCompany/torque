PROG_ROOT = ../..
SRC_ROOT = ${PROG_ROOT}/../..

AM_CFLAGS = -g -DTEST_FUNCTION -I${SRC_ROOT}/test/mock_lib/ -I${PROG_ROOT}/ -I${PROG_ROOT}/${PBS_MACH} --coverage
AM_CXXFLAGS = ${AM_CFLAGS}

lib_LTLIBRARIES = libcsv.la 

AM_LDFLAGS = @CHECK_LIBS@ ${lib_LTLIBRARIES} -L/usr/src/gtest/ -lgtest -lgmock

check_PROGRAMS = test_csv

libcsv_la_SOURCES = ${PROG_ROOT}/csv.c
libcsv_la_LDFLAGS = @CHECK_LIBS@ -shared -lgcov

test_csv_SOURCES = test_csv.cpp

check_SCRIPTS = ${SRC_ROOT}/test/coverage_run.sh

TESTS = ${check_PROGRAMS} ${check_SCRIPTS}

CLEANFILES = *.gcno *.gcda *.gcov core *.lo
