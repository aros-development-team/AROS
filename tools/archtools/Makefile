all: real-all

include $(TOP)/config/make.cfg

CFLAGS := -Wall -Wunused -O2

real-all: $(ARCHTOOL)

$(ARCHTOOL) : archtool.c
	@$(ECHO) "Compiling $@..."
	@$(CC) $(CFLAGS) $< -o $@
	@$(STRIP) $@

clean:
	@$(ECHO) "Cleaning sources..."
	@$(RM) -f $(ARCHTOOL)

