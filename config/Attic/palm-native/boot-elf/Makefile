## Makefile for LCD test
TOP = ../../..
include $(TOP)/config/make.cfg
TARGET_ARCH = 

TARGET = exec
APPNAME = "Exec test"
APPID = "STB3"

OBJS = $(TARGET).o palmgfx.o
LIBS = $(TOP)/bin/palm-m68k/AROS/lib/startup.o -L$(TOP)/bin/palm-m68k/AROS/lib -lexec -larossupport -larosc -L./ -lgcc1

CC = m68k-linux-gcc
AS = m68k-linux-as

INC = -I $(AROS_INCLUDES)

CFLAGS = -Wall -O0 $(INC) $(USRINC) -mc68000

OBJRES = m68k-linux-obj-res
NM = m68k-linux-nm

all: $(TARGET)

%.o : %.c
	$(CC) -c $^ $(INC) $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) -nostartfiles -Wl,-r $(LIBS)
	! $(NM) -u $(TARGET) | grep .

depend:
	makedepend -Y -I. *.c

clean:
	-rm -f *.o $(TARGET).elf

veryclean: clean
	-rm -f $(TARGET).elf

