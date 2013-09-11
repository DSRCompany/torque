<<<<<<< HEAD
 
PROG_ROOT = ../..

AM_CFLAGS = -g -DTEST_FUNCTION -I${PROG_ROOT}/ -I${PROG_ROOT}/${PBS_MACH} -I$(PROG_ROOT)/../include --coverage `xml2-config --cflags`
AM_LIBS=`xml2-config --libs`

lib_LTLIBRARIES = libtest_u_misc.la

AM_LDFLAGS = @CHECK_LIBS@ $(lib_LTLIBRARIES) $(AM_LIBS)

check_PROGRAMS = test_u_misc

libtest_u_misc_la_SOURCES = scaffolding.c $(PROG_ROOT)/u_misc.c
libtest_u_misc_la_LDFLAGS = @CHECK_LIBS@ $(AM_LIBS) -shared

test_u_misc_SOURCES = test_u_misc.c

check_SCRIPTS = ${PROG_ROOT}/../../test/coverage_run.sh
TESTS = $(check_PROGRAMS) 

CLEANFILES = *.gcno *.gcda *.gcov_core *.lo
=======
PROG_ROOT = ../..

AM_CFLAGS = -g -DTEST_FUNCTION -I${PROG_ROOT}/ -I${PROG_ROOT}/${PBS_MACH} --coverage

lib_LTLIBRARIES = libu_misc.la

AM_LDFLAGS = @CHECK_LIBS@ ${lib_LTLIBRARIES}

check_PROGRAMS = test_u_misc

libu_misc_la_SOURCES = scaffolding.c ${PROG_ROOT}/u_misc.c
libu_misc_la_LDFLAGS = @CHECK_LIBS@ -shared -lgcov

test_u_misc_SOURCES = test_u_misc.c



check_SCRIPTS = ${PROG_ROOT}/../../test/coverage_run.sh

TESTS = ${check_PROGRAMS} ${check_SCRIPTS}

CLEANFILES = *.gcno *.gcda *.gcov core *.lo
>>>>>>> Connect to nodes in random order.
