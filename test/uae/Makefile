
CXX = g++
CXXFLAGS = -Wall -g -DDEBUG

all :
	$(CXX) $(CXXFLAGS) -S test.c -o test.S
	$(CXX) $(CXXFLAGS) -c test.c -o test.o
	$(CXX) $(CXXFLAGS) -O6 -S test.c -o test6.S
	$(CXX) $(CXXFLAGS) -O6 -c test.c -o test6.o
	$(CXX) $(CXXFLAGS) -O0 test.c -o t
	./t

clean :
	-rm t test.S test.o test6.S test6.o
