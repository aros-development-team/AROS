MAKE := make
CC := vc +aos68k -c99
AS := vasmm68k_mot
RM := rm -f
CP := cp -p --remove-destination

CFLAGS := -O2 -cpu=68020 -maxoptpasses=30 -I../include \
	-I../include/amigaos -Iinclude -DBZ_NO_STDIO
ASFLAGS := -quiet -Fhunk -m68020
LINK := -nostdlib
LIBS := -L../lib -lsupport

TARGET := bz2.library
VERSION := 1
OBJS := stub_m68k.o init.o malloc.o bzlib.o compress.o decompress.o huffman.o \
	blocksort.o crctable.o randtable.o
RELEASEDIR := ../../release/m68k-amigaos/diskimage_device

# -------------------------------------------------------------

all: $(TARGET)

$(TARGET): $(OBJS) ../lib/support.lib
	$(CC) $(LINK) -o $@ $(OBJS) $(LIBS)

init.o: $(TARGET)_rev.h

../lib/libsupport.a:
	$(MAKE) -C .. lib/support.lib

install: all
	$(CP) $(TARGET) LIBS:

dist: all
	$(CP) $(TARGET) $(RELEASEDIR)/Libs/

clean:
	$(RM) $(TARGET) $(OBJS)

revision:
	bumprev $(VERSION) $(TARGET)
