# This Makefile is a huge hack but that's because SMake doesn't support é in
# Moiré (it just dies) and DMake doesn't support overriding variables (as in:
# DMake NAME=Bob). So I have to use GNU Make, but it tries to translate leading
# /'s into a volume which makes things a real pain in the butt, moreover it
# has no clue about paths (or more importantly multi-volume assigns) and so I
# have to specify full path name to all the stinking executables.

DIR = ../../../GBlanker/Blankers
DOSDIR = ///GBlanker/Blankers

CC = SC:c/SC $(MATH)
LD = SC:c/SLink

RM = GNU:bin/rm
RMFLAGS = -f
CP = GNU:bin/cp

TEXT = $(NAME).txt
INTERFACE = $(NAME).ifc

SCOPS = SCOPTIONS

$(NAME): $(SCOPS) ../main.o blank.o
	Echo "  Linking $(NAME)"
	$(CC) LINK PNAME=$(NAME) /main.o blank.o
	$(RM) $(RMFLAGS) $(NAME).lnk

$(NAME)Dbg: $(SCOPS) ../debug.o blank.o
	Echo "  Linking $(NAME)Dbg"
	$(CC) LINK PNAME=$(NAME)Dbg /debug.o blank.o
	$(RM) $(RMFLAGS) $(NAME)Dbg.lnk

$(SCOPS): ../$(SCOPS) LOCAL_$(SCOPS)
	Echo "  Generating $(SCOPS)"
	C:Join /$(SCOPS) LOCAL_$(SCOPS) AS $(SCOPS)

../main.o: ../main.c
	Echo "  Compiling ../main.c"
	$(CC) /main.c

../debug.o: ../debug.c
	Echo "  Compiling ../debug.c"
	$(CC) /debug.c

.c.o:
	Echo "  Compiling $*.c"
	$(CC) $*.c

revision:
	Echo "  Bumping revision of $(NAME)"
	SYS:Hack/Bin/BumpRev 38 $(NAME)
	$(RM) $(RMFLAGS) $(NAME)_rev.i

clean:
	Echo "  Deleting *.o and $(NAME)"
	$(RM) $(RMFLAGS) ../main.o blank.o $(NAME)

$(DIR)/$(NAME): $(NAME)
	Echo "  Installing $(NAME) in $(DIR)"
	$(LD) QUIET NODEBUG NOICONS FROM $(NAME) TO $(DOSDIR)/$(NAME)

$(DIR)/$(INTERFACE): $(INTERFACE)
	Echo "  Copying $(INTERFACE) to $(DIR)"
	$(CP) $(INTERFACE) $(DIR)

$(DIR)/$(TEXT): $(TEXT)
	Echo "  Copying $(TEXT) to $(DIR)"
	$(CP) $(TEXT) $(DIR)

install: $(DIR)/$(NAME) $(DIR)/$(INTERFACE) $(DIR)/$(TEXT)

debug: $(NAME)Dbg
