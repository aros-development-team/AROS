#
# Makefile to build "build" file
#

CC=gcc -O2 -D__BFD__

$(BUILD): $(BUILD).c
	gcc -O2 -D__BFD__ $< -o $@

clean:
	rm -f build
