export PWD=$(shell pwd)
export BUILD=$(PWD)/build

# TODO: enable after first tag
# VERSION='"$(shell git describe --tags --dirty)"'
VERSION=v2-pre0.1

export CC=gcc
export CFLAGS=-std=c99 \
	-fPIC \
	-I$(PWD)/include \
	-I$(BUILD)/include \
	-DNRL_VERSION=$(VERSION)
CFLAGS_RELEASE=-O2 -w
CFLAGS_DEBUG=-Wall -Wextra -g
export LDFLAGS=

export AR=ar
export ARFLAGS=rvsc

export TARGET_STATIC=$(BUILD)/lib/nanorl.a
export TARGET_SHARED=$(BUILD)/lib/nanorl.so
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
	mkdir -p $(PREFIX)/bin $(PREFIX)/share/man/man1
	cp $(TARGET) $(PREFIX)/bin
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
	ENABLE='internal' $(DOXYGEN) $(DOXYGEN_CONF)
