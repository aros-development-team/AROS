DEBUG =
CFLAGS = -Wall -Wunused -O2
CC = gcc
STRIP = strip
ECHO = echo

EXE = archtool

ARCHIVE = spbase.arch
SPECIAL = spenhanced.arch
MERGE = spmerged.arch
MMERGE = sp.arch

GENSRC = spadd.c spdiv.c spmul.c spneg.c spsub.c spsquare.c
CSRC = spbase.c

all   : $(EXE)

$(EXE) : archtool.c
	@$(ECHO) "Compiling $@..."
	@$(CC) $(CFLAGS) $(DEBUG) $< -o $@
	@$(STRIP) $@


clean :
	rm -f $(EXE) *.o functable.c *.adoc $(MERGE) $(GENSRC) $(CSRC) $(MMERGE) libdefs.h


test : $(EXE)
	@$(ECHO) "Extracting all functions from $(ARCHIVE)..."
	@$(EXE) -e . $(ARCHIVE)
	@$(ECHO) "Generating AutoDocs of all functions from $(ARCHIVE)..."
	@$(EXE) -a . $(ARCHIVE)
	@$(ECHO) "Generating functable.c from $(ARCHIVE)..."
	@$(EXE) -t MathFFP $(ARCHIVE)
	@$(ECHO) "Merging $(ARCHIVE) and $(SPECIAL) to $(MERGE)..."
	@$(EXE) -m $(ARCHIVE) $(SPECIAL) $(MERGE)
	@$(ECHO) "Extracting source from $(ARCHIVE) to $(CSRC)..."
	@$(EXE) -s $(ARCHIVE) $(CSRC)
	@$(ECHO) "Merging multiple files..."
	@$(EXE) -M $(MMERGE) $(MERGE) $(SPECIAL) $(ARCHIVE)
	@$(ECHO) "Generating libdefs.h from lib.conf..."
	@$(EXE) -c

