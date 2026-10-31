TEST_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PYTHON ?= python3
CC ?= cc
SIZEBAR_ARGS ?=

test:
	@$(PYTHON) -I "$(TEST_DIR)/run_sizebar.py" --cc "$(CC)" $(SIZEBAR_ARGS)

# The runner works in a system temp directory it removes afterwards, so
# there is nothing in this directory to delete; the target exists so
# `make -C developer/debug/test/pci_host clean` works from the root.
clean:
	@:

.PHONY: test clean
