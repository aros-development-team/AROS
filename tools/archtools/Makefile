DEBUG =
CFLAGS = -Wall -Wunused -O2
CC = gcc
STRIP = strip
ECHO = echo
RM = rm

EXE = archtool

all   : $(EXE)

$(EXE) : archtool.c
	@$(ECHO) "Compiling $@..."
	@$(CC) $(CFLAGS) $(DEBUG) $< -o $@
	@$(STRIP) $@


clean :
	@$(ECHO) "Cleaning sources..."
	@$(RM) -f $(EXE)

