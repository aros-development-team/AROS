NAME    = Garshneblanker
ITPNAME = PrefInterp
DIR     = /GBlanker

BGUIEXES     = BGUI/$(NAME) BGUI/$(ITPNAME)
MUIEXES      = MUI/$(NAME) MUI/$(ITPNAME)
GADTOOLSEXES = GadTools/ShowInfo #GadTools/$(NAME)
EXES         = $(BGUIEXES) $(MUIEXES) $(GADTOOLSEXES)

BGUIINSTALL     = $(DIR)/BGUI/$(NAME) $(DIR)/BGUI/$(ITPNAME)
MUIINSTALL      = $(DIR)/MUI/$(NAME) $(DIR)/MUI/$(ITPNAME)
GADTOOLSINSTALL = $(DIR)/GadTools/ShowInfo #$(DIR)/GadTools/$(NAME)
INSTALLEXES     = $(BGUIINSTALL) $(MUIINSTALL) $(GADTOOLSINSTALL)

CC = SC
LD = SLink

LDFLAGS = NODEBUG STRIPDEBUG NOICONS QUIET

COMMON_OBJS   = main.o prefs.o module.o cxhand.o libraries.o handlers.o var.o
GADTOOLS_OBJS = Gadtools/interface.o Gadtools/Garshneblanker.o
BGUI_OBJS     = BGUI/interface.o
MUI_OBJS      = MUI/interface.o
ALL_OBJS      = $(COMMON_OBJS) $(BGUI_OBJS) $(GADTOOLS_OBJS) $(MUI_OBJS)

all: $(EXES)

BGUI/$(NAME): $(COMMON_OBJS) $(BGUI_OBJS)
	$(CC) LINK $(BGUI_OBJS) $(COMMON_OBJS) PNAME BGUI/$(NAME)
	Delete FORCE QUIET BGUI/$(NAME).lnk

GadTools/$(NAME): $(COMMON_OBJS) $(GADTOOLS_OBJS)
	$(CC) LINK $(GADTOOLS_OBJS) $(COMMON_OBJS) PNAME GadTools/$(NAME)
	Delete FORCE QUIET GadTools/$(NAME).lnk

MUI/$(NAME): $(COMMON_OBJS) $(MUI_OBJS)
	$(CC) LINK $(MUI_OBJS) $(COMMON_OBJS) PNAME MUI/$(NAME)
	Delete FORCE QUIET MUI/$(NAME).lnk

Gadtools/Garshneblanker.o: Gadtools/Garshneblanker.c
	$(CC) IGN=100 IGN=147 IGN=154 $*.c

BGUI/$(ITPNAME): var.o parse.o BGUI/$(ITPNAME).c
	$(CC) LINK BGUI/$(ITPNAME).c parse.o var.o
	Delete FORCE QUIET BGUI/$(ITPNAME).lnk

MUI/$(ITPNAME): parse.o MUI/$(ITPNAME).c
	$(CC) LINK MUI/$(ITPNAME) parse.o
	Delete FORCE QUIET MUI/$(ITPNAME).lnk

GadTools/ShowInfo: GadTools/ShowInfo.c
	$(CC) LINK GadTools/ShowInfo.c
	Delete FORCE QUIET GadTools/ShowInfo.lnk

.c.o:
	$(CC) $*.c

.a.o:
	$(CC) $*.a

clean:
	Delete FORCE QUIET \#?.o BGUI/\#?.o MUI/\#?.o GadTools/\#?.o $(EXES)

$(DIR)/BGUI/$(NAME): BGUI/$(NAME)
	$(LD) $(LDFLAGS) FROM BGUI/$(NAME) TO $(DIR)/BGUI/$(NAME)

$(DIR)/BGUI/$(ITPNAME): BGUI/$(ITPNAME)
	$(LD) $(LDFLAGS) FROM BGUI/$(ITPNAME) TO $(DIR)/BGUI/$(ITPNAME)

$(DIR)/MUI/$(NAME): MUI/$(NAME)
	$(LD) $(LDFLAGS) FROM MUI/$(NAME) TO $(DIR)/MUI/$(NAME)

$(DIR)/MUI/$(ITPNAME): MUI/$(ITPNAME)
	$(LD) $(LDFLAGS) FROM MUI/$(ITPNAME) TO $(DIR)/MUI/$(ITPNAME)

$(DIR)/GadTools/$(NAME): GadTools/$(NAME)
	$(LD) $(LDFLAGS) FROM GadTools/$(NAME) TO $(DIR)/GadTools/$(NAME)

$(DIR)/GadTools/ShowInfo: GadTools/ShowInfo
	$(LD) $(LDFLAGS) FROM GadTools/ShowInfo TO $(DIR)/GadTools/ShowInfo

install: $(INSTALLEXES)

bumprev:
	BumpRev 38 Garshneblanker

revision: bumprev
	Delete FORCE QUIET Garshneblanker_rev.i
