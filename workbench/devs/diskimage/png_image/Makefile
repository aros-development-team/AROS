MAKE := make
CC := vc +aos68k -c99
AS := vasmm68k_mot
RM := rm -f
CP := cp -p --remove-destination

CFLAGS := -O3 -cpu=68020 -maxoptpasses=20 -I../include -I../include/amigaos -Iinclude
ASFLAGS := -quiet -Fhunk -m68020
LINK := -final -nostdlib
LIBS := -L../lib -lsupport -lamiga -lvc

TARGET := png.image
VERSION := 1
OBJS := stub_m68k.o init.o class.o loadpng.o remap.o writergbapixels_p96.o writergbapixels_cgx.o

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
	$(CP) $(TARGET) $(RELEASEDIR)/Classes/Images/

clean:
	$(RM) $(TARGET) $(OBJS)

revision:
	bumprev $(VERSION) $(TARGET)
