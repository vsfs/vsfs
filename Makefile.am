AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = vsfs

coverage:
	lcov -c -d . -o raw_coverage.info
	lcov -e raw_coverage.info -b . '*/vsfs/vsfs/*' -o coverage.info
	$(RM) -rf coverage
	genhtml coverage.info -o coverage
#$(RM) raw_coverage.info coverage.info

.PHONY: coverage
