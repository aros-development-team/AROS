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
