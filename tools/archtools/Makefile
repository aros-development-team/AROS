DEBUG = 
CFLAGS = -Wall -Wunused -O2
CC = gcc
STRIP = strip
ECHO = echo

EXES = genfunctable genautodocs extractfiles mergearch gensource

ARCHIVE = spbase.arch
SPECIAL = spenhanced.arch
MERGE = spmerged.arch

GENSRC = spadd.c spdiv.c spmul.c spneg.c spsub.c spsquare.c
CSRC = spbase.c

all   : $(EXES)

genfunctable : line.o genfunctable.o
	@$(ECHO) "Linking $@..."
	@$(CC) $(CFLAGS) $(DEBUG) $? -o $@
	@$(STRIP) $@

gensource : line.o gensource.o
	@$(ECHO) "Linking $@..."
	@$(CC) $(CFLAGS) $(DEBUG) $? -o $@
	@$(STRIP) $@

genautodocs : line.o genautodocs.o
	@$(ECHO) "Linking $@..."
	@$(CC) $(CFLAGS) $(DEBUG) $? -o $@
	@$(STRIP) $@

extractfiles : line.o extractfiles.o
	@$(ECHO) "Linking $@..."
	@$(CC) $(CFLAGS) $(DEBUG) $? -o $@
	@$(STRIP) $@

mergearch : line.o mergearch.o
	@$(ECHO) "Linking $@..."
	@$(CC) $(CFLAGS) $(DEBUG) $? -o $@
	@$(STRIP) $@

%.o : %.c
	@$(ECHO) "Compiling $@..."
	@$(CC) $(CFLAGS) $(DEBUG) -c $< -o $@

clean :
	rm -f $(EXES) *.o functable.c *.adoc $(MERGE) $(GENSRC) $(CSRC)


test : $(EXES)
	@$(ECHO) "Extracting all functions from $(ARCHIVE)..."
	@extractfiles . $(ARCHIVE)
	@$(ECHO) "Generating AutoDocs of all functions from $(ARCHIVE)..."
	@genautodocs . $(ARCHIVE)
	@$(ECHO) "Generating functable.c from $(ARCHIVE)..."
	@genfunctable MathFFP $(ARCHIVE)
	@$(ECHO) "Merging $(ARCHIVE) and $(SPECIAL) to $(MERGE)..."
	@mergearch $(ARCHIVE) $(SPECIAL) $(MERGE)
	@$(ECHO) "Extracting source from $(ARCHIVE) to $(CSRC)..."
	@gensource $(ARCHIVE) $(CSRC)

