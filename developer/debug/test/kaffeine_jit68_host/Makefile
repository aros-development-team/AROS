# Copyright (C) 2026, The AROS Development Team. All rights reserved.
#
# Host build and checks of the kaffeine-jit68 engine
# (workbench/libs/kaffeine-jit68), for an AArch64 host (macOS or Linux): the
# translator emits AArch64 code and the checks run it. README.md says what
# the checks are and what a run of this Makefile gave.

CC     ?= cc
BUILD  ?= build
ENGINE ?= ../../../../workbench/libs/kaffeine-jit68

COMMON  = -std=c99 -Wall -Wextra -Wno-unused-parameter -Werror -g
DEFS    = -DKJ68_TEST_SEAMS -I include -I $(ENGINE)
SAN     = -fsanitize=address,undefined -fno-sanitize-recover=undefined
ASANFL  = $(COMMON) -O1 -fno-omit-frame-pointer $(SAN) $(DEFS)
O2FL    = $(COMMON) -O2 $(DEFS)

HEADERS = $(ENGINE)/kaffeine-jit68-abi-v1.h $(ENGINE)/kj68_intern.h include/exec/types.h
NAMES   = kj68_engine kj68_translate kj68_host_tests

vpath %.c $(ENGINE) .

.PHONY: all check clean

all: $(BUILD)/asan/kj68_host_tests $(BUILD)/o2/kj68_host_tests

$(BUILD)/asan/%.o: %.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(ASANFL) -c $< -o $@

$(BUILD)/o2/%.o: %.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(O2FL) -c $< -o $@

$(BUILD)/asan/kj68_host_tests: $(NAMES:%=$(BUILD)/asan/%.o)
	$(CC) $(SAN) -o $@ $^

$(BUILD)/o2/kj68_host_tests: $(NAMES:%=$(BUILD)/o2/%.o)
	$(CC) -o $@ $^

check: all
	ASAN_OPTIONS=halt_on_error=1:abort_on_error=0:detect_leaks=0 \
	UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
	$(BUILD)/asan/kj68_host_tests
	$(BUILD)/o2/kj68_host_tests

clean:
	rm -rf $(BUILD)
