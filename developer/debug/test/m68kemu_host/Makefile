TEST_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
M68KEMU_DIR := $(abspath $(TEST_DIR)/../../../../rom/m68kemu)
PYTHON ?= python3
OPENLIBRARY_ARGS ?=
MEMHEADER_ARGS ?=
MEMORY_BOUNDS_ARGS ?=

CXXFLAGS = -std=c++20 -Wall -Wextra -Wno-unused-parameter -g \
           -I$(TEST_DIR)/stubs -I$(M68KEMU_DIR) -I$(M68KEMU_DIR)/Moira

TESTS = test_m68kemu test_moira_exec test_hello_thunks

all: $(TESTS)

test_m68kemu: $(TEST_DIR)/test_m68kemu.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_moira_exec: $(TEST_DIR)/test_moira_exec.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_hello_thunks: $(TEST_DIR)/test_hello_thunks.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test: all test-memory-bounds test-openlibrary-versions test-memheader
	./test_m68kemu
	./test_moira_exec
	./test_hello_thunks

test-memory-bounds:
	$(PYTHON) "$(TEST_DIR)/regressions/run_memory_bounds.py" --cxx "$(CXX)" $(MEMORY_BOUNDS_ARGS)

test-memory-bounds-sanitize:
	$(PYTHON) "$(TEST_DIR)/regressions/run_memory_bounds.py" --cxx "$(CXX)" --sanitize $(MEMORY_BOUNDS_ARGS)

test-openlibrary-versions:
	$(PYTHON) "$(TEST_DIR)/regressions/run_openlibrary_versions.py" --cxx "$(CXX)" $(OPENLIBRARY_ARGS)

test-openlibrary-versions-sanitize:
	$(PYTHON) "$(TEST_DIR)/regressions/run_openlibrary_versions.py" --cxx "$(CXX)" --sanitize $(OPENLIBRARY_ARGS)

test-memheader:
	$(PYTHON) "$(TEST_DIR)/regressions/run_memheader.py" --cc "$(CC)" $(MEMHEADER_ARGS)

test-memheader-sanitize:
	$(PYTHON) "$(TEST_DIR)/regressions/run_memheader.py" --cc "$(CC)" --sanitize $(MEMHEADER_ARGS)

clean:
	rm -f $(TESTS)

.PHONY: all test test-memheader test-memheader-sanitize test-openlibrary-versions test-openlibrary-versions-sanitize test-memory-bounds test-memory-bounds-sanitize clean

# The full host suite already requires this Moira checkout.
LIBRARY_LOADING_ARGS ?=
test: test-library-loading-versions

test-library-loading-versions:
	$(PYTHON) "$(TEST_DIR)/regressions/run_library_loading_versions.py" --cxx "$(CXX)" $(LIBRARY_LOADING_ARGS)

test-library-loading-versions-sanitize:
	$(PYTHON) "$(TEST_DIR)/regressions/run_library_loading_versions.py" --cxx "$(CXX)" --sanitize $(LIBRARY_LOADING_ARGS)

.PHONY: test-library-loading-versions test-library-loading-versions-sanitize

# Requested root-stack reservation: ordinary fixture in test, sanitizer optional.
REQUESTED_STACK_ARGS ?=
test: test-requested-stack

test-requested-stack:
	$(PYTHON) "$(TEST_DIR)/regressions/run_requested_stack.py" --cc "$(CC)" $(REQUESTED_STACK_ARGS)

test-requested-stack-sanitize:
	$(PYTHON) "$(TEST_DIR)/regressions/run_requested_stack.py" --cc "$(CC)" --sanitize $(REQUESTED_STACK_ARGS)

.PHONY: test-requested-stack test-requested-stack-sanitize

# Process stream borrowing and translation: ordinary fixture in test, sanitizer optional.
PROCESS_STREAMS_ARGS ?=
test: test-process-streams

test-process-streams:
	$(PYTHON) "$(TEST_DIR)/regressions/run_process_streams.py" --cc "$(CC)" $(PROCESS_STREAMS_ARGS)

test-process-streams-sanitize:
	$(PYTHON) "$(TEST_DIR)/regressions/run_process_streams.py" --cc "$(CC)" --sanitize $(PROCESS_STREAMS_ARGS)

.PHONY: test-process-streams test-process-streams-sanitize

# Initial Process arguments: ordinary fixture in test, sanitizer optional.
INITIAL_ARGUMENTS_ARGS ?=
test: test-initial-arguments

test-initial-arguments:
	$(PYTHON) "$(TEST_DIR)/regressions/run_initial_arguments.py" --cc "$(CC)" $(INITIAL_ARGUMENTS_ARGS)

test-initial-arguments-sanitize:
	$(PYTHON) "$(TEST_DIR)/regressions/run_initial_arguments.py" --cc "$(CC)" --sanitize $(INITIAL_ARGUMENTS_ARGS)

.PHONY: test-initial-arguments test-initial-arguments-sanitize

# Guest GetArgStr and SetArgStr: ordinary fixture in test, sanitizer optional.
ARGUMENT_APIS_ARGS ?=
test: test-argument-apis

test-argument-apis:
	$(PYTHON) "$(TEST_DIR)/regressions/run_argument_apis.py" --cc "$(CC)" $(ARGUMENT_APIS_ARGS)

test-argument-apis-sanitize:
	$(PYTHON) "$(TEST_DIR)/regressions/run_argument_apis.py" --cc "$(CC)" --sanitize $(ARGUMENT_APIS_ARGS)

.PHONY: test-argument-apis test-argument-apis-sanitize

# AllocBitMap result projection: ordinary fixture in test, sanitizer optional.
BITMAP_RESULT_ARGS ?=
test: test-bitmap-result

test-bitmap-result:
	$(PYTHON) "$(TEST_DIR)/regressions/run_bitmap_result.py" --cc "$(CC)" $(BITMAP_RESULT_ARGS)

test-bitmap-result-sanitize:
	$(PYTHON) "$(TEST_DIR)/regressions/run_bitmap_result.py" --cc "$(CC)" --sanitize $(BITMAP_RESULT_ARGS)

.PHONY: test-bitmap-result test-bitmap-result-sanitize
