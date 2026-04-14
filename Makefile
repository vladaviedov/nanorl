export PWD=$(shell pwd)
export BUILD=$(PWD)/build
VERSION='"$(shell git describe --tags --dirty)"'

export CC=gcc
CFLAGS_CONFIG?=
export CFLAGS=-std=c99 \
	-fPIC \
	-I$(PWD)/include \
	-I$(BUILD)/include \
	-DNRL_VERSION=$(VERSION) \
	$(CFLAGS_CONFIG)
CFLAGS_RELEASE=-O2 -w
CFLAGS_DEBUG=-Wall -Wextra -g

export AR=ar
export ARFLAGS=rvsc

export TARGET_STATIC_IM=$(BUILD)/lib/libnanorl.part.a
export TARGET_STATIC=$(BUILD)/lib/libnanorl.a
export TARGET_SHARED=$(BUILD)/lib/libnanorl.so
export TARGET_EXAMPLE=$(BUILD)/bin/nrl_example
export TARGET_MAN=$(BUILD)/share/man/man3/nanorl.3.gz

export OBJ_DIR=
export TASK=

BUILD_MK=$(PWD)/build.mk
PREFIX?=/usr
MAN_SRC=

# Build tasks
.PHONY: release
release: TASK=release
release: CFLAGS+=$(CFLAGS_RELEASE)
release: OBJ_DIR=$(BUILD)/obj
release:
	$(MAKE) -f $(BUILD_MK)

.PHONY: debug
debug: TASK=debug
debug: CFLAGS+=$(CFLAGS_DEBUG)
debug: OBJ_DIR=$(BUILD)/objd
debug:
	$(MAKE) -f $(BUILD_MK)

.PHONY: install
install:
	mkdir -p $(PREFIX)/lib $(PREFIX)/include/nanorl
	# $(PREFIX)/share/man/man1
	install -Dm755 $(TARGET_SHARED) $(PREFIX)/lib
	find $(BUILD)/include/nanorl -type f -exec install -Dm644 {} $(PREFIX)/include/nanorl \;
	# gzip -c $(MAN_SRC) > $(PREFIX)/share/man/man1/mesh.1.gz

.PHONY: clean
clean:
	rm -rf $(BUILD)

# Formatting
FORMAT=clang-format
FORMAT_CHECK_FLAGS=--dry-run --Werror
FORMAT_FIX_FLAGS=-i

FORMAT_FILES=$(shell find src -type f) \
			 $(shell find include -type f)

.PHONY: checkformat
checkformat:
	$(FORMAT) $(FORMAT_CHECK_FLAGS) $(FORMAT_FILES)

.PHONY: format
format:
	$(FORMAT) $(FORMAT_FIX_FLAGS) $(FORMAT_FILES)

# Documentation
DOXYGEN=doxygen
DOXYGEN_CONF=Doxyfile

.PHONY: docs
docs:
	$(DOXYGEN) $(DOXYGEN_CONF)

.PHONY: fulldocs
fulldocs:
	SRC_ENABLE='src' $(DOXYGEN) $(DOXYGEN_CONF)
