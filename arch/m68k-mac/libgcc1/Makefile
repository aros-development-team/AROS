#
# Makefile pour construire une cross-librairie libgcc1 pour
# gcc sur 68000.
#
#
# $Id$
# $Log$
# Revision 1.1  2002/01/13 00:27:32  bergers
# Renamed target to m68k-pp-native and did some cleanups.
#
# Revision 1.1  2001/07/07 16:51:04  bergers
# Libgcc1 as needed for gcc for m68000 targets since the compiler itself does not know the code for certain 32 bit operations that are not part of the m68000 instruction set. Found this library on http://rpmfind.net/pub/CROSS-68000. I contacted the author (Daniel Veillard) and he said it is alright for us to use the code.
#
# Revision 1.2  1994/06/30  09:55:20  veillard
# Changed to use gcc68 as the interface for the comassembler
#
# Revision 1.1  1993/07/21  14:25:56  veillard
# Initial revision
#
#
SRCSS= _addsubdf3.s _addsubsf3.s _divdf3.s _divmodsi3.s _divsf3.s _eqdf2.s \
_eqsf2.s _extend.s _extendsfdf2.s _fixdfsi.s _fixsfsi.s _floatsidf.s \
_floatsisf.s _gedf2.s _gesf2.s _gtdf2.s _gtsf2.s _ledf2.s _lesf2.s _ltdf2.s \
_ltsf2.s _muldf3.s _mulsf3.s _mulsi3.s _nedf2.s _negdf2.s _negsf2.s _nesf2.s \
_normdf.s _normsf.s _truncdfsf2.s _udivmodsi3.s

SRCS= Makefile $(SRCSS)
OBJS= _addsubdf3.o _addsubsf3.o _divdf3.o _divmodsi3.o _divsf3.o _eqdf2.o \
_eqsf2.o _extend.o _extendsfdf2.o _fixdfsi.o _fixsfsi.o _floatsidf.o \
_floatsisf.o _gedf2.o _gesf2.o _gtdf2.o _gtsf2.o _ledf2.o _lesf2.o _ltdf2.o \
_ltsf2.o _muldf3.o _mulsf3.o _mulsi3.o _nedf2.o _negdf2.o _negsf2.o _nesf2.o \
_normdf.o _normsf.o _truncdfsf2.o _udivmodsi3.o
LIB= libgcc1.a

AR=m68k-linux-ar
AS=m68k-linux-as
CC=m68k-linux-gcc

SUBDIRS=     # pas de sous repertoires

#
# regle principale
#

all : $(LIB)

#
# definitions liees a RCS
#
CO= co -q
CI= ci -q -r
RLOG= rlog -q
RCSDIFF= rcsdiff -q
RCS_UNLOCK= rcs -U -q
RCS_LOCK= rcs -L -q
RM=rm -f


#
# regle de recuperation automatique des fichiers
# sources necessaires a la compilation.
#
$(SRCS) : 
	@echo $@ "<--- RCS"
	@$(CO) $@

#
# regle de restoration des fichier depuis la base RCS
#
restore :
	@echo "***** restoring from  RCS for " $(COMPONENT)
	@for i in $(SRCS) ; do echo $$i ; $(RM) $$i ; $(CO) $$i ; \
             done ; exit 0
	@if [ "$(SUBDIRS)" != "" ] ; then make restore_subdir ; fi ; \
             exit 0
restore_subdir :
	@for i in $(SUBDIRS) ; do echo $$i ; cd $$i ; make restore ; \
         cd .. ;  done
 
#
# regle de sauvegarde des fichier dans la base RCS
#
rcs : 
	@echo "unlocking RCS files"
	@for i in $(SRCS) ; do if [ -f $$i ] ; then \
         $(RCS_UNLOCK) $$i ; fi ; done  ; exit 0
	@echo "saving RCS version"
	@(for i in $(SRCS) ; do if [ -f $$i ] ; then \
         echo $$i "---> RCS" ;  \
         $(RCSDIFF) $$i || test $$? -ne 0 ; \
         $(CI) $$i || test $$? -ne 0 ; fi ; \
         if [ "$$i" = "Makefile" ] ; then $(CO) Makefile ; fi ; \
         done  ; exit 0)
	@if [ "$(SUBDIRS)" != "" ] ; then make rcs_subdir ; fi ; exit 0
rcs_subdir:
	@for i in $(SUBDIRS) ; do echo $$i ; cd $$i ; make rcs ; \
         cd .. ; done 
  
#
# dependances
#
.s.o:
	$(AS) $< -o $@

$(LIB) : $(OBJS)
	$(AR) rv $(LIB) $(OBJS)

clean :
	rm -f $(LIB) $(OBJS)
