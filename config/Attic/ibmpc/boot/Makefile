#
# i386/boot/Makefile
#
#
#

AS86=as86 -0 -a 
LD86=ld86 -0
CPP =$(CC) -E

$(HEADER): head.o init.o logo.o text.o
	@$(LINK) -r head.o init.o logo.o text.o -o $(HEADER)

$(BOOTSECT): bootsect.S $(IPATH)/aros/config.h Makefile
	@echo -n Compiling bootsector...
	@$(CPP) -traditional $< -o bootsect.s
	@$(AS86) -o bootsect.o bootsect.s
	@$(LD86) -s -o $(BOOTSECT) bootsect.o
	@rm -f bootsect.s bootsect.o
	@echo Done!

$(SETUP): setup.S $(IPATH)/aros/config.h Makefile
	@echo -n Compiling setup functions...
	@$(CPP) -traditional -I$(IPATH) $< -o setup.s
	@$(AS86) -o setup.o setup.s
	@$(LD86) -s -o $(SETUP) setup.o
	@rm -f setup.s setup.o
	@echo Done!

init.o:	init.c
	$(CC) -c init.c

logo.o:	logo.c
	$(CC) -c logo.c

text.o:	text.c
	$(CC) -c text.c

head.o:	head.S
	@$(CC) -traditional -c head.S

%.s: %.c
	@gcc -S -O1 -I../include -I../../../compiler/include $<

clean:
	@rm -f $(BOOTSECT) $(SETUP) $(HEADER)
	@rm -f *.o
