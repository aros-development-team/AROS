SYSTEM := $(shell uname -s)
CC := vc +aos68k -c99
AS := vasmm68k_mot
RM := rm -f
CP := cp -p --remove-destination

OPTIMIZE := -O3 -size -cpu=68020 -maxoptpasses=20
CFLAGS := $(OPTIMIZE) -Iinclude -Iinclude/amigaos -Idms -DCATCOMP_NUMBERS -DMIN_OS_VERSION=39
DEVICE_FLAGS := -Idevice/include -DDEVICE
PLUGIN_FLAGS := -D__NOLIBBASE__
GUI_FLAGS := -Ira_gui/include -DGUI -DALL_REACTION_CLASSES -DALL_REACTION_MACROS -DFALLBACK_IMAGES
ASFLAGS := -quiet -Fhunk -m68020
LINK := $(OPTIMIZE) -final -Llib
LIBS := -ldos64 -lamiga -lvc
GUI_LIBS := -lamiga -lauto

TARGET  := diskimage.device
VERSION := 52

SFD := sfd/diskimage_lib.sfd sfd/z_lib.sfd sfd/bz2_lib.sfd
PROTO := $(patsubst sfd/%_lib.sfd,include/amigaos/proto/%.h,$(SFD))
CLIB := $(patsubst sfd/%_lib.sfd,include/amigaos/clib/%_protos.h,$(SFD))
INLINE := $(patsubst sfd/%_lib.sfd,include/amigaos/inline/%_protos.h,$(SFD))
INCLUDE := $(PROTO) $(CLIB) $(INLINE)

SUPPORT_OBJS := support/amigaos/allocvecpooled.o support/amigaos/asprintf.o \
	support/amigaos/asprintfpooled.o support/checkbptr.o support/checklib.o \
	support/copystringbstrtoc.o support/deletelibrary.o support/dos64.o \
	support/envvar.o support/getattrs.o support/getcurrentdir.o support/hooks.o \
	support/lists.o support/messages.o support/mutexes.o support/paths.o support/ports.o \
	support/semaphores.o support/strlcpy.o support/m68k/swab2.o support/tooltypes.o \
	support/istext.o support/trimstr.o support/diskimagedevice.o support/localeinfo.o \
	support/translatefuncs.o support/reallocbuf.o support/setprocwindow.o
PREFS_OBJS := prefs/prefs.o prefs/readprefs.o prefs/writeprefs.o
DEVICE_OBJS := device/stub_m68k.o device/init.o device/io.o device/unit.o device/scsicmd.o device/locale.o \
	device/plugins.o device/tempfile.o device/progress.o device/password.o device/main_vectors.o \
	device/plugin_vectors.o plugins/generic.o plugins/adf.o plugins/d64.o plugins/iso.o
PLUGIN_OBJS := $(patsubst %.c,%.o,$(wildcard plugins/*.c) $(wildcard plugins/cue/*.c) \
	$(wildcard plugins/dmg/*.c) $(wildcard plugins/fdi/*.c))
CUE_OBJS := plugins/cue/cue.o audio/aiff.o audio/mp3_mpega.o audio/wave.o
DMS_OBJS := dms/crc_csum.o dms/getbits.o dms/tables.o dms/maketbl.o dms/u_init.o \
	dms/u_deep.o dms/u_heavy.o dms/u_medium.o dms/u_quick.o dms/u_rle.o
GUI_OBJS := ra_gui/locale.o ra_gui/main.o ra_gui/cxbroker.o ra_gui/gui.o ra_gui/snapshot.o \
	ra_gui/images.o ra_gui/requesters.o ra_gui/driveops.o ra_gui/setdevicetype.o \
	ra_gui/extwindowclass.o ra_gui/extscrollerclass.o
RAAUTO_OBJS := $(patsubst %.c,%.o,$(wildcard raauto/*.c))
ALL_OBJS := lib/support.lib lib/libsupport.a support/*.o support/amigaos/*.o \
	support/m68k/*.o lib/prefs.lib lib/libprefs.a prefs/*.o device/*.o plugins/*.o \
	plugins/cue/*.o plugins/dmg/*.o plugins/fdi/*.o dms/*.o audio/*.o cmd/*.o ra_gui/*.o \
	lib/raauto.lib raauto/*.o
PLUGINS := plugins/CCD plugins/CISO plugins/CPC plugins/cue/CUE plugins/DAA plugins/DAX \
	plugins/dmg/DMG plugins/DMS plugins/fdi/FDI plugins/GI plugins/IPF plugins/MDS plugins/NRG \
	plugins/UIF plugins/XAD plugins/XPK
RELEASEDIR := ../release/m68k-amigaos/diskimage_device

# -------------------------------------------------------------

all: $(INCLUDE) lib/support.lib device/$(TARGET) $(PLUGINS) plugins/Crypt z_library bz2_library \
	cmd/MountDiskImage cmd/MountHDF ra_gui/DiskImageGUI png_image

include/amigaos/proto/%.h: sfd/%_lib.sfd
ifeq ($(SYSTEM),AmigaOS)
	fd2pragma $< SPECIAL 38 TO $(dir $@)
else
	fd2pragma -i $< -s 38 -t $(dir $@)
endif

include/amigaos/clib/%_protos.h: sfd/%_lib.sfd
ifeq ($(SYSTEM),AmigaOS)
	fd2pragma $< SPECIAL 111 TO $(dir $@)
else
	fd2pragma -i $< -s 111 -t $(dir $@)
endif

include/amigaos/inline/%_protos.h: sfd/%_lib.sfd
ifeq ($(SYSTEM),AmigaOS)
	fd2pragma $< SPECIAL 70 TO $(dir $@)
else
	fd2pragma -i $< -s 70 -t $(dir $@)
endif

lib/support.lib: $(SUPPORT_OBJS)
	cat $^ >$@

lib/prefs.lib: $(PREFS_OBJS)
	cat $^ >$@

lib/raauto.lib: $(RAAUTO_OBJS)
	cat $^ >$@

$(DEVICE_OBJS): CFLAGS += $(DEVICE_FLAGS)

device/$(TARGET): $(DEVICE_OBJS) lib/prefs.lib lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ $(LIBS)

device/init.o: include/rev/$(TARGET)_rev.h
device/locale.o: include/device_locale.h

include/device_locale.h: device/catalogs/diskimagedevice.cd
ifeq ($(SYSTEM),AmigaOS)
	catcomp $< NOCODE CFILE $@
else
	catcomp $< --cfile $@
endif

$(PLUGIN_OBJS): include/rev/$(TARGET)_rev.h
$(PLUGIN_OBJS): CFLAGS += $(PLUGIN_FLAGS)

plugins/CCD: plugins/stub_m68k.o plugins/ccd.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/CISO: plugins/stub_m68k.o plugins/ciso.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/CPC: plugins/stub_m68k.o plugins/cpc.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/cue/CUE: plugins/stub_m68k.o $(CUE_OBJS) lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/Crypt: plugins/stub_m68k.o plugins/crypt.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/DAA: plugins/stub_m68k.o plugins/daa.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/DAX: plugins/stub_m68k.o plugins/dax.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/dmg/DMG: plugins/stub_m68k.o plugins/dmg/dmg.o plugins/dmg/base64.o plugins/dmg/adc.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/DMS: plugins/stub_m68k.o plugins/dms.o $(DMS_OBJS)
	$(CC) -nostdlib $(LINK) -o $@ $^ -lvc

plugins/fdi/FDI: plugins/stub_m68k.o plugins/fdi/fdi.o plugins/fdi/fdi2raw.o
	$(CC) -nostdlib $(LINK) -o $@ $^ -lmieee -lvc

plugins/GI: plugins/stub_m68k.o plugins/gi.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/IPF: plugins/stub_m68k.o plugins/ipf.o
	$(CC) -nostdlib $(LINK) -o $@ $^ -lvc

plugins/MDS: plugins/stub_m68k.o plugins/mds.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/NRG: plugins/stub_m68k.o plugins/nrg.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/UIF: plugins/stub_m68k.o plugins/uif.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/XAD: plugins/stub_m68k.o plugins/xad.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

plugins/XPK: plugins/stub_m68k.o plugins/xpk.o lib/support.lib
	$(CC) -nostdlib $(LINK) -o $@ $^ -ldos64 -lvc

cmd/MountDiskImage: cmd/mountdiskimage.o lib/support.lib
	$(CC) $(LINK) -o $@ $^ -lamiga -lauto

cmd/mountdiskimage.o: include/rev/MountDiskImage_rev.h

cmd/MountHDF: cmd/mounthdf.o lib/support.lib
	$(CC) $(LINK) -o $@ $^ -lamiga -lauto

cmd/mounthdf.o: include/rev/MountHDF_rev.h

$(GUI_OBJS): CFLAGS += $(GUI_FLAGS)

ra_gui/DiskImageGUI: $(GUI_OBJS) lib/prefs.lib lib/support.lib lib/raauto.lib
	$(CC) $(LINK) -o $@ $^ $(GUI_LIBS)

ra_gui/main.o ra_gui/requesters.o: include/rev/DiskImageGUI_rev.h
ra_gui/locale.o: ra_gui/include/locale.h

ra_gui/include/locale.h: ra_gui/catalogs/diskimagegui.cd
ifeq ($(SYSTEM),AmigaOS)
	catcomp $< NOCODE CFILE $@
else
	catcomp $< --cfile $@
endif

.PHONY: z_library bz2_library expat_library png_image

z_library bz2_library expat_library png_image:
	$(MAKE) -C $@

.PHONY: dist clean revision

dist: all
	$(CP) device/$(TARGET) $(RELEASEDIR)/Devs/
	$(CP) $(PLUGINS) $(RELEASEDIR)/Devs/DiskImage/
	$(CP) z_library/z.library $(RELEASEDIR)/Libs/
	$(CP) bz2_library/bz2.library $(RELEASEDIR)/Libs/
	$(CP) cmd/MountDiskImage cmd/MountHDF $(RELEASEDIR)/C/
	$(CP) ra_gui/DiskImageGUI $(RELEASEDIR)/
	$(CP) png_image/png.image $(RELEASEDIR)/Classes/Images/
	$(CP) device/catalogs/*.cd $(RELEASEDIR)/Catalogs/
	$(CP) ra_gui/catalogs/*.cd $(RELEASEDIR)/Catalogs/

clean:
	$(RM) $(ALL_OBJS)
	$(MAKE) -C png_image clean
	$(MAKE) -C z_library clean
	$(MAKE) -C bz2_library clean

revision:
	bumprev -n $(TARGET) $(VERSION) include/rev/$(TARGET)
	bumprev -n MountDiskImage $(VERSION) include/rev/MountDiskImage
	bumprev -n MountHDF $(VERSION) include/rev/MountHDF
	bumprev -n DiskImageGUI $(VERSION) include/rev/DiskImageGUI
