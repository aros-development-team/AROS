#
# i386/boot/Makefile
#
#
#

HPATH=../include
CC=gcc -I$(HPATH)
CPP=$(CC) -E
AS86=as86 -0 -a 
LD86=ld86 -0
LD=ld -Ttext 0x1000 -e startup_32
BUILD=tools/build

disk: image
	dd bs=8192 if=image of=/dev/fd0
	rm -f image

image: bootsect setup bin tools/build
	tools/build bootsect setup bin >image
	rm -f bootsect setup bin

tools/build: tools/build.c
	$(CC) -D__BFD__ -O2 $< -o $@

bin: head.o init.o
	$(LD) -o $@ head.o init.o
	objcopy -O binary $@
	rm -f head.o init.o

init.o: init.c
	$(CC) -c init.c

head.o: head.S
	$(CC) -traditional -c head.S

setup: setup.o
	$(LD86) -s -o $@ $<
	rm -f setup.o

setup.o: setup.s
	$(AS86) -o $@ $<
	rm -f setup.s

setup.s: setup.S ../include/aros/config.h Makefile
	$(CPP) -traditional $< -o $@

bootsect: bootsect.o
	$(LD86) -s -o $@ $<
	rm -f bootsect.o

bootsect.o: bootsect.s
	$(AS86) -o $@ $<
	rm -f bootsect.s

bootsect.s: bootsect.S ../include/aros/config.h Makefile
	$(CPP) -traditional $< -o $@

clean:
	rm -f bootsect bootsect.o bootsect.s
	rm -f setup setup.o setup.s
	rm -f head.o
	rm -f bin image
	rm -f tools/build
 