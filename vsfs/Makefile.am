SUBDIRS = rpc common query index masterd indexd client fuse
BUILT_SOURCES = rpc

rpc:
	make -C rpc

integration-test:
	$(MAKE) -C masterd $@

.PHONY: rpc integratio-test
