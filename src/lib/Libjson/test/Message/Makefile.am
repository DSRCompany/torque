PROG_ROOT = ../..

AM_CFLAGS = -g -DTEST_FUNCTION -I${PROG_ROOT}/ -I${PROG_ROOT}/${PBS_MACH} --coverage

lib_LTLIBRARIES = libMessage.la

AM_LDFLAGS = @CHECK_LIBS@ ${lib_LTLIBRARIES} ${TRANSPORTLIBS}

check_PROGRAMS = test_Message

libMessage_la_SOURCES = scaffolding.c ${PROG_ROOT}/Message.cpp
libMessage_la_LDFLAGS = @CHECK_LIBS@ -shared -lgcov

test_Message_SOURCES = test_Message.cpp

check_SCRIPTS = ${PROG_ROOT}/../../test/coverage_run.sh

TESTS = ${check_PROGRAMS} ${check_SCRIPTS}

CLEANFILES = *.gcno *.gcda *.gcov core *.lo
