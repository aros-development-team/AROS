# Copyright (C) 2026, The AROS Development Team. All rights reserved.
#
# Host build and checks of the m68kjit engine
# (workbench/libs/m68kjit), for an AArch64 host (macOS or Linux): the
# translator emits AArch64 code and the checks run it. README.md says what
# the checks are and what a run of this Makefile gave.

CC     ?= cc
BUILD  ?= build
ENGINE ?= ../../../../workbench/libs/m68kjit

COMMON  = -std=c99 -Wall -Wextra -Wno-unused-parameter -Werror -g
M68KEMU ?= ../../../../rom/m68kemu
# the host can run the code the translator writes: an AArch64 Mac or Linux
DEFS    = -DM68KJIT_TEST_SEAMS -DM68KJIT_PLAT_RUNS=1 -I include -I $(ENGINE) -I $(M68KEMU)
SAN     = -fsanitize=address,undefined -fno-sanitize-recover=undefined
ASANFL  = $(COMMON) -O1 -fno-omit-frame-pointer $(SAN) $(DEFS)
O2FL    = $(COMMON) -O2 $(DEFS)

HEADERS = $(M68KEMU)/m68kemu_engine.h $(ENGINE)/m68kjit_intern.h $(ENGINE)/m68kjit_platform.h include/exec/types.h
NAMES   = m68kjit_engine m68kjit_translate m68kjit_platform_host m68kjit_host_tests

# The translator keeps a temporary in x18, which is an ordinary caller-saved
# register in the AROS ABI. Darwin does not keep x18 across a context
# switch: what translated code leaves there is lost when its thread is
# preempted. So on a Mac these checks build the translator from a copy with
# that one temporary in x23, a register the trampolines save and the
# translator does not use. On Linux they build the file as it is.
ifeq ($(shell uname -s),Darwin)
TRANSLATE = $(BUILD)/darwin/m68kjit_translate.c
$(TRANSLATE): $(ENGINE)/m68kjit_translate.c
	@mkdir -p $(@D)
	sed -e 's/^#define R_SRC     18 /#define R_SRC     23 /' $< > $@
	grep -q '^#define R_SRC     23 ' $@
else
TRANSLATE = $(ENGINE)/m68kjit_translate.c
endif

vpath %.c $(ENGINE) .

.PHONY: all check clean

all: $(BUILD)/asan/m68kjit_host_tests $(BUILD)/o2/m68kjit_host_tests

$(BUILD)/asan/m68kjit_translate.o: $(TRANSLATE) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(ASANFL) -c $< -o $@

$(BUILD)/o2/m68kjit_translate.o: $(TRANSLATE) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(O2FL) -c $< -o $@

$(BUILD)/asan/%.o: %.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(ASANFL) -c $< -o $@

$(BUILD)/o2/%.o: %.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(O2FL) -c $< -o $@

$(BUILD)/asan/m68kjit_host_tests: $(NAMES:%=$(BUILD)/asan/%.o)
	$(CC) $(SAN) -o $@ $^

$(BUILD)/o2/m68kjit_host_tests: $(NAMES:%=$(BUILD)/o2/%.o)
	$(CC) -o $@ $^

check: all
	ASAN_OPTIONS=halt_on_error=1:abort_on_error=0:detect_leaks=0 \
	UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
	$(BUILD)/asan/m68kjit_host_tests
	$(BUILD)/o2/m68kjit_host_tests

clean:
	rm -rf $(BUILD)
