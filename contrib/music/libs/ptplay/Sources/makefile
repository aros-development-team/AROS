# Project files

OBJECTS	=	LibHeader.o LibInit.o ptinit.o ptplay.o ptplaytest.o

ODIRGCC	=	ObjectsGCC/
ODIR68K	=	Objects68k/

OBJS68K	=	$(addprefix $(ODIR68K),$(OBJECTS))
OBJSGCC	=	$(addprefix $(ODIRGCC),$(OBJECTS))

###########################################################
# VBCC
###########################################################

VBCC		=	DH1:Ohjelmointi/VBCC/Bin/vc

SET68K	= +aos68k -cpu=68020 -fpu=68040 -sc
SETOPT	= -D__VBCC__ -DAMIGA -Iusr:include -k -c -c99 -size -O3 -maxoptpasses=20 -inline-size=20 -unroll-size=20 -schedule

###########################################################
# GCC
###########################################################

GCCMOS	=	gcc -noixemul

DEFMOS	=	-DUSE_INLINE_STDARG

INCMOS	=	-I. -I/usr/include

OPTMOS	=	-O2							\
				-ffast-math					\
				-finline-functions		\
				-fomit-frame-pointer		\
				-mcpu=750					\
				-mmultiple					\
				-mno-prototype				\
				-mstring						\
				-Wall							\
				-Wno-parentheses			\
				-Wno-missing-braces		\
				-c

G_LIBMOS	=
G_LFLAGS	=

CODETYPE	=	PPC
TARGET	=	PPC

mos	: ptplay.library.elf
68k	: ptplay.library
all	: ptplay.library.elf ptplay.library

###########################################################
# GCC MorphOS 
###########################################################

ptplay.library.elf : $(OBJSGCC)
	ppc-morphos-gcc -noixemul -nostartfiles $(G_LIBMOS) -o $@.db $(OBJSGCC) $(G_LFLAGS)
	ppc-morphos-strip --strip-unneeded --remove-section=.comment $@.db -o $@

$(ODIRGCC)LibHeader.o	: LibHeader.c LibHeader.h
	$(GCCMOS) $(INCMOS) $(OPTMOS) LibHeader.c -o $@

$(ODIRGCC)LibInit.o		: LibInit.c LibHeader.h
	$(GCCMOS) $(INCMOS) $(OPTMOS) $(DEFMOS) LibInit.c -o $@

$(ODIRGCC)ptinit.o		: ptinit.c ptplay_priv.h
	$(GCCMOS) $(INCMOS) $(OPTMOS) $(DEFMOS) ptinit.c -o $@

$(ODIRGCC)ptplay.o		: ptplay.c ptplay_priv.h
	$(GCCMOS) $(INCMOS) $(OPTMOS) $(DEFMOS) ptplay.c -o $@

$(ODIRGCC)ptplaytest.o	: ptplaytest.c ptplay_priv.h
	$(GCCMOS) $(INCMOS) $(OPTMOS) $(DEFMOS) ptplaytest.c -o $@

dump:
	objdump --disassemble-all ptplay.library.elf.db >ptplay.s

###########################################################
# VBCC AmigaOS
###########################################################

ptplay.library	: $(OBJS68K)
	$(VBCC) +aos68k -final $(OBJS68K) -o $@ -nostdlib

$(ODIR68K)LibHeader.o	: LibHeader.c LibHeader.h
	$(VBCC) +aos68k $(SETOPT) LibHeader.c -o $@

$(ODIR68K)LibInit.o		: LibInit.c LibHeader.h
	$(VBCC) $(SET68K) $(SETOPT) LibInit.c -o $@

$(ODIR68K)ptinit.o		: ptinit.c ptplay_priv.h
	$(VBCC) $(SET68K) $(SETOPT) -DAMIGA ptinit.c -o $@

$(ODIR68K)ptplay.o		: ptplay.c ptplay_priv.h
	$(VBCC) $(SET68K) $(SETOPT) -DAMIGA ptplay.c -o $@

$(ODIR68K)ptplaytest.o		: ptplaytest.c ptplay_priv.h
	$(VBCC) $(SET68K) $(SETOPT) -DAMIGA ptplaytest.c -o $@
