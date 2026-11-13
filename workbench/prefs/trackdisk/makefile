CFLAGS = -O2 -noixemul -nostartfiles -I../common

all: Trackdisk

Trackdisk: trackdisk_prefs.c trackdisk_prefs.h ../common/prefs_common.h
	gcc $(CFLAGS) trackdisk_prefs.c -o Trackdisk -labox
	strip --remove-section=.comment Trackdisk

clean:
	rm -f trackdisk

install: trackdisk
	Copy Trackdisk Trackdisk.info SYS:Prefs

