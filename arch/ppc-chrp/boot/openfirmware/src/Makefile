# $Id: Makefile 51 2008-03-20 15:51:17Z michalsc $

FILES 		:= bootstrap of1275 support menu elf gpr
TARGETDIR	:= .obj

all:	depend ../bin/bootloader

../bin/bootloader: $(foreach f, $(FILES), $(TARGETDIR)/$(f).o)
		@if [ ! -d ../bin ]; then mkdir ../bin; fi
		@echo "Linking kernel"
		@$(LD) -N -e bootstrap -T ldscript.lds -o $@ $(foreach f, $(FILES), $(TARGETDIR)/$(f).o)


#		@$(STRIP) -R.note -R.comment --strip-unneeded "$@"

$(TARGETDIR)/%.o: %.c
		@if [ ! -d "$(TARGETDIR)" ]; then mkdir .obj; fi
		@echo -n "Compiling $< "
		@if $(CC) $(CFLAGS) -DVERSION=\"`svn info --xml |grep revision=\" | head -n 1 | sed -e 's|[^"]*"||' -e 's|"[^"]*||'`\" $(INCLUDES) -c $< -o $@; then echo "done"; else echo "failed"; exit 1; fi

$(TARGETDIR)/%.d: %.c
		@if [ ! -d $(TARGETDIR) ]; then mkdir $(TARGETDIR); fi
		@echo "Make depend $<"
		@$(CC) $(CFLAGS) -DVERSION=\"`svn info --xml |grep revision=\" | head -n 1 | sed -e 's|[^"]*"||' -e 's|"[^"]*||'`\" $(INCLUDES) -MM $< > $@.$$$$; \
		sed 's#^.*\.o:#$(TARGETDIR)/&#' < $@.$$$$ | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@; rm -f $@.$$$$

clean:
		-rm -rf $(TARGETDIR)

depend:	$(foreach f, $(FILES), $(TARGETDIR)/$(f).d)

-include $(foreach f, $(FILES), $(TARGETDIR)/$(f).d)
