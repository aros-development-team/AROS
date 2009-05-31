TARGET=///GBlanker/Libs/V37/Garshnelib.library

all: Garshnelib.library

Garshnelib.library: Garshnelib.o
	SLink WITH <<
LIBFD Garshnelib.fd
TO Garshnelib.library
FROM LIB:libent.o LIB:libinit.o Garshnelib.o
LIB LIB:sc.lib
NOICONS
SMALLDATA SMALLCODE
LIBID "Garshnelib.library 37.1 (23.10.94)"
LIBVERSION 37 LIBREVISION 2
<
      
Garshnelib.o: Garshnelib.c
	SC DEBUG=LINE Garshnelib.c

install: all
	SLink FROM Garshnelib.library TO $(TARGET) NODEBUG NOICONS

clean:
	Delete FORCE QUIET Garshnelib.library Garshnelib.o

revise:
