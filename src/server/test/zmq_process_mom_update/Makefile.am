PROG_ROOT = ../..

AM_CFLAGS = -g -DTEST_FUNCTION -I${PROG_ROOT}/ -I${PROG_ROOT}/${PBS_MACH} --coverage

lib_LTLIBRARIES = libzmq_process_mom_update.la

AM_LDFLAGS = @CHECK_LIBS@ ${lib_LTLIBRARIES} ${TRANSPORTLIBS}

check_PROGRAMS = test_zmq_process_mom_update

libzmq_process_mom_update_la_SOURCES = scaffolding.c ${PROG_ROOT}/zmq_process_mom_update.c
libzmq_process_mom_update_la_LDFLAGS = @CHECK_LIBS@ -shared -lgcov

test_zmq_process_mom_update_SOURCES = test_zmq_process_mom_update.c

check_SCRIPTS = ${PROG_ROOT}/../test/coverage_run.sh

TESTS = ${check_PROGRAMS} ${check_SCRIPTS}

CLEANFILES = *.gcno *.gcda *.gcov core *.lo
