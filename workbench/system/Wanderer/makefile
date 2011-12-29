CC        = gcc
OBJ       = support.o main.o iconwindow.o wandererprefsfont.o wanderer.o locale.o filesystems.o iconwindowbackfill.o iconwindow.o iconwindow.o iconwindow_iconlist.o iconwindow_volumelist.o iconwindow_drawerlist.o iconwindow_networkbrowserlist.o portable_utils.o Classes/icondrawerlist.o Classes/iconlist.o Classes/iconlistview.o Classes/iconvolumelist.o wandererprefs.o
LINKOBJ   = support.o main.o iconwindow.o wandererprefsfont.o wanderer.o locale.o filesystems.o iconwindowbackfill.o iconwindowcontents.o portable_utils.o Classes/icondrawerlist.o Classes/iconlist.o Classes/iconlistview.o Classes/iconvolumelist.o wandererprefs.o

INCS      =
CXXINCS   =
BIN       = Wanderer
DEFINES   = 
CXXFLAGS  = $(CXXINCS) $(DEFINES)  
CFLAGS    = -O2 $(INCS) $(DEFINES)
RM        = rm -f
LIBS      =
LINK      = gcc

.PHONY: all all-before all-after clean clean-custom
all: all-before $(BIN) all-after

clean: clean-custom
	$(RM) $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(LINK) $(LINKOBJ) -o "Wanderer_MOS" $(LIBS)

support.o: $(GLOBALDEPS) support.c
	$(CC) -c support.c -o support.o $(CFLAGS)

main.o: $(GLOBALDEPS) main.c
	$(CC) -c main.c -o main.o $(CFLAGS)

iconwindow.o: $(GLOBALDEPS) iconwindow.c
	$(CC) -c iconwindow.c -o iconwindow.o $(CFLAGS)

wandererprefsfont.o: $(GLOBALDEPS) wandererprefsfont.c
	$(CC) -c wandererprefsfont.c -o wandererprefsfont.o $(CFLAGS)

wanderer.o: $(GLOBALDEPS) wanderer.c
	$(CC) -c wanderer.c -o wanderer.o $(CFLAGS)

locale.o: $(GLOBALDEPS) locale.c strings.h
	$(CC) -c locale.c -o locale.o $(CFLAGS)

filesystems.o: $(GLOBALDEPS) filesystems.c filesystems.h
	$(CC) -c filesystems.c -o filesystems.o $(CFLAGS)

iconwindowbackfill.o: $(GLOBALDEPS) iconwindowbackfill.c iconwindowbackfill.h
	$(CC) -c iconwindowbackfill.c -o iconwindowbackfill.o $(CFLAGS)

iconwindowcontents.o: $(GLOBALDEPS) iconwindowcontents.c iconwindowcontents.h
	$(CC) -c iconwindowcontents.c -o iconwindowcontents.o $(CFLAGS)

portable_utils.o: $(GLOBALDEPS) portable_utils.c 
	$(CC) -c portable_utils.c -o portable_utils.o $(CFLAGS)

Classes/icondrawerlist.o: $(GLOBALDEPS) Classes/icondrawerlist.c Classes/icondrawerlist_private.h 
	$(CC) -c Classes/icondrawerlist.c -o Classes/icondrawerlist.o $(CFLAGS)

Classes/iconlist.o: $(GLOBALDEPS) Classes/iconlist.c Classes/iconlist_private.h
	$(CC) -c Classes/iconlist.c -o Classes/iconlist.o $(CFLAGS)

Classes/iconlistview.o: $(GLOBALDEPS) Classes/iconlistview.c Classes/iconlistview_private.h
	$(CC) -c Classes/iconlistview.c -o Classes/iconlistview.o $(CFLAGS)

Classes/iconvolumelist.o: $(GLOBALDEPS) Classes/iconvolumelist.c Classes/iconvolumelist_private.h
	$(CC) -c Classes/iconvolumelist.c -o Classes/iconvolumelist.o $(CFLAGS)

wandererprefs.o: $(GLOBALDEPS) wandererprefs.c
	$(CC) -c wandererprefs.c -o wandererprefs.o $(CFLAGS)

strings.h: Catalogs/wanderer.cd
	catcomp $? CFILE $@
