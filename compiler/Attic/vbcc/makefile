
CC	= cc
CFLAGS	= -g -I. -I$(OBJDIR)
LDFLAGS = -lm

OBJDIR = stage1
BINDIR = bin1
TARGET = i386

all: setup $(BINDIR)/vcpp $(BINDIR)/vc $(BINDIR)/vbcc$(TARGET)

setup: makedirs prepstage

makedirs:
	@if test ! -d $(OBJDIR) ; then mkdir $(OBJDIR) ; else true ; fi
	@if test ! -d $(BINDIR) ; then mkdir $(BINDIR) ; else true ; fi

stage1: setup
	make TARGET=$(TARGET)

stage2: setup
	make OBJDIR=stage2 TARGET=$(TARGET) BINDIR=bin2 CC="bin1/vc +vc-stage2.config"

stage3: setup
	make OBJDIR=stage3 TARGET=$(TARGET) BINDIR=bin3 CC="bin2/vc +vc-stage3.config"

stage4: setup
	make OBJDIR=stage4 TARGET=$(TARGET) BINDIR=bin4 CC="bin3/vc +vc-stage4.config"

bootstrap:
	make stage1 TARGET=$(TARGET)
	make stage2 TARGET=$(TARGET)
	make stage3 TARGET=$(TARGET)
	make stage4 TARGET=$(TARGET)

compare:
	@for file in vc vcpp vbcc$(TARGET) ; do \
	    echo "Compare bin2/$$file bin3/$$file..." ; \
	    cmp bin2/$$file bin3/$$file ; \
	    echo "Compare bin3/$$file bin4/$$file..." ; \
	    cmp bin3/$$file bin4/$$file ; \
	done

clean:
	rm -r stage? bin?

.PHONY: stage1 stage2

prepstage: $(OBJDIR)/machine.h $(OBJDIR)/machine.c

$(OBJDIR)/machine.h : machines/$(TARGET)/machine.h
	cp $< $@

$(OBJDIR)/machine.c : machines/$(TARGET)/machine.c
	cp $< $@

vbccs: $(BINDIR)/vbccs$(TARGET)

$(BINDIR)/vc: frontend/vc.c
	$(CC) $(CFLAGS) frontend/vc.c -o $(BINDIR)/vc

vcppobjs =  $(OBJDIR)/cpp.o $(OBJDIR)/eval.o $(OBJDIR)/getopt.o \
	    $(OBJDIR)/hideset.o $(OBJDIR)/include.o \
	    $(OBJDIR)/lex.o $(OBJDIR)/macro.o $(OBJDIR)/nlist.o \
	    $(OBJDIR)/tokens.o $(OBJDIR)/unix.o

$(BINDIR)/vcpp: $(vcppobjs)
	$(CC) $(LDFLAGS) $(vcppobjs) -o $(BINDIR)/vcpp

$(OBJDIR)/cpp.o: vcpp/cpp.c vcpp/cpp.h
	$(CC) $(CFLAGS) -c -Ivcpp vcpp/cpp.c -o $@

$(OBJDIR)/eval.o: vcpp/eval.c vcpp/cpp.h
	$(CC) $(CFLAGS) -c -Ivcpp vcpp/eval.c -o $@

$(OBJDIR)/getopt.o: vcpp/getopt.c
	$(CC) $(CFLAGS) -c -Ivcpp vcpp/getopt.c -o $@

$(OBJDIR)/hideset.o: vcpp/hideset.c vcpp/cpp.h
	$(CC) $(CFLAGS) -c -Ivcpp vcpp/hideset.c -o $@

$(OBJDIR)/include.o: vcpp/include.c vcpp/cpp.h
	$(CC) $(CFLAGS) -c -Ivcpp vcpp/include.c -o $@

$(OBJDIR)/lex.o: vcpp/lex.c vcpp/cpp.h
	$(CC) $(CFLAGS) -c -Ivcpp vcpp/lex.c -o $@

$(OBJDIR)/macro.o: vcpp/macro.c vcpp/cpp.h
	$(CC) $(CFLAGS) -c -Ivcpp vcpp/macro.c -o $@

$(OBJDIR)/nlist.o: vcpp/nlist.c vcpp/cpp.h
	$(CC) $(CFLAGS) -c -Ivcpp vcpp/nlist.c -o $@

$(OBJDIR)/tokens.o: vcpp/tokens.c vcpp/cpp.h
	$(CC) $(CFLAGS) -c -Ivcpp vcpp/tokens.c -o $@

$(OBJDIR)/unix.o: vcpp/unix.c vcpp/cpp.h
	$(CC) $(CFLAGS) -c -Ivcpp vcpp/unix.c -o $@

bobjects = $(OBJDIR)/main.o $(OBJDIR)/vars.o $(OBJDIR)/declaration.o \
	   $(OBJDIR)/parse_expr.o $(OBJDIR)/type_expr.o $(OBJDIR)/ic.o \
	   $(OBJDIR)/machine.o $(OBJDIR)/statements.o $(OBJDIR)/preproc.o \
	   $(OBJDIR)/supp.o $(OBJDIR)/dt.o

fobjects = $(OBJDIR)/opt.o $(OBJDIR)/av.o $(OBJDIR)/rd.o $(OBJDIR)/regs.o \
	   $(OBJDIR)/flow.o $(OBJDIR)/cse.o $(OBJDIR)/cp.o $(OBJDIR)/loop.o \
	   $(OBJDIR)/alias.o $(bobjects)

sobjects = $(OBJDIR)/opts.o $(OBJDIR)/regss.o $(bobjects)

tasm	 = $(OBJDIR)/supp.o $(OBJDIR)/tasm.o $(OBJDIR)/dt.o \
	   $(OBJDIR)/opt.o $(OBJDIR)/av.o $(OBJDIR)/rd.o $(OBJDIR)/regs.o \
	   $(OBJDIR)/flow.o $(OBJDIR)/cse.o $(OBJDIR)/cp.o $(OBJDIR)/loop.o \
	   $(OBJDIR)/alias.o $(OBJDIR)/machine.o

mbasic	 = $(OBJDIR)/supp.o $(OBJDIR)/mbasic.o $(OBJDIR)/dt.o \
	   $(OBJDIR)/opt.o $(OBJDIR)/av.o $(OBJDIR)/rd.o $(OBJDIR)/regs.o \
	   $(OBJDIR)/flow.o $(OBJDIR)/cse.o $(OBJDIR)/cp.o $(OBJDIR)/loop.o \
	   $(OBJDIR)/alias.o $(OBJDIR)/machine.o

vscobjects = $(OBJDIR)/vsc.o $(OBJDIR)/schedule.o

$(BINDIR)/vbcc$(TARGET): $(fobjects)
	$(CC) $(LDFLAGS) $(fobjects) -o $(BINDIR)/vbcc$(TARGET)

$(BINDIR)/vbccs$(TARGET): $(sobjects)
	$(CC) $(LDFLAGS) $(sobjects) -o $(BINDIR)/vbccs$(TARGET)

bin/vsc$(TARGET): $(vscobjects)
	$(CC) $(LDFLAGS) $(vscobjects) -o bin/vsc$(TARGET)

$(BINDIR)/tasm$(TARGET): $(tasm)
	$(CC) $(LDFLAGS) $(tasm) -o $(BINDIR)/tasm$(TARGET)

$(BINDIR)/mbasic$(TARGET): $(mbasic)
	$(CC) $(LDFLAGS) $(mbasic) -o $(BINDIR)/mbasic$(TARGET)

$(BINDIR)/dtgen: datatypes/dtgen.c datatypes/datatypes.h datatypes/dtconv.h
	$(CC) datatypes/dtgen.c -o $(BINDIR)/dtgen -Idatatypes

#dt.h: $(BINDIR)/dtgen $(OBJDIR)/machine.dt
#	 $(BINDIR)/dtgen $(OBJDIR)/machine.dt dt.h dt.c
#
#dt.c: $(BINDIR)/dtgen $(OBJDIR)/machine.dt
#	 $(BINDIR)/dtgen $(OBJDIR)/machine.dt dt.h dt.c

$(OBJDIR)/dt.o: dt.h dt.c
	$(CC) $(CFLAGS) -c dt.c -o $(OBJDIR)/dt.o -Idatatypes

$(OBJDIR)/tasm.o: tasm.c supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c tasm.c -o $(OBJDIR)/tasm.o

$(OBJDIR)/mbasic.o: mbasic.c supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c mbasic.c -o $(OBJDIR)/mbasic.o

$(OBJDIR)/supp.o: supp.c supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c supp.c -o $(OBJDIR)/supp.o

$(OBJDIR)/main.o: main.c vbc.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c main.c -o $(OBJDIR)/main.o

$(OBJDIR)/vars.o: vars.c vbc.h supp.h $(OBJDIR)/machine.h errors.h dt.h
	$(CC) $(CFLAGS) -c vars.c -o $(OBJDIR)/vars.o

$(OBJDIR)/declaration.o: declaration.c vbc.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c declaration.c -o $(OBJDIR)/declaration.o

$(OBJDIR)/parse_expr.o: parse_expr.c vbc.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c parse_expr.c -o $(OBJDIR)/parse_expr.o

$(OBJDIR)/type_expr.o: type_expr.c vbc.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c type_expr.c -o $(OBJDIR)/type_expr.o

$(OBJDIR)/ic.o: ic.c vbc.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c ic.c -o $(OBJDIR)/ic.o

$(OBJDIR)/statements.o: statements.c vbc.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c statements.c -o $(OBJDIR)/statements.o

$(OBJDIR)/opt.o: opt.c opt.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c opt.c -o $(OBJDIR)/opt.o

$(OBJDIR)/av.o: av.c opt.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c av.c -o $(OBJDIR)/av.o

$(OBJDIR)/rd.o: rd.c opt.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c rd.c -o $(OBJDIR)/rd.o

$(OBJDIR)/regs.o: regs.c opt.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c regs.c -o $(OBJDIR)/regs.o

$(OBJDIR)/flow.o: flow.c opt.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c flow.c -o $(OBJDIR)/flow.o

$(OBJDIR)/cse.o: cse.c opt.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c cse.c -o $(OBJDIR)/cse.o

$(OBJDIR)/cp.o: cp.c opt.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c cp.c -o $(OBJDIR)/cp.o

$(OBJDIR)/loop.o: loop.c opt.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c loop.c -o $(OBJDIR)/loop.o

$(OBJDIR)/alias.o: alias.c opt.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c alias.c -o $(OBJDIR)/alias.o

$(OBJDIR)/preproc.o: preproc.c vbpp.h supp.h vbc.h dt.h
	$(CC) $(CFLAGS) -c preproc.c -o $(OBJDIR)/preproc.o

$(OBJDIR)/machine.o: $(OBJDIR)/machine.c supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c $(OBJDIR)/machine.c -o $(OBJDIR)/machine.o

$(OBJDIR)/opts.o: opt.c opt.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c -DNO_OPTIMIZER opt.c -o $(OBJDIR)/opts.o

$(OBJDIR)/regss.o: regs.c opt.h supp.h $(OBJDIR)/machine.h dt.h
	$(CC) $(CFLAGS) -c -DNO_OPTIMIZER regs.c -o $(OBJDIR)/regss.o

$(OBJDIR)/vsc.o: vsc/vsc.h vsc/vsc.c $(OBJDIR)/schedule.h
	$(CC) -c vsc/vsc.c -o $(OBJDIR)/vsc.o -I$(OBJDIR)

$(OBJDIR)/schedule.o: vsc/vsc.h $(OBJDIR)/schedule.h $(OBJDIR)/schedule.c
	$(CC) -c $(OBJDIR)/schedule.c -o $(OBJDIR)/schedule.o -I$(OBJDIR) -Ivsc


