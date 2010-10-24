# Makefile for Bootloader
# $Id: Makefile 43 2008-03-19 23:22:39Z michalsc $

export

TOP	:= $(shell pwd)

CC		:= powerpc-elf-gcc
CXX		:= powerpc-elf-g++
LD		:= powerpc-elf-ld
AR		:= powerpc-elf-ar
STRIP		:= powerpc-elf-strip
OBJCOPY		:= powerpc-elf-objcopy
MAKE		:= make --no-print-directory

CFLAGS	:= -Os -mcpu=603e -ffreestanding  -nostdlib -Wall 
INCLUDES:= -I"$(TOP)/include"

all:
		@$(MAKE) -C src all
		@cp bin/bootloader /tftpboot/os_image

clean:
		rm -rf bin
		@$(MAKE) -C src clean
