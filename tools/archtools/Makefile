DEBUG = 
CFLAGS = -Wall -Wunused -O2
CC = gcc
STRIP = strip
ECHO = echo

EXES = genfunctable genautodocs extractfiles mergearch

ARCHIVE = spbase.arch
SPECIAL = spenhanced.arch
MERGE = spmerged.arch

GENSRC = spadd.c spdiv.c spmul.c spneg.c spsub.c

all   : $(EXES)

genfunctable : line.o genfunctable.o
	@$(ECHO) "Linking $@..."
	@$(CC) $(CFLAGS) $(DEBUG) line.o genfunctable.o -o $@
	@$(STRIP) $@

genautodocs : line.o genautodocs.o
	@$(ECHO) "Linking $@..."
	@$(CC) $(CFLAGS) $(DEBUG) line.o genautodocs.o -o $@
	@$(STRIP) $@

extractfiles : line.o extractfiles.o
	@$(ECHO) "Linking $@..."
	@$(CC) $(CFLAGS) $(DEBUG) line.o extractfiles.o -o $@
	@$(STRIP) $@

mergearch : line.o mergearch.o
	@$(ECHO) "Linking $@..."
	@$(CC) $(CFLAGS) $(DEBUG) line.o mergearch.o -o $@
	@$(STRIP) $@

%.o : %.c
	@$(ECHO) "Compiling $@..."
	@$(CC) $(CFLAGS) $(DEBUG) -c $< -o $@

clean :
	rm -f $(EXES) *.o functable.c *.adoc $(MERGE) $(GENSRC)


test : $(EXES)
	@$(ECHO) "Extracting all functions from $(ARCHIVE)..."
	@extractfiles . $(ARCHIVE)
	@$(ECHO) "Generating AutoDocs of all functions from $(ARCHIVE)..."
	@genautodocs . $(ARCHIVE)
	@$(ECHO) "Generating functable.c from $(ARCHIVE)..."
	@genfunctable MathFFP $(ARCHIVE)
	@$(ECHO) "Merging $(ARCHIVE) and $(SPECIAL) to $(MERGE)..."
	@mergearch $(ARCHIVE) $(SPECIAL) $(MERGE)

