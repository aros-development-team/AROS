TOP=..

include $(TOP)/make.cfg

all: $(SLIBDIR)/dummy.library

clean:
	$(RM) $(SLIBDIR)/dummy.library

$(SLIBDIR)/dummy.library: dummylib.c
	$(CC) $(CFLAGS) -I $(TOP) $^ -c -o $@
