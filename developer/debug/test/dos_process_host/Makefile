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
	rm -f "$(BUILD_DIR)/test_dosentry" "$(BUILD_DIR)/production_dosentry.inc" "$(BUILD_DIR)/production_constants.h"
	rm -f "$(BUILD_DIR)/sanitize/test_dosentry" "$(BUILD_DIR)/sanitize/production_dosentry.inc" "$(BUILD_DIR)/sanitize/production_constants.h"

.PHONY: all test test-sanitize clean
