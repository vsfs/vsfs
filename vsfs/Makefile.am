SUBDIRS = rpc common query index masterd indexd client fuse testing
BUILT_SOURCES = rpc

rpc:
	make -C rpc

integration-test:
	$(MAKE) -C masterd $@
	$(MAKE) -C testing $@
	$(MAKE) -C fuse $@

.PHONY: rpc integratio-test
