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

error.o: /usr/include/stdio.h /usr/include/sys/cdefs.h
error.o: /usr/include/machine/ansi.h /usr/include/errno.h
error.o: /usr/include/string.h ../toollib/error.h ../toollib/toollib.h
error.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
error.o: /usr/include/machine/types.h /usr/include/machine/endian.h
error.o: /usr/include/stdarg.h /usr/include/stdlib.h
filesup.o: /usr/include/stdlib.h /usr/include/sys/cdefs.h
filesup.o: /usr/include/machine/ansi.h /usr/include/stdio.h
filesup.o: /usr/include/sys/stat.h /usr/include/sys/time.h
filesup.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
filesup.o: /usr/include/machine/types.h /usr/include/machine/endian.h
filesup.o: /usr/include/time.h /usr/include/sys/_posix.h ../toollib/toollib.h
filesup.o: /usr/include/stdarg.h ../toollib/filesup.h
hash.o: /usr/include/string.h /usr/include/machine/ansi.h
hash.o: /usr/include/sys/cdefs.h /usr/include/stdlib.h /usr/include/ctype.h
hash.o: /usr/include/runetype.h ../toollib/hash.h ../toollib/toollib.h
hash.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
hash.o: /usr/include/machine/types.h /usr/include/machine/endian.h
hash.o: /usr/include/stdarg.h ../toollib/callback.h
lineparser.o: /usr/include/stdlib.h /usr/include/sys/cdefs.h
lineparser.o: /usr/include/machine/ansi.h /usr/include/stdio.h
lineparser.o: /usr/include/string.h /usr/include/ctype.h
lineparser.o: /usr/include/runetype.h ../toollib/lineparser.h
lineparser.o: ../toollib/toollib.h /usr/include/sys/types.h
lineparser.o: /usr/include/sys/inttypes.h /usr/include/machine/types.h
lineparser.o: /usr/include/machine/endian.h /usr/include/stdarg.h
mystream.o: /usr/include/string.h /usr/include/machine/ansi.h
mystream.o: /usr/include/sys/cdefs.h ../toollib/mystream.h
mystream.o: /usr/include/stdio.h ../toollib/toollib.h
mystream.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
mystream.o: /usr/include/machine/types.h /usr/include/machine/endian.h
mystream.o: /usr/include/stdarg.h /usr/include/stdlib.h ../toollib/callback.h
mystream.o: ../toollib/error.h
stdiocb.o: /usr/include/string.h /usr/include/machine/ansi.h
stdiocb.o: /usr/include/sys/cdefs.h /usr/include/errno.h ../toollib/error.h
stdiocb.o: ../toollib/toollib.h /usr/include/sys/types.h
stdiocb.o: /usr/include/sys/inttypes.h /usr/include/machine/types.h
stdiocb.o: /usr/include/machine/endian.h /usr/include/stdarg.h
stdiocb.o: /usr/include/stdlib.h ../toollib/stdiocb.h /usr/include/stdio.h
stdiocb.o: ../toollib/mystream.h ../toollib/callback.h
stringcb.o: /usr/include/errno.h /usr/include/sys/cdefs.h ../toollib/error.h
stringcb.o: ../toollib/toollib.h /usr/include/sys/types.h
stringcb.o: /usr/include/sys/inttypes.h /usr/include/machine/ansi.h
stringcb.o: /usr/include/machine/types.h /usr/include/machine/endian.h
stringcb.o: /usr/include/stdarg.h /usr/include/stdlib.h ../toollib/stringcb.h
stringcb.o: ../toollib/mystream.h /usr/include/stdio.h ../toollib/callback.h
stringcb.o: ../toollib/vstring.h
toollib.o: /usr/include/stdio.h /usr/include/sys/cdefs.h
toollib.o: /usr/include/machine/ansi.h /usr/include/stdlib.h
toollib.o: /usr/include/string.h /usr/include/ctype.h /usr/include/runetype.h
toollib.o: /usr/include/assert.h ../toollib/toollib.h
toollib.o: /usr/include/sys/types.h /usr/include/sys/inttypes.h
toollib.o: /usr/include/machine/types.h /usr/include/machine/endian.h
toollib.o: /usr/include/stdarg.h
vstring.o: /usr/include/string.h /usr/include/machine/ansi.h
vstring.o: /usr/include/sys/cdefs.h /usr/include/ctype.h
vstring.o: /usr/include/runetype.h ../toollib/vstring.h
