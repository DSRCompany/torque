include $(top_srcdir)/buildutils/config.mk

PROG_ROOT = ../..

AM_CFLAGS = -g -DTEST_FUNCTION -I${PROG_ROOT}/ -I${PROG_ROOT}/${PBS_MACH} --coverage -DPBS_SERVER_HOME=\"$(PBS_SERVER_HOME)\"

lib_LTLIBRARIES = libtorquecfg.la

AM_LDFLAGS = @CHECK_LIBS@ ${lib_LTLIBRARIES}

check_PROGRAMS = test_torquecfg

libtorquecfg_la_SOURCES = scaffolding.c ${PROG_ROOT}/torquecfg.c
libtorquecfg_la_LDFLAGS = @CHECK_LIBS@ -shared -lgcov

test_torquecfg_SOURCES = test_torquecfg.c

check_SCRIPTS = ${PROG_ROOT}/../../test/coverage_run.sh

TESTS = ${check_PROGRAMS} ${check_SCRIPTS} 

CLEANFILES = *.gcno *.gcda *.gcov core *.lo
