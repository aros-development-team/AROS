MAKE = make
CC = vc +aos68k -c99
AS := vasmm68k_mot
RM := rm -f
CP := cp -p --remove-destination

CFLAGS := -O2 -cpu=68020 -I../include -I../include/amigaos -Iinclude \
	-DSTDC -DAMIGA -DDYNAMIC_CRC_TABLE
ASFLAGS := -quiet -Fhunk -m68020
LINK := -nostdlib
LIBS := -L../lib -lsupport

TARGET := z.library
VERSION := 1
OBJS := stub_m68k.o init.o malloc.o zutil.o deflate.o inflate.o inftrees.o \
	inffast.o compress.o uncompr.o trees.o crc32.o adler32.o
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
