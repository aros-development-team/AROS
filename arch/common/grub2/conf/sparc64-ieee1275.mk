
# -*- makefile -*-

COMMON_ASFLAGS = -nostdinc
COMMON_CFLAGS = -ggdb -ffreestanding -m64 -mcpu=v9 -mtune=ultrasparc

# Images.

MOSTLYCLEANFILES += grubof_symlist.c kernel_syms.lst
DEFSYMFILES += kernel_syms.lst

grubof_HEADERS = arg.h boot.h device.h disk.h dl.h elf.h env.h err.h \
	file.h fs.h kernel.h misc.h mm.h net.h rescue.h symbol.h \
	term.h types.h loader.h \
	partition.h pc_partition.h ieee1275/ieee1275.h machine/time.h

grubof_symlist.c: $(addprefix include/grub/,$(grubof_HEADERS)) gensymlist.sh
	sh $(srcdir)/gensymlist.sh $(filter %.h,$^) > $@

kernel_syms.lst: $(addprefix include/grub/,$(grubof_HEADERS)) genkernsyms.sh
	sh $(srcdir)/genkernsyms.sh $(filter %h,$^) > $@

# Programs
pkgdata_PROGRAMS = grubof

# Utilities.
#bin_UTILITIES = grub-emu grub-mkimage
noinst_UTILITIES = genmoddep

# For grub-mkimage.
grub_mkimage_SOURCES = util/sparc64/ieee1275/grub-mkimage.c util/misc.c \
        util/resolve.c 

# For grub-emu
#grub_emu_SOURCES = commands/boot.c commands/cat.c commands/cmp.c 	\
#	commands/configfile.c commands/default.c commands/help.c	\
#	commands/search.c commands/terminal.c commands/ls.c		\
#	commands/timeout.c						\
#	commands/ieee1275/halt.c commands/ieee1275/reboot.c		\
#	disk/loopback.c							\
#	fs/ext2.c fs/fat.c fs/fshelp.c fs/hfs.c fs/iso9660.c fs/jfs.c	\
#	fs/minix.c fs/ufs.c						\
#	kern/device.c kern/disk.c kern/dl.c kern/env.c kern/err.c 	\
#	kern/file.c kern/fs.c kern/loader.c kern/main.c kern/misc.c	\
#	kern/partition.c kern/rescue.c kern/term.c			\
#	normal/arg.c normal/cmdline.c normal/command.c 			\
#	normal/completion.c normal/context.c	\
#	normal/main.c normal/menu.c normal/menu_entry.c	normal/misc.c	\
#	partmap/amiga.c	partmap/apple.c partmap/pc.c partmap/sun.c	\
#	util/console.c util/grub-emu.c util/misc.c			\
#	util/i386/pc/biosdisk.c util/i386/pc/getroot.c			\
#	util/sparc64/ieee1275/misc.c

#grub_emu_LDFLAGS = $(LIBCURSES)

grubof_SOURCES = kern/sparc64/ieee1275/init.c kern/ieee1275/ieee1275.c \
	kern/main.c kern/device.c kern/disk.c kern/dl.c kern/file.c \
	kern/fs.c kern/err.c kern/misc.c kern/mm.c kern/loader.c \
	kern/rescue.c kern/term.c term/ieee1275/ofconsole.c \
	kern/sparc64/ieee1275/openfw.c disk/ieee1275/ofdisk.c \
	kern/partition.c kern/env.c kern/sparc64/dl.c grubof_symlist.c \
	kern/sparc64/cache.c
CLEANFILES += grubof grubof-kern_sparc64_ieee1275_init.o grubof-kern_ieee1275_ieee1275.o grubof-kern_main.o grubof-kern_device.o grubof-kern_disk.o grubof-kern_dl.o grubof-kern_file.o grubof-kern_fs.o grubof-kern_err.o grubof-kern_misc.o grubof-kern_mm.o grubof-kern_loader.o grubof-kern_rescue.o grubof-kern_term.o grubof-term_ieee1275_ofconsole.o grubof-kern_sparc64_ieee1275_openfw.o grubof-disk_ieee1275_ofdisk.o grubof-kern_partition.o grubof-kern_env.o grubof-kern_sparc64_dl.o grubof-grubof_symlist.o grubof-kern_sparc64_cache.o
MOSTLYCLEANFILES += grubof-kern_sparc64_ieee1275_init.d grubof-kern_ieee1275_ieee1275.d grubof-kern_main.d grubof-kern_device.d grubof-kern_disk.d grubof-kern_dl.d grubof-kern_file.d grubof-kern_fs.d grubof-kern_err.d grubof-kern_misc.d grubof-kern_mm.d grubof-kern_loader.d grubof-kern_rescue.d grubof-kern_term.d grubof-term_ieee1275_ofconsole.d grubof-kern_sparc64_ieee1275_openfw.d grubof-disk_ieee1275_ofdisk.d grubof-kern_partition.d grubof-kern_env.d grubof-kern_sparc64_dl.d grubof-grubof_symlist.d grubof-kern_sparc64_cache.d

grubof: grubof-kern_sparc64_ieee1275_init.o grubof-kern_ieee1275_ieee1275.o grubof-kern_main.o grubof-kern_device.o grubof-kern_disk.o grubof-kern_dl.o grubof-kern_file.o grubof-kern_fs.o grubof-kern_err.o grubof-kern_misc.o grubof-kern_mm.o grubof-kern_loader.o grubof-kern_rescue.o grubof-kern_term.o grubof-term_ieee1275_ofconsole.o grubof-kern_sparc64_ieee1275_openfw.o grubof-disk_ieee1275_ofdisk.o grubof-kern_partition.o grubof-kern_env.o grubof-kern_sparc64_dl.o grubof-grubof_symlist.o grubof-kern_sparc64_cache.o
	$(CC) -o $@ $^ $(LDFLAGS) $(grubof_LDFLAGS)

grubof-kern_sparc64_ieee1275_init.o: kern/sparc64/ieee1275/init.c
	$(CC) -Ikern/sparc64/ieee1275 -I$(srcdir)/kern/sparc64/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_sparc64_ieee1275_init.d: kern/sparc64/ieee1275/init.c
	set -e; 	  $(CC) -Ikern/sparc64/ieee1275 -I$(srcdir)/kern/sparc64/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,init\.o[ :]*,grubof-kern_sparc64_ieee1275_init.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_sparc64_ieee1275_init.d

grubof-kern_ieee1275_ieee1275.o: kern/ieee1275/ieee1275.c
	$(CC) -Ikern/ieee1275 -I$(srcdir)/kern/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_ieee1275_ieee1275.d: kern/ieee1275/ieee1275.c
	set -e; 	  $(CC) -Ikern/ieee1275 -I$(srcdir)/kern/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,ieee1275\.o[ :]*,grubof-kern_ieee1275_ieee1275.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_ieee1275_ieee1275.d

grubof-kern_main.o: kern/main.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_main.d: kern/main.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,grubof-kern_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_main.d

grubof-kern_device.o: kern/device.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_device.d: kern/device.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,device\.o[ :]*,grubof-kern_device.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_device.d

grubof-kern_disk.o: kern/disk.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_disk.d: kern/disk.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,disk\.o[ :]*,grubof-kern_disk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_disk.d

grubof-kern_dl.o: kern/dl.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_dl.d: kern/dl.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,grubof-kern_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_dl.d

grubof-kern_file.o: kern/file.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_file.d: kern/file.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,file\.o[ :]*,grubof-kern_file.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_file.d

grubof-kern_fs.o: kern/fs.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_fs.d: kern/fs.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,fs\.o[ :]*,grubof-kern_fs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_fs.d

grubof-kern_err.o: kern/err.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_err.d: kern/err.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,grubof-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_err.d

grubof-kern_misc.o: kern/misc.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_misc.d: kern/misc.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grubof-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_misc.d

grubof-kern_mm.o: kern/mm.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_mm.d: kern/mm.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,mm\.o[ :]*,grubof-kern_mm.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_mm.d

grubof-kern_loader.o: kern/loader.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_loader.d: kern/loader.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,loader\.o[ :]*,grubof-kern_loader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_loader.d

grubof-kern_rescue.o: kern/rescue.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_rescue.d: kern/rescue.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,rescue\.o[ :]*,grubof-kern_rescue.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_rescue.d

grubof-kern_term.o: kern/term.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_term.d: kern/term.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,term\.o[ :]*,grubof-kern_term.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_term.d

grubof-term_ieee1275_ofconsole.o: term/ieee1275/ofconsole.c
	$(CC) -Iterm/ieee1275 -I$(srcdir)/term/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-term_ieee1275_ofconsole.d: term/ieee1275/ofconsole.c
	set -e; 	  $(CC) -Iterm/ieee1275 -I$(srcdir)/term/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,ofconsole\.o[ :]*,grubof-term_ieee1275_ofconsole.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-term_ieee1275_ofconsole.d

grubof-kern_sparc64_ieee1275_openfw.o: kern/sparc64/ieee1275/openfw.c
	$(CC) -Ikern/sparc64/ieee1275 -I$(srcdir)/kern/sparc64/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_sparc64_ieee1275_openfw.d: kern/sparc64/ieee1275/openfw.c
	set -e; 	  $(CC) -Ikern/sparc64/ieee1275 -I$(srcdir)/kern/sparc64/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,openfw\.o[ :]*,grubof-kern_sparc64_ieee1275_openfw.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_sparc64_ieee1275_openfw.d

grubof-disk_ieee1275_ofdisk.o: disk/ieee1275/ofdisk.c
	$(CC) -Idisk/ieee1275 -I$(srcdir)/disk/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-disk_ieee1275_ofdisk.d: disk/ieee1275/ofdisk.c
	set -e; 	  $(CC) -Idisk/ieee1275 -I$(srcdir)/disk/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,ofdisk\.o[ :]*,grubof-disk_ieee1275_ofdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-disk_ieee1275_ofdisk.d

grubof-kern_partition.o: kern/partition.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_partition.d: kern/partition.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,grubof-kern_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_partition.d

grubof-kern_env.o: kern/env.c
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_env.d: kern/env.c
	set -e; 	  $(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,env\.o[ :]*,grubof-kern_env.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_env.d

grubof-kern_sparc64_dl.o: kern/sparc64/dl.c
	$(CC) -Ikern/sparc64 -I$(srcdir)/kern/sparc64 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_sparc64_dl.d: kern/sparc64/dl.c
	set -e; 	  $(CC) -Ikern/sparc64 -I$(srcdir)/kern/sparc64 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,grubof-kern_sparc64_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_sparc64_dl.d

grubof-grubof_symlist.o: grubof_symlist.c
	$(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-grubof_symlist.d: grubof_symlist.c
	set -e; 	  $(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,grubof_symlist\.o[ :]*,grubof-grubof_symlist.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-grubof_symlist.d

grubof-kern_sparc64_cache.o: kern/sparc64/cache.c
	$(CC) -Ikern/sparc64 -I$(srcdir)/kern/sparc64 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_sparc64_cache.d: kern/sparc64/cache.c
	set -e; 	  $(CC) -Ikern/sparc64 -I$(srcdir)/kern/sparc64 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,cache\.o[ :]*,grubof-kern_sparc64_cache.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_sparc64_cache.d

grubof_HEADERS = grub/sparc64/ieee1275/ieee1275.h
grubof_CFLAGS = $(COMMON_CFLAGS)
grubof_ASFLAGS = $(COMMON_ASFLAGS)
grubof_LDFLAGS = -m64 -nostdlib -Wl,-N,-Ttext,0x200000,-Bstatic -Xlinker --oformat -Xlinker elf64-sparc

# For genmoddep.
genmoddep_SOURCES = util/genmoddep.c
CLEANFILES += genmoddep genmoddep-util_genmoddep.o
MOSTLYCLEANFILES += genmoddep-util_genmoddep.d

genmoddep: genmoddep-util_genmoddep.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(genmoddep_LDFLAGS)

genmoddep-util_genmoddep.o: util/genmoddep.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(genmoddep_CFLAGS) -c -o $@ $<

genmoddep-util_genmoddep.d: util/genmoddep.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(genmoddep_CFLAGS) -M $< 	  | sed 's,genmoddep\.o[ :]*,genmoddep-util_genmoddep.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include genmoddep-util_genmoddep.d


# Modules.
#pkgdata_MODULES = _linux.mod linux.mod fat.mod ufs.mod ext2.mod minix.mod \
#	hfs.mod jfs.mod normal.mod hello.mod font.mod ls.mod \
#	boot.mod cmp.mod cat.mod terminal.mod fshelp.mod amiga.mod apple.mod \
#	pc.mod suspend.mod loopback.mod help.mod reboot.mod halt.mod sun.mod \
#	default.mod timeout.mod configfile.mod search.mod

# For fshelp.mod.
fshelp_mod_SOURCES = fs/fshelp.c
fshelp_mod_CFLAGS = $(COMMON_CFLAGS)

# For fat.mod.
fat_mod_SOURCES = fs/fat.c
fat_mod_CFLAGS = $(COMMON_CFLAGS)

# For ext2.mod.
ext2_mod_SOURCES = fs/ext2.c
ext2_mod_CFLAGS = $(COMMON_CFLAGS)

# For ufs.mod.
ufs_mod_SOURCES = fs/ufs.c
ufs_mod_CFLAGS = $(COMMON_CFLAGS)

# For minix.mod.
minix_mod_SOURCES = fs/minix.c
minix_mod_CFLAGS = $(COMMON_CFLAGS)

# For hfs.mod.
hfs_mod_SOURCES = fs/hfs.c
hfs_mod_CFLAGS = $(COMMON_CFLAGS)

# For jfs.mod.
jfs_mod_SOURCES = fs/jfs.c
jfs_mod_CFLAGS = $(COMMON_CFLAGS)

# For iso9660.mod.
iso9660_mod_SOURCES = fs/iso9660.c
iso9660_mod_CFLAGS = $(COMMON_CFLAGS)

# For _linux.mod.
_linux_mod_SOURCES = loader/sparc64/ieee1275/linux.c
_linux_mod_CFLAGS = $(COMMON_CFLAGS)
 
# For linux.mod.
linux_mod_SOURCES = loader/sparc64/ieee1275/linux_normal.c
linux_mod_CFLAGS = $(COMMON_CFLAGS)

# For normal.mod.
normal_mod_SOURCES = normal/arg.c normal/cmdline.c normal/command.c	\
	normal/context.c normal/main.c normal/menu.c			\
	normal/menu_entry.c						\
	normal/sparc64/setjmp.c
normal_mod_CFLAGS = $(COMMON_CFLAGS)
normal_mod_ASFLAGS = $(COMMON_ASFLAGS)

# For hello.mod.
hello_mod_SOURCES = hello/hello.c
hello_mod_CFLAGS = $(COMMON_CFLAGS)

# For boot.mod.
boot_mod_SOURCES = commands/boot.c
boot_mod_CFLAGS = $(COMMON_CFLAGS)

# For terminal.mod.
terminal_mod_SOURCES = commands/terminal.c
terminal_mod_CFLAGS = $(COMMON_CFLAGS)

# For ls.mod.
ls_mod_SOURCES = commands/ls.c
ls_mod_CFLAGS = $(COMMON_CFLAGS)

# For cmp.mod.
cmp_mod_SOURCES = commands/cmp.c
cmp_mod_CFLAGS = $(COMMON_CFLAGS)

# For cat.mod.
cat_mod_SOURCES = commands/cat.c
cat_mod_CFLAGS = $(COMMON_CFLAGS)

# For font.mod.
font_mod_SOURCES = font/manager.c
font_mod_CFLAGS = $(COMMON_CFLAGS)

# For amiga.mod
amiga_mod_SOURCES = partmap/amiga.c
amiga_mod_CFLAGS = $(COMMON_CFLAGS)

# For apple.mod
apple_mod_SOURCES = partmap/apple.c
apple_mod_CFLAGS = $(COMMON_CFLAGS)

# For pc.mod
pc_mod_SOURCES = partmap/pc.c
pc_mod_CFLAGS = $(COMMON_CFLAGS)

# For sun.mod
sun_mod_SOURCES = partmap/sun.c
sun_mod_CFLAGS = $(COMMON_CFLAGS)

# For loopback.mod
loopback_mod_SOURCES = disk/loopback.c
loopback_mod_CFLAGS = $(COMMON_CFLAGS)

# For suspend.mod
suspend_mod_SOURCES = commands/ieee1275/suspend.c
suspend_mod_CFLAGS = $(COMMON_CFLAGS)

# For reboot.mod
reboot_mod_SOURCES = commands/ieee1275/reboot.c
reboot_mod_CFLAGS = $(COMMON_CFLAGS)

# For halt.mod
halt_mod_SOURCES = commands/ieee1275/halt.c
halt_mod_CFLAGS = $(COMMON_CFLAGS)

# For help.mod.
help_mod_SOURCES = commands/help.c
help_mod_CFLAGS = $(COMMON_CFLAGS)

# For default.mod
default_mod_SOURCES = commands/default.c
default_mod_CFLAGS =  $(COMMON_CFLAGS)

# For timeout.mod
timeout_mod_SOURCES = commands/timeout.c
timeout_mod_CFLAGS =  $(COMMON_CFLAGS)

# For configfile.mod
configfile_mod_SOURCES = commands/configfile.c
configfile_mod_CFLAGS = $(COMMON_CFLAGS)

# For search.mod.
search_mod_SOURCES = commands/search.c
search_mod_CFLAGS = $(COMMON_CFLAGS)
CLEANFILES += moddep.lst command.lst fs.lst
pkgdata_DATA += moddep.lst command.lst fs.lst
moddep.lst: $(DEFSYMFILES) $(UNDSYMFILES) genmoddep
	cat $(DEFSYMFILES) /dev/null | ./genmoddep $(UNDSYMFILES) > $@ \
	  || (rm -f $@; exit 1)

command.lst: $(COMMANDFILES)
	cat $^ /dev/null | sort > $@

fs.lst: $(FSFILES)
	cat $^ /dev/null | sort > $@
