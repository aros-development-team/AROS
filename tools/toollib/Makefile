#   Copyright (C) 1998-2000 AROS - The Amiga Research OS
#   $Id$
#
#   Desc: Make the tool library.
#
SRC	= error filesup hash lineparser mystream \
	  stdiocb stringcb toollib vstring

# Include the AROS build information. This will fail if $(TOP) is not
# set, and build in the current directory.
-include $(TOP)/config/make.cfg

# Values relating to stuff.
PURIFY	= /home/digulla/AROS/compiler/purify/purify

# If the include above doesn't work, we need some defaults.
HOST_CC	?= $(CC)
HOST_AR ?= $(AR) cru 
TOOLDIR ?= .
MKDIR ?= mkdir
MECHO ?= echo
TOOLLIB ?= $(TOOLDIR)/libtool.a
HOST_CFLAGS := $(HOST_CFLAGS) -Wall -g -DHAVE_VSNPRINTF -I.. -O

OBJDIR	= $(TOOLDIR)/obj

all : depend $(OBJDIR) $(TOOLLIB)

$(TOOLLIB) : $(foreach f,$(SRC),$(OBJDIR)/$(f).o)
	@$(MECHO) "Creating toollib/$(notdir $@)..."
	@$(HOST_AR) $@ $?

$(OBJDIR)/%.o : %.c
	@$(MECHO) "Compiling toollib/$(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) -o $@ -c $<

$(OBJDIR) : 
	@$(MKDIR) $(OBJDIR)

pure :
	-@rm *.o
	@$(MAKE) "CC=$(PURIFY) $(CC)"

clean :
	-@$(RM) -r $(OBJDIR) $(TOOLLIB)
	@sed '/^# DO NOT DELETE$$/,$$d' Makefile > Makefile.tmp
	@echo "# DO NOT DELETE" >> Makefile.tmp
	@mv -f Makefile.tmp Makefile
	-@$(RM) Makefile.bak

depend :
	@makedepend -- $(HOST_CFLAGS) -- *.c

# DO NOT DELETE

error.o: /usr/include/stdio.h /usr/include/features.h
error.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
error.o: /usr/include/stddef.h
error.o: /usr/lib/gcc-lib/i486-linux/egcs-2.91.66/include/stdarg.h
error.o: /usr/include/bits/types.h /usr/include/libio.h
error.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
error.o: /usr/include/errno.h /usr/include/bits/errno.h
error.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
error.o: /usr/include/string.h ../toollib/error.h ../toollib/toollib.h
error.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h
error.o: /usr/include/bits/endian.h /usr/include/sys/select.h
error.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
error.o: /usr/include/sys/sysmacros.h /usr/include/stdlib.h
error.o: /usr/include/alloca.h
filesup.o: /usr/include/stdlib.h /usr/include/features.h
filesup.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
filesup.o: /usr/include/stddef.h /usr/include/sys/types.h
filesup.o: /usr/include/bits/types.h /usr/include/time.h
filesup.o: /usr/include/endian.h /usr/include/bits/endian.h
filesup.o: /usr/include/sys/select.h /usr/include/bits/select.h
filesup.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
filesup.o: /usr/include/alloca.h /usr/include/stdio.h
filesup.o: /usr/lib/gcc-lib/i486-linux/egcs-2.91.66/include/stdarg.h
filesup.o: /usr/include/libio.h /usr/include/_G_config.h
filesup.o: /usr/include/bits/stdio_lim.h /usr/include/sys/stat.h
filesup.o: /usr/include/bits/stat.h ../toollib/toollib.h ../toollib/filesup.h
hash.o: /usr/include/string.h /usr/include/features.h
hash.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
hash.o: /usr/include/stddef.h /usr/include/stdlib.h /usr/include/sys/types.h
hash.o: /usr/include/bits/types.h /usr/include/time.h /usr/include/endian.h
hash.o: /usr/include/bits/endian.h /usr/include/sys/select.h
hash.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
hash.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
hash.o: /usr/include/ctype.h ../toollib/hash.h ../toollib/toollib.h
hash.o: /usr/lib/gcc-lib/i486-linux/egcs-2.91.66/include/stdarg.h
hash.o: ../toollib/callback.h
lineparser.o: /usr/include/stdlib.h /usr/include/features.h
lineparser.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
lineparser.o: /usr/include/stddef.h /usr/include/sys/types.h
lineparser.o: /usr/include/bits/types.h /usr/include/time.h
lineparser.o: /usr/include/endian.h /usr/include/bits/endian.h
lineparser.o: /usr/include/sys/select.h /usr/include/bits/select.h
lineparser.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
lineparser.o: /usr/include/alloca.h /usr/include/stdio.h
lineparser.o: /usr/lib/gcc-lib/i486-linux/egcs-2.91.66/include/stdarg.h
lineparser.o: /usr/include/libio.h /usr/include/_G_config.h
lineparser.o: /usr/include/bits/stdio_lim.h /usr/include/string.h
lineparser.o: /usr/include/ctype.h ../toollib/lineparser.h
lineparser.o: ../toollib/toollib.h
mystream.o: /usr/include/string.h /usr/include/features.h
mystream.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
mystream.o: /usr/include/stddef.h ../toollib/mystream.h /usr/include/stdio.h
mystream.o: /usr/lib/gcc-lib/i486-linux/egcs-2.91.66/include/stdarg.h
mystream.o: /usr/include/bits/types.h /usr/include/libio.h
mystream.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
mystream.o: ../toollib/toollib.h /usr/include/sys/types.h /usr/include/time.h
mystream.o: /usr/include/endian.h /usr/include/bits/endian.h
mystream.o: /usr/include/sys/select.h /usr/include/bits/select.h
mystream.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
mystream.o: /usr/include/stdlib.h /usr/include/alloca.h ../toollib/callback.h
mystream.o: ../toollib/error.h
stdiocb.o: /usr/include/string.h /usr/include/features.h
stdiocb.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
stdiocb.o: /usr/include/stddef.h /usr/include/errno.h
stdiocb.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
stdiocb.o: /usr/include/asm/errno.h ../toollib/error.h ../toollib/toollib.h
stdiocb.o: /usr/include/sys/types.h /usr/include/bits/types.h
stdiocb.o: /usr/include/time.h /usr/include/endian.h
stdiocb.o: /usr/include/bits/endian.h /usr/include/sys/select.h
stdiocb.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
stdiocb.o: /usr/include/sys/sysmacros.h
stdiocb.o: /usr/lib/gcc-lib/i486-linux/egcs-2.91.66/include/stdarg.h
stdiocb.o: /usr/include/stdlib.h /usr/include/alloca.h ../toollib/stdiocb.h
stdiocb.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
stdiocb.o: /usr/include/bits/stdio_lim.h ../toollib/mystream.h
stdiocb.o: ../toollib/callback.h
stringcb.o: /usr/include/errno.h /usr/include/features.h
stringcb.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
stringcb.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
stringcb.o: /usr/include/asm/errno.h ../toollib/error.h ../toollib/toollib.h
stringcb.o: /usr/include/sys/types.h /usr/include/bits/types.h
stringcb.o: /usr/include/stddef.h /usr/include/time.h /usr/include/endian.h
stringcb.o: /usr/include/bits/endian.h /usr/include/sys/select.h
stringcb.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
stringcb.o: /usr/include/sys/sysmacros.h
stringcb.o: /usr/lib/gcc-lib/i486-linux/egcs-2.91.66/include/stdarg.h
stringcb.o: /usr/include/stdlib.h /usr/include/alloca.h ../toollib/stringcb.h
stringcb.o: ../toollib/mystream.h /usr/include/stdio.h /usr/include/libio.h
stringcb.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
stringcb.o: ../toollib/callback.h ../toollib/vstring.h
toollib.o: /usr/include/stdio.h /usr/include/features.h
toollib.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
toollib.o: /usr/include/stddef.h
toollib.o: /usr/lib/gcc-lib/i486-linux/egcs-2.91.66/include/stdarg.h
toollib.o: /usr/include/bits/types.h /usr/include/libio.h
toollib.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
toollib.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
toollib.o: /usr/include/endian.h /usr/include/bits/endian.h
toollib.o: /usr/include/sys/select.h /usr/include/bits/select.h
toollib.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
toollib.o: /usr/include/alloca.h /usr/include/string.h /usr/include/ctype.h
toollib.o: /usr/include/assert.h ../toollib/toollib.h
vstring.o: /usr/include/string.h /usr/include/features.h
vstring.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
vstring.o: /usr/include/stddef.h /usr/include/ctype.h
vstring.o: /usr/include/bits/types.h /usr/include/endian.h
vstring.o: /usr/include/bits/endian.h ../toollib/vstring.h
