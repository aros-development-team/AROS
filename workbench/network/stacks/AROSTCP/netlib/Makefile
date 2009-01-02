# $Id: Makefile,v 1.13 2006/12/15 08:45:26 sonic_amiga Exp $
#
# Makefile for AmiTCP/IP network support library 
#
# Copyright © 2005 - 2006 Pavel Fedin
#
# Based on Smakefile which is
# Copyright © 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
#		   Helsinki University of Technology, Finland.
#		   All rights reserved. 
#
# Created      : Tue Oct 11 13:11:57 2005 sonic
# Last modified: Wed Oct 12 21:28:57 2005 sonic
#
# $Log: Makefile,v $
# Revision 1.13  2006/12/15 08:45:26  sonic_amiga
# - miami.library moved into the kernel
# - usergroup.library reintegrated into the main package and heavily refactored
# - updated autodocs
#
# Revision 1.9  2005/11/29 06:28:06  sonic_amiga
# - EZTCP_FindKernVar() function declared private
# - kvm functions moved from netlib to separate private library
# - asprintf() and err() family now work
# - added SIOCGIFMTU and SIOCGIFINDEX definitions
#
# Revision 1.8  2005/11/28 09:40:17  sonic_amiga
# Added some more functions
#
# Revision 1.7  2005/11/28 06:26:15  sonic_amiga
# KVM functions now work
#
# Revision 1.6  2005/11/23 08:07:22  sonic_amiga
# Replaced back kill() removed by mistake
#
# Revision 1.5  2005/11/22 12:35:17  sonic_amiga
# Removed unfinished and unused functions
#
# Revision 1.4  2005/11/14 06:15:09  sonic_amiga
# - Added some more functions
# - kill.c fixed, now compiles
#
# Revision 1.3  2005/11/10 14:20:55  sonic_amiga
# Added ExecVEAsync() function
#
# Revision 1.2  2005/11/09 08:39:26  sonic_amiga
# Added kill() function
#
# Revision 1.1.1.1  2005/11/08 13:51:05  sonic_amiga
# Initial partial import upon request
#

SDK_INSTALL_DEST = /gg/lib

MAKE = make

RM= rm -f
RM_RECURSIVE= rm -r
MKDIR= mkdir
CP= cp

CC= gcc -noixemul
LD= ld
AR= ar
AR_FLAGS= rs

OPTIMIZATION-OPTION = -O2
BASEREL-OPTION =
DEBUG-OPTION = -g
VERBOSITY = -Wall -Wno-unused -Wno-uninitialized
OBJECT-ONLY = -c

CFLAGS= $(OPTIMIZATION-OPTION) $(BASEREL-OPTION) $(DEBUG-OPTION)\
	 $(VERBOSITY) $(OBJECT-ONLY) 

INCLUDE_DIRS= -I ../include

# We define SOCKETBASE_IN_CONTEXT for kernel usage.  Check <inline/socket.h>.
# same for USERGROUPBASE_IN_CONTEXT

DEFS= -DAMITCP=1 -DNETLIB=1 \
      -D__USE_SYSBASE=1 -DUSE_BUILTIN_MATH=1 -DAMITCP_NEW_NAMES\
	-DSOCKETBASE_IN_CONTEXT -DUSERGROUPBASE_IN_CONTEXT
 
#SCOPTIONS= parm=BOTH Nostackcheck \
#	Stringmerge Noerrorsource Nomultipleincludes Structureequivalence \
#	noversion noerrorhighlight IGNORE=224 IGNORE=92 \
#	optinl optinlocal opttime optcomp=5 optdep=5 optrdep=5 \
#	 $(IDIRS) $(DEFS) debug=line

#
# Note: sources with leading underscore "_" are specific with SAS C UFB 
# 
# As the standard autodoc extractor is broken (it allows only for 32 arguments)
# don't add sources with autodocs to the end of macro
#
#DOCSRC= autoinit.c gettimeofday.c herror.c \
#	lineread.c perror.c printfault.c rcmd.c strerror.c \
#	syslog.c timerinit.c dostat.c utime.c chmod.c _dup.c _dup2.c \
#	chown.c printuserfault.c init_usergroup.c \
#	popen.c sleep.c usleep.c \
#	init_inet_daemon.c set_socket_stdio.c serveraccept.c

#NETLIBSRC= strerror.c autoinit.c dummy.c lineread.c \
#        getopt.c printfault.c stubs.c perror.c herror.c \
#        timerinit.c gettimeofday.c rcmd.c syslog.c \
#        getpid.c chmod.c chown.c utime.c popen.c sleep.c usleep.c \
#	printuserfault.c init_usergroup.c setegid.c seteuid.c \
#	_dup.c _dup2.c stat.c _fstat.c fib.c dostat.c access.c \
#        _allocufb.c _chkufb.c _close.c _lseek.c _open.c _read.c \
#        _write.c fhopen.c ioctl.c iomode.c isatty.c \
#	init_inet_daemon.c set_socket_stdio.c serveraccept.c


NETLIBSRC= strerror.c autoinit.c dummy.c lineread.c \
        printfault.c stubs.c \
        herror.c syslog.c \
        getpid.c getppid.c chmod.c popen.c \
        printuserfault.c \
	init_inet_daemon.c serveraccept.c \
        linkaddr.c linkntoa.c \
        random.c kill.c writev.c \
	vasprintf.c asprintf.c err.c

SANA2LIBSRC= sana2perror.c sana2errlist.c sana2printfault.c

MIAMILIBSRC= miami_autoinit.c

#DOCS= netlib.doc sana2lib.doc

LIBS= libnet.a libsana2.a libmiami.a

NETOBJ= strerror.o autoinit.o dummy.o lineread.o \
        printfault.o stubs.o \
        herror.o syslog.o \
        getpid.o getppid.o chmod.o popen.o \
	printuserfault.o \
	init_inet_daemon.o serveraccept.o \
        linkaddr.o linkntoa.o \
        random.o kill.o writev.o \
	vasprintf.o asprintf.o err.o

#NETOBJ= strerror.o autoinit.o dummy.o lineread.o \
#        getopt.o printfault.o stubs.o perror.o herror.o \
#        timerinit.o gettimeofday.o rcmd.o syslog.o \
#        getpid.o chmod.o chown.o utime.o popen.o sleep.o usleep.o \
#	printuserfault.o init_usergroup.o setegid.o seteuid.o \
#	_dup.o _dup2.o stat.o _fstat.o fib.o dostat.o access.o \
#        _allocufb.o _chkufb.o _close.o _lseek.o _open.o _read.o \
#        _write.o fhopen.o ioctl.o iomode.o isatty.o \
#	init_inet_daemon.o set_socket_stdio.o serveraccept.o

SANA2OBJ=   sana2perror.o   sana2errlist.o   sana2printfault.o

MIAMIOBJ= miami_autoinit.o

.SUFFIXES: .c .o  
#
# Generic rules
#
.c.o:
	$(CC) $(CFLAGS) $(OFLAGS) $(DEFS) $(INCLUDE_DIRS) $*.c

normal: $(LIBS)

all: normal 

#
# Network libraries
#
libnet.a: $(NETOBJ)
	-$(RM) $@
	$(AR) $(AR_FLAGS) $@ $(NETOBJ) 

#
# Sana2.lib, convenience functions for Sana2 utilities
#
libsana2.a: $(SANA2OBJ)
	-$(RM) $@
	$(AR) $(AR_FLAGS) $@ $(SANA2OBJ)

#
# miami.lib, miami.library autoinit routine
#
libmiami.a: $(MIAMIOBJ)
	-$(RM) $@
	$(AR) $(AR_FLAGS) $@ $(MIAMIOBJ)

clean:
	-$(RM) *.a *.o

install:
	cp $(LIBS) $(SDK_INSTALL_DEST)

package:
	mkdir -p $(SDK_DEST)/Docs
	mkdir -p $(SDK_DEST)/lib
	cp netlib.doc $(SDK_DEST)/Docs
	cp $(LIBS) $(SDK_DEST)/lib
