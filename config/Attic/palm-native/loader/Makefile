## Makefile for LCD test
TOP = ../../..
include $(TOP)/config/host.cfg
TARGET_ARCH = 

TARGET = loader
APPNAME = "AROS loader"
APPID = "AROS"

OBJS = $(TARGET).o util.o

CC = m68k-palmos-coff-gcc
AS = m68k-palmos-coff-as

INC = -I$(TOP)/bin/palm-m68k/AROS/Include -I.


CFLAGS = -Wall -g -O2 $(INC) $(USRINC)

PILRC = pilrc
OBJRES = m68k-palmos-coff-obj-res
NM = m68k-palmos-coff-nm
BUILDPRC = build-prc
PILOTXFER = pilot-xfer

all: $(TARGET).prc

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) AA -I. $<

#%.o : %.s
#	$(AS) -g -I. $^ -o $@

$(TARGET).prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin.res
	$(BUILDPRC) $(TARGET).prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

code0000.$(TARGET).grc: $(TARGET)
	$(OBJRES) $(TARGET)

code0001.$(TARGET).grc: code0000.$(TARGET).grc

data0000.$(TARGET).grc: code0000.$(TARGET).grc

pref0000.$(TARGET).grc: code0000.$(TARGET).grc

rloc0000.$(TARGET).grc: code0000.$(TARGET).grc

bin.res: $(TARGET).rcp $(TARGET).pbm
	$(PILRC) $(TARGET).rcp .
	touch bin.res

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)
	! $(NM) -u $(TARGET) | grep .

send: $(TARGET).prc
	$(PILOTXFER) -p /dev/ttyS0 -i $(TARGET).prc

gdbpanel: $(TARGET).prc
	$(PILOTXFER) -p /dev/ttyS0 -i /home/palm/gdbpanel/gdbpanel.prc

emu:  $(TARGET).prc
	$(PILOTXFER) -p /dev/ttyqe -i $(TARGET).prc
	$(PILOTXFER) -p /dev/ttyqe -i /home/palm/gdbpanel/gdbpanel.prc

depend:
	makedepend -Y -I. *.c

clean:
	-rm -f *.[oa] $(TARGET) *.bin bin.res *.grc Makefile.bak *.prc

veryclean: clean
	-rm -f $(TARGET).prc pilot.ram pilot.scratch

