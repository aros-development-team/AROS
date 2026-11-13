MAKE   = make
CC     = vc +aosppc
CP	   = copy
RM     = delete
MAKEDT = RX REXX:MakeDT.rexx

# options to specify on make command-line

# enable/disable gsm support
GSM = 1
# enable/disable g72x support
G72X = 1

ifeq "$(GSM)" "1"
GSMOPT = -DGSM610_SUPPORT
GSMLIB = -Lgsm -lgsm
GSMSRC = wave_gsm610.c
else
GSMOPT =
GSMLIB =
GSMSRC =
endif

ifeq "$(G72X)" "1"
G72XOPT = -DG72X_SUPPORT
G72XSRC = wave_g72x.c g72x/g72x.c g72x/g721.c g72x/g723_16.c g72x/g723_24.c g72x/g723_40.c \
	bitpack_msb.c
else
G72XOPT =
G72XSRC =
endif

# Change these as required
OPTIMIZE = -O4 -speed -maxoptpasses=15
DEBUG = 
CFLAGS = $(OPTIMIZE) $(DEBUG) -I. -Iinclude $(GSMOPT) $(G72XOPT)

# Flags passed to gcc during linking
LINK = $(OPTIMIZE) $(DEBUG)

# Additional linker libraries
LIBS = $(GSMLIB) -lvc -lm

# Name of the "thing" to build
TARGET  = wave.datatype
DT_DESC = WAVE
VERSION = 50

# Source code files used in this project
# Add any additional files to this line

SRCS = init.c wave_class.c decoders.c wave_pcm.c wave_ima_adpcm.c wave_ms_adpcm.c wave_alaw.c \
	wave_ieee_float.c $(G72XSRC) $(GSMSRC) \
	bitpack.c bitpack_lsb.c

# -------------------------------------------------------------
# Nothing should need changing below this line

OBJS = $(SRCS:.c=.o)

# Rules for building
all: $(TARGET) $(DT_DESC)

$(TARGET): $(OBJS)
	$(CC) $(LINK) -nostdlib -o $(TARGET) $(OBJS) $(LIBS)

init.o: $(TARGET)_rev.h

$(DT_DESC): $(DT_DESC).dt
	$(MAKEDT) $(DT_DESC).dt $(DT_DESC)

.PHONY: install
install: $(TARGET) $(DT_DESC)
	$(CP) $(TARGET) SYS:Classes/DataTypes/
	$(CP) $(DT_DESC) DEVS:DataTypes/

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS)

.PHONY: distclean
distclean:
	$(RM) $(OBJS)

.PHONY: revision
revision:
	bumprev $(VERSION) $(TARGET)
