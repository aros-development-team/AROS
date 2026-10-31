# Copyright (C) 2026, The AROS Development Team. All rights reserved.
TEST_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PYTHON ?= python3
CXX ?= c++
TEST_ARGS ?=
BUILD_DIR ?= $(TEST_DIR)/build
NATIVE_CC ?= aarch64-aros-gcc
NATIVE_CPPFLAGS ?=
NATIVE_LDFLAGS ?=

all: test

test:
	$(PYTHON) "$(TEST_DIR)/run_tests.py" --cxx "$(CXX)" $(TEST_ARGS)

test-sanitize:
	$(PYTHON) "$(TEST_DIR)/run_tests.py" --cxx "$(CXX)" --sanitize $(TEST_ARGS)

native: $(BUILD_DIR)/fbgfx_palette

$(BUILD_DIR)/fbgfx_palette: $(TEST_DIR)/fbgfx_palette.c
	mkdir -p "$(BUILD_DIR)"
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) -O2 -Wall -o $@ $< $(NATIVE_LDFLAGS)

clean:
	rm -f "$(BUILD_DIR)/fbgfx_palette"

.PHONY: all test test-sanitize native clean
