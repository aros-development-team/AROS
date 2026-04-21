CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -g \
           -Istubs -I../../rom/m68kemu
test_m68kemu: test_m68kemu.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<
test: test_m68kemu
	./test_m68kemu
clean:
	rm -f test_m68kemu
.PHONY: test clean
