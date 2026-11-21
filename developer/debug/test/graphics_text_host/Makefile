# Copyright (C) 2026, The AROS Development Team. All rights reserved.
TEST_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
AROS_ROOT ?= $(abspath $(TEST_DIR)/../../../..)
BUILD_DIR ?= $(TEST_DIR)/build
PYTHON ?= python3
CC ?= cc

all: test

test:
	$(PYTHON) "$(TEST_DIR)/run_tests.py" --source-root "$(AROS_ROOT)" --build-dir "$(BUILD_DIR)" --cc "$(CC)"

test-sanitize:
	$(PYTHON) "$(TEST_DIR)/run_tests.py" --source-root "$(AROS_ROOT)" --build-dir "$(BUILD_DIR)/sanitize" --cc "$(CC)" --sanitize

clean:
	rm -f "$(BUILD_DIR)/test_textextent" "$(BUILD_DIR)/production_textextent.inc" "$(BUILD_DIR)/production_constants.h"
	rm -f "$(BUILD_DIR)/sanitize/test_textextent" "$(BUILD_DIR)/sanitize/production_textextent.inc" "$(BUILD_DIR)/sanitize/production_constants.h"
	rm -rf "$(BUILD_DIR)/sanitize" "$(BUILD_DIR)"

.PHONY: all test test-sanitize clean
