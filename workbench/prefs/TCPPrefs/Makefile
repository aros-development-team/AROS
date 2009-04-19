# Project: tcpprefs
# Compiler: gcc g++
# Compiler Type: cross i386 AROS
# Makefile created by wxDev-C++ 6.10.2 on 13.03.08 15:27
# adapted by weissms

CPP       = /usr/local/bin/i386-aros-g++
CC        = /usr/local/bin/i386-aros-gcc
OBJ       = main.o args.o misc.o fpeditor.o interface.o
LINKOBJ   = main.o args.o misc.o fpeditor.o interface.o
LIBS      =
INCS      =
CXXINCS   = $(INCS)
BIN       = tcpprefs
DEFINES   = 
CXXFLAGS  = $(CXXINCS) $(DEFINES)
CFLAGS    = $(INCS) $(DEFINES)
RM        = rm -f
LINK      = /usr/local/bin/i386-aros-g++ -s
CP        = cp

.PHONY: all all-before all-after clean clean-custom
all: all-before $(BIN) all-after

all-after:
	$(CP) $(BIN) /home/weissms/aros/debug/bin/linux-i386/AROS

clean: clean-custom
	$(RM) $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(LINK) $(LINKOBJ) -o $(BIN) $(LIBS)

main.o: main.c
	$(CC) -c main.c -o main.o $(CFLAGS)

misc.o: misc.c
	$(CC) -c misc.c -o misc.o $(CFLAGS)

args.o: args.c
	$(CC) -c args.c -o args.o $(CFLAGS)

fpeditor.o: fpeditor.c
	$(CC) -c fpeditor.c -o fpeditor.o $(CFLAGS)

interface.o: interface.cpp
	$(CPP) -c interface.cpp -o interface.o $(CXXFLAGS)
