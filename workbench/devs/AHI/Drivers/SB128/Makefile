#  $Id: Makefile.in,v 1.16 2003/07/23 15:39:29 martin Exp $
#
#  Makefile.in for SB128 sound card driver
#



srcdir		= /usr/src/os4/ahi

DRIVER      = sb128.audio
MODEFILE	= SB128

OBJECTS		= driver-init.o sb128.o accel.o misc.o interrupt.o

include ../Common/Makefile.card


# CAMD driver

host_cpu	= powerpc
CAMDDIR		= CAMD

camd-clean:
	$(RM) camdstubs.c camdstubs.h CAMD/emu10kx CAMD/emu10kx.o CAMD.stamp

camd-bindist:	all
	$(INSTALL) --directory $(CAMDDIR)
	$(INSTALL_PROGRAM) CAMD/emu10kx $(CAMDDIR)
	$(STRIP) $(CAMDDIR)/emu10kx

CAMD/emu10kx:	CAMD/emu10kx.o camdstubs.o
	$(CC) $(LDFLAGS) $^ -o $@

CAMD.stamp:
	test -e CAMD || mkdir CAMD
	touch $@

CAMD/emu10kx.o:	CAMD.stamp CAMD/emu10kx.c camdstubs.h card-camd.h version.h
