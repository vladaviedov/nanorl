CXX=g++
CXX_FLAGS=-Wall -Wextra -g -std=c++14 -I$(BUILD)/include
LDFLAGS=-lpthread -lgtest -lgtest_main

COV_DIR=cov

SRCS=$(shell cd $(PWD)/test && find * -type f -name '*.cpp')
TESTS=$(SRCS:.cpp=)
TARGETS=$(addprefix $(BUILD)/test/, $(TESTS))

RUN_TASKS=$(addprefix run_, $(TESTS))
OBJS=$(shell find $(BUILD)/objd -type f)

FFF=$(BUILD)/include/fff.h

.PHONY: test
test: $(BUILD)/test $(FFF) $(TARGETS) $(RUN_TASKS)

define compile_test
$(BUILD)/test/$(1): $(PWD)/test/$(1).cpp $(BUILD)/test
	$(CXX) $(CXX_FLAGS) -o $$@ $$< $(LDFLAGS)
endef

define run_test
.PHONY: run_$(1)
run_$(1): $(BUILD)/test/$(1)
	$(BUILD)/test/$(1)
endef

$(BUILD)/test:
	mkdir -p $@

$(FFF): $(PWD)/lib/fff/fff.h
	cp $< $@

$(foreach test, $(TESTS), $(eval $(call compile_test,$(test))))
$(foreach test, $(TESTS), $(eval $(call run_test,$(test))))
