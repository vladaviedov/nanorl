BUILD_DIRS=$(BUILD) \
		   $(BUILD)/lib \
		   $(OBJ_DIR)

SUBDIRS=$(shell cd $(PWD)/src && find * -type d)
MKSUBDIRS=$(addprefix $(OBJ_DIR)/, $(SUBDIRS))
SRCS=$(shell cd $(PWD)/src && find * -type f -name '*.c')
OBJS=$(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))

.PHONY: build
build: $(BUILD_DIRS) headers $(TARGET_STATIC) $(TARGET_SHARED)

# Templates
define make_build_dir
$(1):
	mkdir -p $$@
endef

define mk_subdir
$(OBJ_DIR)/$(1): $(BUILD)
	mkdir -p $$@
endef

define compile_subdir
$(OBJ_DIR)/$(1)%.o: $(PWD)/src/$(1)%.c $(MKSUBDIRS)
	$$(CC) $$(CFLAGS) -c -o $$@ $$<
endef

# Build directory rules
$(foreach build_dir, $(BUILD_DIRS), \
	$(eval $(call make_build_dir,$(build_dir))))

.PHONY: headers
headers: $(BUILD_DIRS)
	cp -R $(PWD)/include $(BUILD)/include

$(TARGET_SHARED): $(BUILD) $(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

$(TARGET_STATIC): $(BUILD) $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

# Build root
$(eval $(call compile_subdir,))

# Build subdirectories
$(foreach subdir, $(SUBDIRS), $(eval $(call mk_subdir,$(subdir))))
$(foreach subdir, $(SUBDIRS), $(eval $(call compile_subdir,$(subdir))))
