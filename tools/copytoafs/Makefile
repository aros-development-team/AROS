include $(TOP)/config/make.cfg

SILENT := yes

DEBUG_OPTS := -g -DDEBUG=0

FILES := copytoafs

ifeq ($(AROS_HOST_ARCH),mingw32)
LIBS := -lws2_32
endif

AFSSRCDIR := $(SRCDIR)/rom/filesys/afs
ifneq ($(HOSTGENDIR),)
OBJDIR := $(HOSTGENDIR)/tools/host_copytoafs
else
OBJDIR := ./tools/host_copytoafs
endif
OBJS   := $(patsubst %, $(OBJDIR)/%.o, $(FILES))
DEPS   := $(patsubst %, $(OBJDIR)/%.d, $(FILES))

INCLUDES := -I$(AFSSRCDIR) -I$(OBJDIR)
HOST_CFLAGS := $(HOST_CFLAGS) $(DEBUG_OPTS) -Wall -Werror-implicit-function-declaration $(INCLUDES) -O2

ifeq ($(SILENT),yes)
	ECHO := true
endif

.PHONY: all depend clean

all: depend $(OBJDIR) $(COPYTOAFS)

$(COPYTOAFS): $(OBJS)
	@$(ECHO) "Linking $@..."
	@$(HOST_CC) $(OBJS) $(AFSLIB) -o $@ $(LIBS)

$(OBJDIR)/config.h: $(OBJDIR) myconf.aros
	@./myconf.aros $(TOP) >> $@

$(OBJDIR)/%.o: %.c $(OBJDIR)/config.h
	@$(ECHO) "Compiling $(notdir $@)..."
	@$(HOST_CC) $(HOST_CFLAGS) -o $@ -c $<

$(OBJDIR)/%.d: %.c $(OBJDIR)/config.h
	@$(ECHO) "Finding dependencies for $<..."
	@$(HOST_CC) $(HOST_CFLAGS) -M -MT '$(patsubst %.d,%.o,$@)' $< > $@

$(OBJDIR):
	@$(MKDIR) $(OBJDIR)

clean:
	@$(RM) -f $(COPYTOAFS) $(OBJDIR)

depend: $(DEPS)

-include $(DEPS)

