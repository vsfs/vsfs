AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = vsfs
DIST_SUBDIRS = lib/vobla $(SUBDIRS)

EXTRA_DIST = \
  LICENSE \
  README.md \
  bootstrap

coverage:
	lcov -c -d . -o raw_coverage.info
	lcov -e raw_coverage.info -b . '*/vsfs/vsfs/*' -o coverage.info
	$(RM) -rf coverage
	genhtml coverage.info -o coverage
#$(RM) raw_coverage.info coverage.info

release:
	make  # Generate all thrift files
	make dist
	cp vsfs-*.gz ~/rpmbuild/SOURCES
	rpmbuild -ba contrib/vsfs.spec

api:
	doxygen

test: check integration-test

integration-test:
	$(MAKE) -C vsfs $@

SYSTEM_TESTS = \
	       contrib/mvd/testing/mvd_tests.py

system-test:
	for t in ${SYSTEM_TESTS}; do ./$$t; done

.PHONY: check coverage release test integration-test
