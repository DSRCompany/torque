include $(top_srcdir)/buildutils/config.mk

CHECK_DIR = test
GCHECK_DIR = gtest
check_SCRIPTS = ../../test/merge_cov.sh

DIST_SUBDIRS =

# all compilation happens in lib/Libpbs

if HAVE_CHECK
check:
	$(MAKE) -C $(CHECK_DIR) $(MAKECMDGOALS)
	$(MAKE) -C $(GCHECK_DIR) $(MAKECMDGOALS)
	${check_SCRIPTS}

.PHONY: cleancheck
cleancheck:
	cd test && $(MAKE) cleancheck
	cd gtest && $(MAKE) cleancheck
endif
