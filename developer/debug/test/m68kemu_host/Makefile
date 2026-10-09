CXXFLAGS = -std=c++20 -Wall -Wextra -Wno-unused-parameter -g \
           -Istubs -I../../rom/m68kemu -I../../rom/m68kemu/Moira

TESTS = test_m68kemu test_moira_exec test_hello_thunks

all: $(TESTS)

test_m68kemu: test_m68kemu.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_moira_exec: test_moira_exec.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_hello_thunks: test_hello_thunks.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test: all
	./test_m68kemu
	./test_moira_exec
	./test_hello_thunks

clean:
	rm -f $(TESTS)

.PHONY: all test clean
