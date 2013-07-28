AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = vsfs
DIST_SUBDIRS = lib/vobla $(SUBDIRS)

EXTRA_DIST = \
  LICENSE \
  README.md

coverage:
	lcov -c -d . -o raw_coverage.info
	lcov -e raw_coverage.info -b . '*/vsfs/vsfs/*' -o coverage.info
	$(RM) -rf coverage
	genhtml coverage.info -o coverage
#$(RM) raw_coverage.info coverage.info

release:
	rpmbuild -ba contrib/vsfs.spec

test: check integration-test

integration-test:
	$(MAKE) -C vsfs $@

.PHONY: check coverage release test integration-test
