#
# GNU Makefile for JanoPref. By T.Pierron
# For use with gcc 2.7.0+
#

# System environment
EXE    = ../../JanoPrefs
CC     = gcc -s
CFLAGS = -I/amiga/include -I.. -O2 -DJANOPREF -m68020
LIBS   = -noixemul
RM     = rm -f #Delete quiet
VPATH  = ..

OBJ = JanoPrefs.o Prefs.o PrefsPort.o Prefs_Strings.o SampleEd.o Utils.o


# Implicit rule to produce object file
%.o:%.c
	$(CC) -c $< $(CFLAGS)

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(OBJ) -o $(EXE) $(LIBS)

clean:
	$(RM) $(OBJ)

# Special  dependencies
JanoPrefs.o: Prefs.h JanoPrefs.h Utils.h IPC_Prefs.h Prefs_Strings.h Jed_Strings.h Sample.h
PrefsPort.o: Prefs.h PrefsPort.c Version.h IPC_Prefs.h
Prefs_Strings.o: Prefs_Strings.h Jed_Strings.h
SampleEd.o: Prefs.h Project.c Project.h Gui.h Memory.h UndoRedo.h Sample.h
Utils.o: Prefs.h Utils.c JanoPrefs.h Utils.h Jed_Strings.h
