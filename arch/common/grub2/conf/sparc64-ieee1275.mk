
# -*- makefile -*-

COMMON_ASFLAGS = -nostdinc
COMMON_CFLAGS = -ggdb -ffreestanding -m64 -mno-app-regs
COMMON_LDFLAGS = -melf64_sparc -nostdlib

# Used by various components.  These rules need to precede them.
normal/execute.c_DEPENDENCIES = grub_script.tab.h
normal/command.c_DEPENDENCIES = grub_script.tab.h
normal/function.c_DEPENDENCIES = grub_script.tab.h
normal/lexer.c_DEPENDENCIES = grub_script.tab.h

# Images.

MOSTLYCLEANFILES += kernel_elf_symlist.c kernel_syms.lst
DEFSYMFILES += kernel_syms.lst

kernel_elf_HEADERS = arg.h boot.h cache.h device.h disk.h dl.h elf.h elfload.h \
	env.h err.h file.h fs.h kernel.h misc.h mm.h net.h parser.h rescue.h \
	symbol.h term.h time.h types.h sparc64/libgcc.h loader.h partition.h \
	pc_partition.h ieee1275/ieee1275.h machine/kernel.h

kernel_elf_symlist.c: $(addprefix include/grub/,$(kernel_elf_HEADERS)) config.h gensymlist.sh
	/bin/sh gensymlist.sh $(filter %.h,$^) > $@ || (rm -f $@; exit 1)

# For the parser.
grub_script.tab.c grub_script.tab.h: normal/parser.y
	$(YACC) -d -p grub_script_yy -b grub_script $(srcdir)/normal/parser.y

kernel_syms.lst: $(addprefix include/grub/,$(kernel_elf_HEADERS)) config.h genkernsyms.sh
	/bin/sh genkernsyms.sh $(filter %.h,$^) > $@ || (rm -f $@; exit 1)

# Programs
pkglib_PROGRAMS = kernel.elf

# Utilities.
#bin_UTILITIES = grub-mkimage
#ifeq ($(enable_grub_emu), yes)
#bin_UTILITIES += grub-emu
#endif

# For grub-mkimage.
grub_mkimage_SOURCES = util/sparc64/ieee1275/grub-mkimage.c util/misc.c \
        util/resolve.c 

# For grub-emu
#grub_emu_SOURCES = commands/boot.c commands/cat.c commands/cmp.c 	\
#	commands/configfile.c commands/default.c commands/help.c	\
#	commands/search.c commands/terminal.c commands/ls.c		\
#	commands/timeout.c commands/test.c				\
#	commands/halt.c commands/reboot.c		\
#	disk/loopback.c							\
#	fs/affs.c fs/ext2.c fs/fat.c fs/fshelp.c fs/hfs.c fs/iso9660.c  \
#	fs/jfs.c fs/minix.c fs/sfs.c fs/ufs.c fs/xfs.c                  \
#	grub_script.tab.c						\
#	io/gzio.c                                                       \
#	kern/device.c kern/disk.c kern/dl.c kern/env.c kern/err.c 	\
#	kern/file.c kern/fs.c kern/loader.c kern/main.c kern/misc.c	\
#	kern/parser.c kern/partition.c kern/rescue.c kern/term.c	\
#	normal/arg.c normal/cmdline.c normal/command.c			\
#	normal/completion.c normal/context.c normal/execute.c		\
#	normal/function.c normal/lexer.c				\
#	normal/main.c normal/menu.c normal/menu_entry.c	normal/misc.c	\
#	partmap/amiga.c	partmap/apple.c partmap/pc.c partmap/sun.c	\
#	partmap/acorn.c							\
#	util/console.c util/grub-emu.c util/misc.c			\
#	util/biosdisk.c util/getroot.c			\
#	util/sparc64/ieee1275/misc.c

grub_emu_LDFLAGS = $(LIBCURSES)

kernel_elf_SOURCES = kern/sparc64/ieee1275/init.c kern/ieee1275/ieee1275.c \
	kern/main.c kern/device.c kern/disk.c kern/dl.c kern/file.c \
	kern/fs.c kern/err.c kern/misc.c kern/mm.c kern/loader.c \
	kern/rescue.c kern/term.c term/ieee1275/ofconsole.c \
	kern/sparc64/ieee1275/openfw.c disk/ieee1275/ofdisk.c \
	kern/partition.c kern/env.c kern/sparc64/dl.c kernel_elf_symlist.c \
	kern/sparc64/cache.S kern/parser.c
CLEANFILES += kernel.elf kernel_elf-kern_sparc64_ieee1275_init.o kernel_elf-kern_ieee1275_ieee1275.o kernel_elf-kern_main.o kernel_elf-kern_device.o kernel_elf-kern_disk.o kernel_elf-kern_dl.o kernel_elf-kern_file.o kernel_elf-kern_fs.o kernel_elf-kern_err.o kernel_elf-kern_misc.o kernel_elf-kern_mm.o kernel_elf-kern_loader.o kernel_elf-kern_rescue.o kernel_elf-kern_term.o kernel_elf-term_ieee1275_ofconsole.o kernel_elf-kern_sparc64_ieee1275_openfw.o kernel_elf-disk_ieee1275_ofdisk.o kernel_elf-kern_partition.o kernel_elf-kern_env.o kernel_elf-kern_sparc64_dl.o kernel_elf-kernel_elf_symlist.o kernel_elf-kern_sparc64_cache.o kernel_elf-kern_parser.o
MOSTLYCLEANFILES += kernel_elf-kern_sparc64_ieee1275_init.d kernel_elf-kern_ieee1275_ieee1275.d kernel_elf-kern_main.d kernel_elf-kern_device.d kernel_elf-kern_disk.d kernel_elf-kern_dl.d kernel_elf-kern_file.d kernel_elf-kern_fs.d kernel_elf-kern_err.d kernel_elf-kern_misc.d kernel_elf-kern_mm.d kernel_elf-kern_loader.d kernel_elf-kern_rescue.d kernel_elf-kern_term.d kernel_elf-term_ieee1275_ofconsole.d kernel_elf-kern_sparc64_ieee1275_openfw.d kernel_elf-disk_ieee1275_ofdisk.d kernel_elf-kern_partition.d kernel_elf-kern_env.d kernel_elf-kern_sparc64_dl.d kernel_elf-kernel_elf_symlist.d kernel_elf-kern_sparc64_cache.d kernel_elf-kern_parser.d

kernel.elf: $(kernel_elf_DEPENDENCIES) kernel_elf-kern_sparc64_ieee1275_init.o kernel_elf-kern_ieee1275_ieee1275.o kernel_elf-kern_main.o kernel_elf-kern_device.o kernel_elf-kern_disk.o kernel_elf-kern_dl.o kernel_elf-kern_file.o kernel_elf-kern_fs.o kernel_elf-kern_err.o kernel_elf-kern_misc.o kernel_elf-kern_mm.o kernel_elf-kern_loader.o kernel_elf-kern_rescue.o kernel_elf-kern_term.o kernel_elf-term_ieee1275_ofconsole.o kernel_elf-kern_sparc64_ieee1275_openfw.o kernel_elf-disk_ieee1275_ofdisk.o kernel_elf-kern_partition.o kernel_elf-kern_env.o kernel_elf-kern_sparc64_dl.o kernel_elf-kernel_elf_symlist.o kernel_elf-kern_sparc64_cache.o kernel_elf-kern_parser.o
	$(TARGET_CC) -o $@ kernel_elf-kern_sparc64_ieee1275_init.o kernel_elf-kern_ieee1275_ieee1275.o kernel_elf-kern_main.o kernel_elf-kern_device.o kernel_elf-kern_disk.o kernel_elf-kern_dl.o kernel_elf-kern_file.o kernel_elf-kern_fs.o kernel_elf-kern_err.o kernel_elf-kern_misc.o kernel_elf-kern_mm.o kernel_elf-kern_loader.o kernel_elf-kern_rescue.o kernel_elf-kern_term.o kernel_elf-term_ieee1275_ofconsole.o kernel_elf-kern_sparc64_ieee1275_openfw.o kernel_elf-disk_ieee1275_ofdisk.o kernel_elf-kern_partition.o kernel_elf-kern_env.o kernel_elf-kern_sparc64_dl.o kernel_elf-kernel_elf_symlist.o kernel_elf-kern_sparc64_cache.o kernel_elf-kern_parser.o $(TARGET_LDFLAGS) $(kernel_elf_LDFLAGS)

kernel_elf-kern_sparc64_ieee1275_init.o: kern/sparc64/ieee1275/init.c $(kern/sparc64/ieee1275/init.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern/sparc64/ieee1275 -I$(srcdir)/kern/sparc64/ieee1275 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_sparc64_ieee1275_init.d

kernel_elf-kern_ieee1275_ieee1275.o: kern/ieee1275/ieee1275.c $(kern/ieee1275/ieee1275.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern/ieee1275 -I$(srcdir)/kern/ieee1275 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_ieee1275_ieee1275.d

kernel_elf-kern_main.o: kern/main.c $(kern/main.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_main.d

kernel_elf-kern_device.o: kern/device.c $(kern/device.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_device.d

kernel_elf-kern_disk.o: kern/disk.c $(kern/disk.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_disk.d

kernel_elf-kern_dl.o: kern/dl.c $(kern/dl.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_dl.d

kernel_elf-kern_file.o: kern/file.c $(kern/file.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_file.d

kernel_elf-kern_fs.o: kern/fs.c $(kern/fs.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_fs.d

kernel_elf-kern_err.o: kern/err.c $(kern/err.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_err.d

kernel_elf-kern_misc.o: kern/misc.c $(kern/misc.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_misc.d

kernel_elf-kern_mm.o: kern/mm.c $(kern/mm.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_mm.d

kernel_elf-kern_loader.o: kern/loader.c $(kern/loader.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_loader.d

kernel_elf-kern_rescue.o: kern/rescue.c $(kern/rescue.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_rescue.d

kernel_elf-kern_term.o: kern/term.c $(kern/term.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_term.d

kernel_elf-term_ieee1275_ofconsole.o: term/ieee1275/ofconsole.c $(term/ieee1275/ofconsole.c_DEPENDENCIES)
	$(TARGET_CC) -Iterm/ieee1275 -I$(srcdir)/term/ieee1275 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-term_ieee1275_ofconsole.d

kernel_elf-kern_sparc64_ieee1275_openfw.o: kern/sparc64/ieee1275/openfw.c $(kern/sparc64/ieee1275/openfw.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern/sparc64/ieee1275 -I$(srcdir)/kern/sparc64/ieee1275 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_sparc64_ieee1275_openfw.d

kernel_elf-disk_ieee1275_ofdisk.o: disk/ieee1275/ofdisk.c $(disk/ieee1275/ofdisk.c_DEPENDENCIES)
	$(TARGET_CC) -Idisk/ieee1275 -I$(srcdir)/disk/ieee1275 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-disk_ieee1275_ofdisk.d

kernel_elf-kern_partition.o: kern/partition.c $(kern/partition.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_partition.d

kernel_elf-kern_env.o: kern/env.c $(kern/env.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_env.d

kernel_elf-kern_sparc64_dl.o: kern/sparc64/dl.c $(kern/sparc64/dl.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern/sparc64 -I$(srcdir)/kern/sparc64 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_sparc64_dl.d

kernel_elf-kernel_elf_symlist.o: kernel_elf_symlist.c $(kernel_elf_symlist.c_DEPENDENCIES)
	$(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kernel_elf_symlist.d

kernel_elf-kern_sparc64_cache.o: kern/sparc64/cache.S $(kern/sparc64/cache.S_DEPENDENCIES)
	$(TARGET_CC) -Ikern/sparc64 -I$(srcdir)/kern/sparc64 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_sparc64_cache.d

kernel_elf-kern_parser.o: kern/parser.c $(kern/parser.c_DEPENDENCIES)
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_elf_CFLAGS) -MD -c -o $@ $<
-include kernel_elf-kern_parser.d

kernel_elf_HEADERS = grub/sparc64/ieee1275/ieee1275.h
kernel_elf_CFLAGS = $(COMMON_CFLAGS)
kernel_elf_ASFLAGS = $(COMMON_ASFLAGS)
kernel_elf_LDFLAGS = -mno-app-regs -nostdlib -Wl,-N,-Ttext,0x200000,-Bstatic,-melf64_sparc

# Modules.
#_linux.mod linux.mod
pkglib_MODULES = fat.mod ufs.mod ext2.mod minix.mod \
	hfs.mod jfs.mod normal.mod hello.mod font.mod ls.mod \
	boot.mod cmp.mod cat.mod terminal.mod fshelp.mod amiga.mod apple.mod \
	pc.mod suspend.mod loopback.mod help.mod reboot.mod halt.mod sun.mod \
	configfile.mod search.mod gzio.mod xfs.mod \
	affs.mod sfs.mod acorn.mod

# For fshelp.mod.
fshelp_mod_SOURCES = fs/fshelp.c
CLEANFILES += fshelp.mod mod-fshelp.o mod-fshelp.c pre-fshelp.o fshelp_mod-fs_fshelp.o und-fshelp.lst
ifneq ($(fshelp_mod_EXPORTS),no)
CLEANFILES += def-fshelp.lst
DEFSYMFILES += def-fshelp.lst
endif
MOSTLYCLEANFILES += fshelp_mod-fs_fshelp.d
UNDSYMFILES += und-fshelp.lst

fshelp.mod: pre-fshelp.o mod-fshelp.o
	-rm -f $@
	$(TARGET_CC) $(fshelp_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-fshelp.o: $(fshelp_mod_DEPENDENCIES) fshelp_mod-fs_fshelp.o
	-rm -f $@
	$(TARGET_CC) $(fshelp_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ fshelp_mod-fs_fshelp.o

mod-fshelp.o: mod-fshelp.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(fshelp_mod_CFLAGS) -c -o $@ $<

mod-fshelp.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'fshelp' $< > $@ || (rm -f $@; exit 1)

ifneq ($(fshelp_mod_EXPORTS),no)
def-fshelp.lst: pre-fshelp.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 fshelp/' > $@
endif

und-fshelp.lst: pre-fshelp.o
	echo 'fshelp' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

fshelp_mod-fs_fshelp.o: fs/fshelp.c $(fs/fshelp.c_DEPENDENCIES)
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(fshelp_mod_CFLAGS) -MD -c -o $@ $<
-include fshelp_mod-fs_fshelp.d

CLEANFILES += cmd-fshelp_mod-fs_fshelp.lst fs-fshelp_mod-fs_fshelp.lst
COMMANDFILES += cmd-fshelp_mod-fs_fshelp.lst
FSFILES += fs-fshelp_mod-fs_fshelp.lst

cmd-fshelp_mod-fs_fshelp.lst: fs/fshelp.c $(fs/fshelp.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(fshelp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh fshelp > $@ || (rm -f $@; exit 1)

fs-fshelp_mod-fs_fshelp.lst: fs/fshelp.c $(fs/fshelp.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(fshelp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh fshelp > $@ || (rm -f $@; exit 1)


fshelp_mod_CFLAGS = $(COMMON_CFLAGS)
fshelp_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For fat.mod.
fat_mod_SOURCES = fs/fat.c
CLEANFILES += fat.mod mod-fat.o mod-fat.c pre-fat.o fat_mod-fs_fat.o und-fat.lst
ifneq ($(fat_mod_EXPORTS),no)
CLEANFILES += def-fat.lst
DEFSYMFILES += def-fat.lst
endif
MOSTLYCLEANFILES += fat_mod-fs_fat.d
UNDSYMFILES += und-fat.lst

fat.mod: pre-fat.o mod-fat.o
	-rm -f $@
	$(TARGET_CC) $(fat_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-fat.o: $(fat_mod_DEPENDENCIES) fat_mod-fs_fat.o
	-rm -f $@
	$(TARGET_CC) $(fat_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ fat_mod-fs_fat.o

mod-fat.o: mod-fat.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(fat_mod_CFLAGS) -c -o $@ $<

mod-fat.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'fat' $< > $@ || (rm -f $@; exit 1)

ifneq ($(fat_mod_EXPORTS),no)
def-fat.lst: pre-fat.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 fat/' > $@
endif

und-fat.lst: pre-fat.o
	echo 'fat' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

fat_mod-fs_fat.o: fs/fat.c $(fs/fat.c_DEPENDENCIES)
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(fat_mod_CFLAGS) -MD -c -o $@ $<
-include fat_mod-fs_fat.d

CLEANFILES += cmd-fat_mod-fs_fat.lst fs-fat_mod-fs_fat.lst
COMMANDFILES += cmd-fat_mod-fs_fat.lst
FSFILES += fs-fat_mod-fs_fat.lst

cmd-fat_mod-fs_fat.lst: fs/fat.c $(fs/fat.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(fat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh fat > $@ || (rm -f $@; exit 1)

fs-fat_mod-fs_fat.lst: fs/fat.c $(fs/fat.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(fat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh fat > $@ || (rm -f $@; exit 1)


fat_mod_CFLAGS = $(COMMON_CFLAGS)
fat_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For ext2.mod.
ext2_mod_SOURCES = fs/ext2.c
CLEANFILES += ext2.mod mod-ext2.o mod-ext2.c pre-ext2.o ext2_mod-fs_ext2.o und-ext2.lst
ifneq ($(ext2_mod_EXPORTS),no)
CLEANFILES += def-ext2.lst
DEFSYMFILES += def-ext2.lst
endif
MOSTLYCLEANFILES += ext2_mod-fs_ext2.d
UNDSYMFILES += und-ext2.lst

ext2.mod: pre-ext2.o mod-ext2.o
	-rm -f $@
	$(TARGET_CC) $(ext2_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ext2.o: $(ext2_mod_DEPENDENCIES) ext2_mod-fs_ext2.o
	-rm -f $@
	$(TARGET_CC) $(ext2_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ ext2_mod-fs_ext2.o

mod-ext2.o: mod-ext2.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ext2_mod_CFLAGS) -c -o $@ $<

mod-ext2.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'ext2' $< > $@ || (rm -f $@; exit 1)

ifneq ($(ext2_mod_EXPORTS),no)
def-ext2.lst: pre-ext2.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 ext2/' > $@
endif

und-ext2.lst: pre-ext2.o
	echo 'ext2' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

ext2_mod-fs_ext2.o: fs/ext2.c $(fs/ext2.c_DEPENDENCIES)
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(ext2_mod_CFLAGS) -MD -c -o $@ $<
-include ext2_mod-fs_ext2.d

CLEANFILES += cmd-ext2_mod-fs_ext2.lst fs-ext2_mod-fs_ext2.lst
COMMANDFILES += cmd-ext2_mod-fs_ext2.lst
FSFILES += fs-ext2_mod-fs_ext2.lst

cmd-ext2_mod-fs_ext2.lst: fs/ext2.c $(fs/ext2.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ext2_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ext2 > $@ || (rm -f $@; exit 1)

fs-ext2_mod-fs_ext2.lst: fs/ext2.c $(fs/ext2.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ext2_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ext2 > $@ || (rm -f $@; exit 1)


ext2_mod_CFLAGS = $(COMMON_CFLAGS)
ext2_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For ufs.mod.
ufs_mod_SOURCES = fs/ufs.c
CLEANFILES += ufs.mod mod-ufs.o mod-ufs.c pre-ufs.o ufs_mod-fs_ufs.o und-ufs.lst
ifneq ($(ufs_mod_EXPORTS),no)
CLEANFILES += def-ufs.lst
DEFSYMFILES += def-ufs.lst
endif
MOSTLYCLEANFILES += ufs_mod-fs_ufs.d
UNDSYMFILES += und-ufs.lst

ufs.mod: pre-ufs.o mod-ufs.o
	-rm -f $@
	$(TARGET_CC) $(ufs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ufs.o: $(ufs_mod_DEPENDENCIES) ufs_mod-fs_ufs.o
	-rm -f $@
	$(TARGET_CC) $(ufs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ ufs_mod-fs_ufs.o

mod-ufs.o: mod-ufs.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ufs_mod_CFLAGS) -c -o $@ $<

mod-ufs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'ufs' $< > $@ || (rm -f $@; exit 1)

ifneq ($(ufs_mod_EXPORTS),no)
def-ufs.lst: pre-ufs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 ufs/' > $@
endif

und-ufs.lst: pre-ufs.o
	echo 'ufs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

ufs_mod-fs_ufs.o: fs/ufs.c $(fs/ufs.c_DEPENDENCIES)
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(ufs_mod_CFLAGS) -MD -c -o $@ $<
-include ufs_mod-fs_ufs.d

CLEANFILES += cmd-ufs_mod-fs_ufs.lst fs-ufs_mod-fs_ufs.lst
COMMANDFILES += cmd-ufs_mod-fs_ufs.lst
FSFILES += fs-ufs_mod-fs_ufs.lst

cmd-ufs_mod-fs_ufs.lst: fs/ufs.c $(fs/ufs.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ufs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ufs > $@ || (rm -f $@; exit 1)

fs-ufs_mod-fs_ufs.lst: fs/ufs.c $(fs/ufs.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ufs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ufs > $@ || (rm -f $@; exit 1)


ufs_mod_CFLAGS = $(COMMON_CFLAGS)
ufs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For minix.mod.
minix_mod_SOURCES = fs/minix.c
CLEANFILES += minix.mod mod-minix.o mod-minix.c pre-minix.o minix_mod-fs_minix.o und-minix.lst
ifneq ($(minix_mod_EXPORTS),no)
CLEANFILES += def-minix.lst
DEFSYMFILES += def-minix.lst
endif
MOSTLYCLEANFILES += minix_mod-fs_minix.d
UNDSYMFILES += und-minix.lst

minix.mod: pre-minix.o mod-minix.o
	-rm -f $@
	$(TARGET_CC) $(minix_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-minix.o: $(minix_mod_DEPENDENCIES) minix_mod-fs_minix.o
	-rm -f $@
	$(TARGET_CC) $(minix_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ minix_mod-fs_minix.o

mod-minix.o: mod-minix.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(minix_mod_CFLAGS) -c -o $@ $<

mod-minix.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'minix' $< > $@ || (rm -f $@; exit 1)

ifneq ($(minix_mod_EXPORTS),no)
def-minix.lst: pre-minix.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 minix/' > $@
endif

und-minix.lst: pre-minix.o
	echo 'minix' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

minix_mod-fs_minix.o: fs/minix.c $(fs/minix.c_DEPENDENCIES)
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(minix_mod_CFLAGS) -MD -c -o $@ $<
-include minix_mod-fs_minix.d

CLEANFILES += cmd-minix_mod-fs_minix.lst fs-minix_mod-fs_minix.lst
COMMANDFILES += cmd-minix_mod-fs_minix.lst
FSFILES += fs-minix_mod-fs_minix.lst

cmd-minix_mod-fs_minix.lst: fs/minix.c $(fs/minix.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(minix_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh minix > $@ || (rm -f $@; exit 1)

fs-minix_mod-fs_minix.lst: fs/minix.c $(fs/minix.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(minix_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh minix > $@ || (rm -f $@; exit 1)


minix_mod_CFLAGS = $(COMMON_CFLAGS)
minix_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For hfs.mod.
hfs_mod_SOURCES = fs/hfs.c
CLEANFILES += hfs.mod mod-hfs.o mod-hfs.c pre-hfs.o hfs_mod-fs_hfs.o und-hfs.lst
ifneq ($(hfs_mod_EXPORTS),no)
CLEANFILES += def-hfs.lst
DEFSYMFILES += def-hfs.lst
endif
MOSTLYCLEANFILES += hfs_mod-fs_hfs.d
UNDSYMFILES += und-hfs.lst

hfs.mod: pre-hfs.o mod-hfs.o
	-rm -f $@
	$(TARGET_CC) $(hfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-hfs.o: $(hfs_mod_DEPENDENCIES) hfs_mod-fs_hfs.o
	-rm -f $@
	$(TARGET_CC) $(hfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ hfs_mod-fs_hfs.o

mod-hfs.o: mod-hfs.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hfs_mod_CFLAGS) -c -o $@ $<

mod-hfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'hfs' $< > $@ || (rm -f $@; exit 1)

ifneq ($(hfs_mod_EXPORTS),no)
def-hfs.lst: pre-hfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 hfs/' > $@
endif

und-hfs.lst: pre-hfs.o
	echo 'hfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

hfs_mod-fs_hfs.o: fs/hfs.c $(fs/hfs.c_DEPENDENCIES)
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(hfs_mod_CFLAGS) -MD -c -o $@ $<
-include hfs_mod-fs_hfs.d

CLEANFILES += cmd-hfs_mod-fs_hfs.lst fs-hfs_mod-fs_hfs.lst
COMMANDFILES += cmd-hfs_mod-fs_hfs.lst
FSFILES += fs-hfs_mod-fs_hfs.lst

cmd-hfs_mod-fs_hfs.lst: fs/hfs.c $(fs/hfs.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh hfs > $@ || (rm -f $@; exit 1)

fs-hfs_mod-fs_hfs.lst: fs/hfs.c $(fs/hfs.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh hfs > $@ || (rm -f $@; exit 1)


hfs_mod_CFLAGS = $(COMMON_CFLAGS)
hfs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For jfs.mod.
jfs_mod_SOURCES = fs/jfs.c
CLEANFILES += jfs.mod mod-jfs.o mod-jfs.c pre-jfs.o jfs_mod-fs_jfs.o und-jfs.lst
ifneq ($(jfs_mod_EXPORTS),no)
CLEANFILES += def-jfs.lst
DEFSYMFILES += def-jfs.lst
endif
MOSTLYCLEANFILES += jfs_mod-fs_jfs.d
UNDSYMFILES += und-jfs.lst

jfs.mod: pre-jfs.o mod-jfs.o
	-rm -f $@
	$(TARGET_CC) $(jfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-jfs.o: $(jfs_mod_DEPENDENCIES) jfs_mod-fs_jfs.o
	-rm -f $@
	$(TARGET_CC) $(jfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ jfs_mod-fs_jfs.o

mod-jfs.o: mod-jfs.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(jfs_mod_CFLAGS) -c -o $@ $<

mod-jfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'jfs' $< > $@ || (rm -f $@; exit 1)

ifneq ($(jfs_mod_EXPORTS),no)
def-jfs.lst: pre-jfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 jfs/' > $@
endif

und-jfs.lst: pre-jfs.o
	echo 'jfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

jfs_mod-fs_jfs.o: fs/jfs.c $(fs/jfs.c_DEPENDENCIES)
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(jfs_mod_CFLAGS) -MD -c -o $@ $<
-include jfs_mod-fs_jfs.d

CLEANFILES += cmd-jfs_mod-fs_jfs.lst fs-jfs_mod-fs_jfs.lst
COMMANDFILES += cmd-jfs_mod-fs_jfs.lst
FSFILES += fs-jfs_mod-fs_jfs.lst

cmd-jfs_mod-fs_jfs.lst: fs/jfs.c $(fs/jfs.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(jfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh jfs > $@ || (rm -f $@; exit 1)

fs-jfs_mod-fs_jfs.lst: fs/jfs.c $(fs/jfs.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(jfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh jfs > $@ || (rm -f $@; exit 1)


jfs_mod_CFLAGS = $(COMMON_CFLAGS)
jfs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For iso9660.mod.
iso9660_mod_SOURCES = fs/iso9660.c
iso9660_mod_CFLAGS = $(COMMON_CFLAGS)
iso9660_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For xfs.mod.
xfs_mod_SOURCES = fs/xfs.c
CLEANFILES += xfs.mod mod-xfs.o mod-xfs.c pre-xfs.o xfs_mod-fs_xfs.o und-xfs.lst
ifneq ($(xfs_mod_EXPORTS),no)
CLEANFILES += def-xfs.lst
DEFSYMFILES += def-xfs.lst
endif
MOSTLYCLEANFILES += xfs_mod-fs_xfs.d
UNDSYMFILES += und-xfs.lst

xfs.mod: pre-xfs.o mod-xfs.o
	-rm -f $@
	$(TARGET_CC) $(xfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-xfs.o: $(xfs_mod_DEPENDENCIES) xfs_mod-fs_xfs.o
	-rm -f $@
	$(TARGET_CC) $(xfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ xfs_mod-fs_xfs.o

mod-xfs.o: mod-xfs.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(xfs_mod_CFLAGS) -c -o $@ $<

mod-xfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'xfs' $< > $@ || (rm -f $@; exit 1)

ifneq ($(xfs_mod_EXPORTS),no)
def-xfs.lst: pre-xfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 xfs/' > $@
endif

und-xfs.lst: pre-xfs.o
	echo 'xfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

xfs_mod-fs_xfs.o: fs/xfs.c $(fs/xfs.c_DEPENDENCIES)
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(xfs_mod_CFLAGS) -MD -c -o $@ $<
-include xfs_mod-fs_xfs.d

CLEANFILES += cmd-xfs_mod-fs_xfs.lst fs-xfs_mod-fs_xfs.lst
COMMANDFILES += cmd-xfs_mod-fs_xfs.lst
FSFILES += fs-xfs_mod-fs_xfs.lst

cmd-xfs_mod-fs_xfs.lst: fs/xfs.c $(fs/xfs.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(xfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh xfs > $@ || (rm -f $@; exit 1)

fs-xfs_mod-fs_xfs.lst: fs/xfs.c $(fs/xfs.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(xfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh xfs > $@ || (rm -f $@; exit 1)


xfs_mod_CFLAGS = $(COMMON_CFLAGS)
xfs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For affs.mod.
affs_mod_SOURCES = fs/affs.c
CLEANFILES += affs.mod mod-affs.o mod-affs.c pre-affs.o affs_mod-fs_affs.o und-affs.lst
ifneq ($(affs_mod_EXPORTS),no)
CLEANFILES += def-affs.lst
DEFSYMFILES += def-affs.lst
endif
MOSTLYCLEANFILES += affs_mod-fs_affs.d
UNDSYMFILES += und-affs.lst

affs.mod: pre-affs.o mod-affs.o
	-rm -f $@
	$(TARGET_CC) $(affs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-affs.o: $(affs_mod_DEPENDENCIES) affs_mod-fs_affs.o
	-rm -f $@
	$(TARGET_CC) $(affs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ affs_mod-fs_affs.o

mod-affs.o: mod-affs.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(affs_mod_CFLAGS) -c -o $@ $<

mod-affs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'affs' $< > $@ || (rm -f $@; exit 1)

ifneq ($(affs_mod_EXPORTS),no)
def-affs.lst: pre-affs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 affs/' > $@
endif

und-affs.lst: pre-affs.o
	echo 'affs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

affs_mod-fs_affs.o: fs/affs.c $(fs/affs.c_DEPENDENCIES)
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(affs_mod_CFLAGS) -MD -c -o $@ $<
-include affs_mod-fs_affs.d

CLEANFILES += cmd-affs_mod-fs_affs.lst fs-affs_mod-fs_affs.lst
COMMANDFILES += cmd-affs_mod-fs_affs.lst
FSFILES += fs-affs_mod-fs_affs.lst

cmd-affs_mod-fs_affs.lst: fs/affs.c $(fs/affs.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(affs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh affs > $@ || (rm -f $@; exit 1)

fs-affs_mod-fs_affs.lst: fs/affs.c $(fs/affs.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(affs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh affs > $@ || (rm -f $@; exit 1)


affs_mod_CFLAGS = $(COMMON_CFLAGS)
affs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For sfs.mod.
sfs_mod_SOURCES = fs/sfs.c
CLEANFILES += sfs.mod mod-sfs.o mod-sfs.c pre-sfs.o sfs_mod-fs_sfs.o und-sfs.lst
ifneq ($(sfs_mod_EXPORTS),no)
CLEANFILES += def-sfs.lst
DEFSYMFILES += def-sfs.lst
endif
MOSTLYCLEANFILES += sfs_mod-fs_sfs.d
UNDSYMFILES += und-sfs.lst

sfs.mod: pre-sfs.o mod-sfs.o
	-rm -f $@
	$(TARGET_CC) $(sfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-sfs.o: $(sfs_mod_DEPENDENCIES) sfs_mod-fs_sfs.o
	-rm -f $@
	$(TARGET_CC) $(sfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ sfs_mod-fs_sfs.o

mod-sfs.o: mod-sfs.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(sfs_mod_CFLAGS) -c -o $@ $<

mod-sfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'sfs' $< > $@ || (rm -f $@; exit 1)

ifneq ($(sfs_mod_EXPORTS),no)
def-sfs.lst: pre-sfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 sfs/' > $@
endif

und-sfs.lst: pre-sfs.o
	echo 'sfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

sfs_mod-fs_sfs.o: fs/sfs.c $(fs/sfs.c_DEPENDENCIES)
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(sfs_mod_CFLAGS) -MD -c -o $@ $<
-include sfs_mod-fs_sfs.d

CLEANFILES += cmd-sfs_mod-fs_sfs.lst fs-sfs_mod-fs_sfs.lst
COMMANDFILES += cmd-sfs_mod-fs_sfs.lst
FSFILES += fs-sfs_mod-fs_sfs.lst

cmd-sfs_mod-fs_sfs.lst: fs/sfs.c $(fs/sfs.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(sfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh sfs > $@ || (rm -f $@; exit 1)

fs-sfs_mod-fs_sfs.lst: fs/sfs.c $(fs/sfs.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(sfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh sfs > $@ || (rm -f $@; exit 1)


sfs_mod_CFLAGS = $(COMMON_CFLAGS)
sfs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For _linux.mod.
#_linux_mod_SOURCES = loader/sparc64/ieee1275/linux.c
#_linux_mod_CFLAGS = $(COMMON_CFLAGS)
#_linux_mod_LDFLAGS = $(COMMON_LDFLAGS)
 
# For linux.mod.
#linux_mod_SOURCES = loader/sparc64/ieee1275/linux_normal.c
#linux_mod_CFLAGS = $(COMMON_CFLAGS)
#linux_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For normal.mod.
normal_mod_SOURCES = normal/arg.c normal/cmdline.c normal/command.c	\
	normal/completion.c normal/execute.c				\
	normal/function.c normal/lexer.c normal/main.c normal/menu.c	\
	normal/menu_entry.c normal/misc.c normal/script.c		\
	normal/sparc64/setjmp.S						\
	grub_script.tab.c
CLEANFILES += normal.mod mod-normal.o mod-normal.c pre-normal.o normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-normal_script.o normal_mod-normal_sparc64_setjmp.o normal_mod-grub_script_tab.o und-normal.lst
ifneq ($(normal_mod_EXPORTS),no)
CLEANFILES += def-normal.lst
DEFSYMFILES += def-normal.lst
endif
MOSTLYCLEANFILES += normal_mod-normal_arg.d normal_mod-normal_cmdline.d normal_mod-normal_command.d normal_mod-normal_completion.d normal_mod-normal_execute.d normal_mod-normal_function.d normal_mod-normal_lexer.d normal_mod-normal_main.d normal_mod-normal_menu.d normal_mod-normal_menu_entry.d normal_mod-normal_misc.d normal_mod-normal_script.d normal_mod-normal_sparc64_setjmp.d normal_mod-grub_script_tab.d
UNDSYMFILES += und-normal.lst

normal.mod: pre-normal.o mod-normal.o
	-rm -f $@
	$(TARGET_CC) $(normal_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-normal.o: $(normal_mod_DEPENDENCIES) normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-normal_script.o normal_mod-normal_sparc64_setjmp.o normal_mod-grub_script_tab.o
	-rm -f $@
	$(TARGET_CC) $(normal_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-normal_script.o normal_mod-normal_sparc64_setjmp.o normal_mod-grub_script_tab.o

mod-normal.o: mod-normal.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

mod-normal.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'normal' $< > $@ || (rm -f $@; exit 1)

ifneq ($(normal_mod_EXPORTS),no)
def-normal.lst: pre-normal.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 normal/' > $@
endif

und-normal.lst: pre-normal.o
	echo 'normal' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

normal_mod-normal_arg.o: normal/arg.c $(normal/arg.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_arg.d

CLEANFILES += cmd-normal_mod-normal_arg.lst fs-normal_mod-normal_arg.lst
COMMANDFILES += cmd-normal_mod-normal_arg.lst
FSFILES += fs-normal_mod-normal_arg.lst

cmd-normal_mod-normal_arg.lst: normal/arg.c $(normal/arg.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_arg.lst: normal/arg.c $(normal/arg.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_cmdline.o: normal/cmdline.c $(normal/cmdline.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_cmdline.d

CLEANFILES += cmd-normal_mod-normal_cmdline.lst fs-normal_mod-normal_cmdline.lst
COMMANDFILES += cmd-normal_mod-normal_cmdline.lst
FSFILES += fs-normal_mod-normal_cmdline.lst

cmd-normal_mod-normal_cmdline.lst: normal/cmdline.c $(normal/cmdline.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_cmdline.lst: normal/cmdline.c $(normal/cmdline.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_command.o: normal/command.c $(normal/command.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_command.d

CLEANFILES += cmd-normal_mod-normal_command.lst fs-normal_mod-normal_command.lst
COMMANDFILES += cmd-normal_mod-normal_command.lst
FSFILES += fs-normal_mod-normal_command.lst

cmd-normal_mod-normal_command.lst: normal/command.c $(normal/command.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_command.lst: normal/command.c $(normal/command.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_completion.o: normal/completion.c $(normal/completion.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_completion.d

CLEANFILES += cmd-normal_mod-normal_completion.lst fs-normal_mod-normal_completion.lst
COMMANDFILES += cmd-normal_mod-normal_completion.lst
FSFILES += fs-normal_mod-normal_completion.lst

cmd-normal_mod-normal_completion.lst: normal/completion.c $(normal/completion.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_completion.lst: normal/completion.c $(normal/completion.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_execute.o: normal/execute.c $(normal/execute.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_execute.d

CLEANFILES += cmd-normal_mod-normal_execute.lst fs-normal_mod-normal_execute.lst
COMMANDFILES += cmd-normal_mod-normal_execute.lst
FSFILES += fs-normal_mod-normal_execute.lst

cmd-normal_mod-normal_execute.lst: normal/execute.c $(normal/execute.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_execute.lst: normal/execute.c $(normal/execute.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_function.o: normal/function.c $(normal/function.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_function.d

CLEANFILES += cmd-normal_mod-normal_function.lst fs-normal_mod-normal_function.lst
COMMANDFILES += cmd-normal_mod-normal_function.lst
FSFILES += fs-normal_mod-normal_function.lst

cmd-normal_mod-normal_function.lst: normal/function.c $(normal/function.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_function.lst: normal/function.c $(normal/function.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_lexer.o: normal/lexer.c $(normal/lexer.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_lexer.d

CLEANFILES += cmd-normal_mod-normal_lexer.lst fs-normal_mod-normal_lexer.lst
COMMANDFILES += cmd-normal_mod-normal_lexer.lst
FSFILES += fs-normal_mod-normal_lexer.lst

cmd-normal_mod-normal_lexer.lst: normal/lexer.c $(normal/lexer.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_lexer.lst: normal/lexer.c $(normal/lexer.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_main.o: normal/main.c $(normal/main.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_main.d

CLEANFILES += cmd-normal_mod-normal_main.lst fs-normal_mod-normal_main.lst
COMMANDFILES += cmd-normal_mod-normal_main.lst
FSFILES += fs-normal_mod-normal_main.lst

cmd-normal_mod-normal_main.lst: normal/main.c $(normal/main.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_main.lst: normal/main.c $(normal/main.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_menu.o: normal/menu.c $(normal/menu.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_menu.d

CLEANFILES += cmd-normal_mod-normal_menu.lst fs-normal_mod-normal_menu.lst
COMMANDFILES += cmd-normal_mod-normal_menu.lst
FSFILES += fs-normal_mod-normal_menu.lst

cmd-normal_mod-normal_menu.lst: normal/menu.c $(normal/menu.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_menu.lst: normal/menu.c $(normal/menu.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_menu_entry.o: normal/menu_entry.c $(normal/menu_entry.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_menu_entry.d

CLEANFILES += cmd-normal_mod-normal_menu_entry.lst fs-normal_mod-normal_menu_entry.lst
COMMANDFILES += cmd-normal_mod-normal_menu_entry.lst
FSFILES += fs-normal_mod-normal_menu_entry.lst

cmd-normal_mod-normal_menu_entry.lst: normal/menu_entry.c $(normal/menu_entry.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_menu_entry.lst: normal/menu_entry.c $(normal/menu_entry.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_misc.o: normal/misc.c $(normal/misc.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_misc.d

CLEANFILES += cmd-normal_mod-normal_misc.lst fs-normal_mod-normal_misc.lst
COMMANDFILES += cmd-normal_mod-normal_misc.lst
FSFILES += fs-normal_mod-normal_misc.lst

cmd-normal_mod-normal_misc.lst: normal/misc.c $(normal/misc.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_misc.lst: normal/misc.c $(normal/misc.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_script.o: normal/script.c $(normal/script.c_DEPENDENCIES)
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_script.d

CLEANFILES += cmd-normal_mod-normal_script.lst fs-normal_mod-normal_script.lst
COMMANDFILES += cmd-normal_mod-normal_script.lst
FSFILES += fs-normal_mod-normal_script.lst

cmd-normal_mod-normal_script.lst: normal/script.c $(normal/script.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_script.lst: normal/script.c $(normal/script.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_sparc64_setjmp.o: normal/sparc64/setjmp.S $(normal/sparc64/setjmp.S_DEPENDENCIES)
	$(TARGET_CC) -Inormal/sparc64 -I$(srcdir)/normal/sparc64 $(TARGET_CPPFLAGS) -DASM_FILE=1 $(TARGET_ASFLAGS) $(normal_mod_ASFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_sparc64_setjmp.d

CLEANFILES += cmd-normal_mod-normal_sparc64_setjmp.lst fs-normal_mod-normal_sparc64_setjmp.lst
COMMANDFILES += cmd-normal_mod-normal_sparc64_setjmp.lst
FSFILES += fs-normal_mod-normal_sparc64_setjmp.lst

cmd-normal_mod-normal_sparc64_setjmp.lst: normal/sparc64/setjmp.S $(normal/sparc64/setjmp.S_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal/sparc64 -I$(srcdir)/normal/sparc64 $(TARGET_CPPFLAGS) $(TARGET_ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_sparc64_setjmp.lst: normal/sparc64/setjmp.S $(normal/sparc64/setjmp.S_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal/sparc64 -I$(srcdir)/normal/sparc64 $(TARGET_CPPFLAGS) $(TARGET_ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-grub_script_tab.o: grub_script.tab.c $(grub_script.tab.c_DEPENDENCIES)
	$(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-grub_script_tab.d

CLEANFILES += cmd-normal_mod-grub_script_tab.lst fs-normal_mod-grub_script_tab.lst
COMMANDFILES += cmd-normal_mod-grub_script_tab.lst
FSFILES += fs-normal_mod-grub_script_tab.lst

cmd-normal_mod-grub_script_tab.lst: grub_script.tab.c $(grub_script.tab.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-grub_script_tab.lst: grub_script.tab.c $(grub_script.tab.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod_CFLAGS = $(COMMON_CFLAGS)
normal_mod_ASFLAGS = $(COMMON_ASFLAGS)
normal_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For hello.mod.
hello_mod_SOURCES = hello/hello.c
CLEANFILES += hello.mod mod-hello.o mod-hello.c pre-hello.o hello_mod-hello_hello.o und-hello.lst
ifneq ($(hello_mod_EXPORTS),no)
CLEANFILES += def-hello.lst
DEFSYMFILES += def-hello.lst
endif
MOSTLYCLEANFILES += hello_mod-hello_hello.d
UNDSYMFILES += und-hello.lst

hello.mod: pre-hello.o mod-hello.o
	-rm -f $@
	$(TARGET_CC) $(hello_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-hello.o: $(hello_mod_DEPENDENCIES) hello_mod-hello_hello.o
	-rm -f $@
	$(TARGET_CC) $(hello_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ hello_mod-hello_hello.o

mod-hello.o: mod-hello.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hello_mod_CFLAGS) -c -o $@ $<

mod-hello.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'hello' $< > $@ || (rm -f $@; exit 1)

ifneq ($(hello_mod_EXPORTS),no)
def-hello.lst: pre-hello.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 hello/' > $@
endif

und-hello.lst: pre-hello.o
	echo 'hello' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

hello_mod-hello_hello.o: hello/hello.c $(hello/hello.c_DEPENDENCIES)
	$(TARGET_CC) -Ihello -I$(srcdir)/hello $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(hello_mod_CFLAGS) -MD -c -o $@ $<
-include hello_mod-hello_hello.d

CLEANFILES += cmd-hello_mod-hello_hello.lst fs-hello_mod-hello_hello.lst
COMMANDFILES += cmd-hello_mod-hello_hello.lst
FSFILES += fs-hello_mod-hello_hello.lst

cmd-hello_mod-hello_hello.lst: hello/hello.c $(hello/hello.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ihello -I$(srcdir)/hello $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hello_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh hello > $@ || (rm -f $@; exit 1)

fs-hello_mod-hello_hello.lst: hello/hello.c $(hello/hello.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ihello -I$(srcdir)/hello $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hello_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh hello > $@ || (rm -f $@; exit 1)


hello_mod_CFLAGS = $(COMMON_CFLAGS)
hello_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For boot.mod.
boot_mod_SOURCES = commands/boot.c
CLEANFILES += boot.mod mod-boot.o mod-boot.c pre-boot.o boot_mod-commands_boot.o und-boot.lst
ifneq ($(boot_mod_EXPORTS),no)
CLEANFILES += def-boot.lst
DEFSYMFILES += def-boot.lst
endif
MOSTLYCLEANFILES += boot_mod-commands_boot.d
UNDSYMFILES += und-boot.lst

boot.mod: pre-boot.o mod-boot.o
	-rm -f $@
	$(TARGET_CC) $(boot_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-boot.o: $(boot_mod_DEPENDENCIES) boot_mod-commands_boot.o
	-rm -f $@
	$(TARGET_CC) $(boot_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ boot_mod-commands_boot.o

mod-boot.o: mod-boot.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(boot_mod_CFLAGS) -c -o $@ $<

mod-boot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'boot' $< > $@ || (rm -f $@; exit 1)

ifneq ($(boot_mod_EXPORTS),no)
def-boot.lst: pre-boot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 boot/' > $@
endif

und-boot.lst: pre-boot.o
	echo 'boot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

boot_mod-commands_boot.o: commands/boot.c $(commands/boot.c_DEPENDENCIES)
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(boot_mod_CFLAGS) -MD -c -o $@ $<
-include boot_mod-commands_boot.d

CLEANFILES += cmd-boot_mod-commands_boot.lst fs-boot_mod-commands_boot.lst
COMMANDFILES += cmd-boot_mod-commands_boot.lst
FSFILES += fs-boot_mod-commands_boot.lst

cmd-boot_mod-commands_boot.lst: commands/boot.c $(commands/boot.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(boot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh boot > $@ || (rm -f $@; exit 1)

fs-boot_mod-commands_boot.lst: commands/boot.c $(commands/boot.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(boot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh boot > $@ || (rm -f $@; exit 1)


boot_mod_CFLAGS = $(COMMON_CFLAGS)
boot_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For terminal.mod.
terminal_mod_SOURCES = commands/terminal.c
CLEANFILES += terminal.mod mod-terminal.o mod-terminal.c pre-terminal.o terminal_mod-commands_terminal.o und-terminal.lst
ifneq ($(terminal_mod_EXPORTS),no)
CLEANFILES += def-terminal.lst
DEFSYMFILES += def-terminal.lst
endif
MOSTLYCLEANFILES += terminal_mod-commands_terminal.d
UNDSYMFILES += und-terminal.lst

terminal.mod: pre-terminal.o mod-terminal.o
	-rm -f $@
	$(TARGET_CC) $(terminal_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-terminal.o: $(terminal_mod_DEPENDENCIES) terminal_mod-commands_terminal.o
	-rm -f $@
	$(TARGET_CC) $(terminal_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ terminal_mod-commands_terminal.o

mod-terminal.o: mod-terminal.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(terminal_mod_CFLAGS) -c -o $@ $<

mod-terminal.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'terminal' $< > $@ || (rm -f $@; exit 1)

ifneq ($(terminal_mod_EXPORTS),no)
def-terminal.lst: pre-terminal.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 terminal/' > $@
endif

und-terminal.lst: pre-terminal.o
	echo 'terminal' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

terminal_mod-commands_terminal.o: commands/terminal.c $(commands/terminal.c_DEPENDENCIES)
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(terminal_mod_CFLAGS) -MD -c -o $@ $<
-include terminal_mod-commands_terminal.d

CLEANFILES += cmd-terminal_mod-commands_terminal.lst fs-terminal_mod-commands_terminal.lst
COMMANDFILES += cmd-terminal_mod-commands_terminal.lst
FSFILES += fs-terminal_mod-commands_terminal.lst

cmd-terminal_mod-commands_terminal.lst: commands/terminal.c $(commands/terminal.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(terminal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh terminal > $@ || (rm -f $@; exit 1)

fs-terminal_mod-commands_terminal.lst: commands/terminal.c $(commands/terminal.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(terminal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh terminal > $@ || (rm -f $@; exit 1)


terminal_mod_CFLAGS = $(COMMON_CFLAGS)
terminal_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For ls.mod.
ls_mod_SOURCES = commands/ls.c
CLEANFILES += ls.mod mod-ls.o mod-ls.c pre-ls.o ls_mod-commands_ls.o und-ls.lst
ifneq ($(ls_mod_EXPORTS),no)
CLEANFILES += def-ls.lst
DEFSYMFILES += def-ls.lst
endif
MOSTLYCLEANFILES += ls_mod-commands_ls.d
UNDSYMFILES += und-ls.lst

ls.mod: pre-ls.o mod-ls.o
	-rm -f $@
	$(TARGET_CC) $(ls_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ls.o: $(ls_mod_DEPENDENCIES) ls_mod-commands_ls.o
	-rm -f $@
	$(TARGET_CC) $(ls_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ ls_mod-commands_ls.o

mod-ls.o: mod-ls.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ls_mod_CFLAGS) -c -o $@ $<

mod-ls.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'ls' $< > $@ || (rm -f $@; exit 1)

ifneq ($(ls_mod_EXPORTS),no)
def-ls.lst: pre-ls.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 ls/' > $@
endif

und-ls.lst: pre-ls.o
	echo 'ls' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

ls_mod-commands_ls.o: commands/ls.c $(commands/ls.c_DEPENDENCIES)
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(ls_mod_CFLAGS) -MD -c -o $@ $<
-include ls_mod-commands_ls.d

CLEANFILES += cmd-ls_mod-commands_ls.lst fs-ls_mod-commands_ls.lst
COMMANDFILES += cmd-ls_mod-commands_ls.lst
FSFILES += fs-ls_mod-commands_ls.lst

cmd-ls_mod-commands_ls.lst: commands/ls.c $(commands/ls.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ls_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ls > $@ || (rm -f $@; exit 1)

fs-ls_mod-commands_ls.lst: commands/ls.c $(commands/ls.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ls_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ls > $@ || (rm -f $@; exit 1)


ls_mod_CFLAGS = $(COMMON_CFLAGS)
ls_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For cmp.mod.
cmp_mod_SOURCES = commands/cmp.c
CLEANFILES += cmp.mod mod-cmp.o mod-cmp.c pre-cmp.o cmp_mod-commands_cmp.o und-cmp.lst
ifneq ($(cmp_mod_EXPORTS),no)
CLEANFILES += def-cmp.lst
DEFSYMFILES += def-cmp.lst
endif
MOSTLYCLEANFILES += cmp_mod-commands_cmp.d
UNDSYMFILES += und-cmp.lst

cmp.mod: pre-cmp.o mod-cmp.o
	-rm -f $@
	$(TARGET_CC) $(cmp_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-cmp.o: $(cmp_mod_DEPENDENCIES) cmp_mod-commands_cmp.o
	-rm -f $@
	$(TARGET_CC) $(cmp_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ cmp_mod-commands_cmp.o

mod-cmp.o: mod-cmp.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cmp_mod_CFLAGS) -c -o $@ $<

mod-cmp.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'cmp' $< > $@ || (rm -f $@; exit 1)

ifneq ($(cmp_mod_EXPORTS),no)
def-cmp.lst: pre-cmp.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 cmp/' > $@
endif

und-cmp.lst: pre-cmp.o
	echo 'cmp' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

cmp_mod-commands_cmp.o: commands/cmp.c $(commands/cmp.c_DEPENDENCIES)
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(cmp_mod_CFLAGS) -MD -c -o $@ $<
-include cmp_mod-commands_cmp.d

CLEANFILES += cmd-cmp_mod-commands_cmp.lst fs-cmp_mod-commands_cmp.lst
COMMANDFILES += cmd-cmp_mod-commands_cmp.lst
FSFILES += fs-cmp_mod-commands_cmp.lst

cmd-cmp_mod-commands_cmp.lst: commands/cmp.c $(commands/cmp.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cmp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh cmp > $@ || (rm -f $@; exit 1)

fs-cmp_mod-commands_cmp.lst: commands/cmp.c $(commands/cmp.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cmp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh cmp > $@ || (rm -f $@; exit 1)


cmp_mod_CFLAGS = $(COMMON_CFLAGS)
cmp_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For cat.mod.
cat_mod_SOURCES = commands/cat.c
CLEANFILES += cat.mod mod-cat.o mod-cat.c pre-cat.o cat_mod-commands_cat.o und-cat.lst
ifneq ($(cat_mod_EXPORTS),no)
CLEANFILES += def-cat.lst
DEFSYMFILES += def-cat.lst
endif
MOSTLYCLEANFILES += cat_mod-commands_cat.d
UNDSYMFILES += und-cat.lst

cat.mod: pre-cat.o mod-cat.o
	-rm -f $@
	$(TARGET_CC) $(cat_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-cat.o: $(cat_mod_DEPENDENCIES) cat_mod-commands_cat.o
	-rm -f $@
	$(TARGET_CC) $(cat_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ cat_mod-commands_cat.o

mod-cat.o: mod-cat.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cat_mod_CFLAGS) -c -o $@ $<

mod-cat.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'cat' $< > $@ || (rm -f $@; exit 1)

ifneq ($(cat_mod_EXPORTS),no)
def-cat.lst: pre-cat.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 cat/' > $@
endif

und-cat.lst: pre-cat.o
	echo 'cat' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

cat_mod-commands_cat.o: commands/cat.c $(commands/cat.c_DEPENDENCIES)
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(cat_mod_CFLAGS) -MD -c -o $@ $<
-include cat_mod-commands_cat.d

CLEANFILES += cmd-cat_mod-commands_cat.lst fs-cat_mod-commands_cat.lst
COMMANDFILES += cmd-cat_mod-commands_cat.lst
FSFILES += fs-cat_mod-commands_cat.lst

cmd-cat_mod-commands_cat.lst: commands/cat.c $(commands/cat.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh cat > $@ || (rm -f $@; exit 1)

fs-cat_mod-commands_cat.lst: commands/cat.c $(commands/cat.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh cat > $@ || (rm -f $@; exit 1)


cat_mod_CFLAGS = $(COMMON_CFLAGS)
cat_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For font.mod.
font_mod_SOURCES = font/manager.c
CLEANFILES += font.mod mod-font.o mod-font.c pre-font.o font_mod-font_manager.o und-font.lst
ifneq ($(font_mod_EXPORTS),no)
CLEANFILES += def-font.lst
DEFSYMFILES += def-font.lst
endif
MOSTLYCLEANFILES += font_mod-font_manager.d
UNDSYMFILES += und-font.lst

font.mod: pre-font.o mod-font.o
	-rm -f $@
	$(TARGET_CC) $(font_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-font.o: $(font_mod_DEPENDENCIES) font_mod-font_manager.o
	-rm -f $@
	$(TARGET_CC) $(font_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ font_mod-font_manager.o

mod-font.o: mod-font.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(font_mod_CFLAGS) -c -o $@ $<

mod-font.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'font' $< > $@ || (rm -f $@; exit 1)

ifneq ($(font_mod_EXPORTS),no)
def-font.lst: pre-font.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 font/' > $@
endif

und-font.lst: pre-font.o
	echo 'font' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

font_mod-font_manager.o: font/manager.c $(font/manager.c_DEPENDENCIES)
	$(TARGET_CC) -Ifont -I$(srcdir)/font $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(font_mod_CFLAGS) -MD -c -o $@ $<
-include font_mod-font_manager.d

CLEANFILES += cmd-font_mod-font_manager.lst fs-font_mod-font_manager.lst
COMMANDFILES += cmd-font_mod-font_manager.lst
FSFILES += fs-font_mod-font_manager.lst

cmd-font_mod-font_manager.lst: font/manager.c $(font/manager.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifont -I$(srcdir)/font $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(font_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh font > $@ || (rm -f $@; exit 1)

fs-font_mod-font_manager.lst: font/manager.c $(font/manager.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifont -I$(srcdir)/font $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(font_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh font > $@ || (rm -f $@; exit 1)


font_mod_CFLAGS = $(COMMON_CFLAGS)
font_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For amiga.mod
amiga_mod_SOURCES = partmap/amiga.c
CLEANFILES += amiga.mod mod-amiga.o mod-amiga.c pre-amiga.o amiga_mod-partmap_amiga.o und-amiga.lst
ifneq ($(amiga_mod_EXPORTS),no)
CLEANFILES += def-amiga.lst
DEFSYMFILES += def-amiga.lst
endif
MOSTLYCLEANFILES += amiga_mod-partmap_amiga.d
UNDSYMFILES += und-amiga.lst

amiga.mod: pre-amiga.o mod-amiga.o
	-rm -f $@
	$(TARGET_CC) $(amiga_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-amiga.o: $(amiga_mod_DEPENDENCIES) amiga_mod-partmap_amiga.o
	-rm -f $@
	$(TARGET_CC) $(amiga_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ amiga_mod-partmap_amiga.o

mod-amiga.o: mod-amiga.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(amiga_mod_CFLAGS) -c -o $@ $<

mod-amiga.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'amiga' $< > $@ || (rm -f $@; exit 1)

ifneq ($(amiga_mod_EXPORTS),no)
def-amiga.lst: pre-amiga.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 amiga/' > $@
endif

und-amiga.lst: pre-amiga.o
	echo 'amiga' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

amiga_mod-partmap_amiga.o: partmap/amiga.c $(partmap/amiga.c_DEPENDENCIES)
	$(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(amiga_mod_CFLAGS) -MD -c -o $@ $<
-include amiga_mod-partmap_amiga.d

CLEANFILES += cmd-amiga_mod-partmap_amiga.lst fs-amiga_mod-partmap_amiga.lst
COMMANDFILES += cmd-amiga_mod-partmap_amiga.lst
FSFILES += fs-amiga_mod-partmap_amiga.lst

cmd-amiga_mod-partmap_amiga.lst: partmap/amiga.c $(partmap/amiga.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(amiga_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh amiga > $@ || (rm -f $@; exit 1)

fs-amiga_mod-partmap_amiga.lst: partmap/amiga.c $(partmap/amiga.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(amiga_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh amiga > $@ || (rm -f $@; exit 1)


amiga_mod_CFLAGS = $(COMMON_CFLAGS)
amiga_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For apple.mod
apple_mod_SOURCES = partmap/apple.c
CLEANFILES += apple.mod mod-apple.o mod-apple.c pre-apple.o apple_mod-partmap_apple.o und-apple.lst
ifneq ($(apple_mod_EXPORTS),no)
CLEANFILES += def-apple.lst
DEFSYMFILES += def-apple.lst
endif
MOSTLYCLEANFILES += apple_mod-partmap_apple.d
UNDSYMFILES += und-apple.lst

apple.mod: pre-apple.o mod-apple.o
	-rm -f $@
	$(TARGET_CC) $(apple_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-apple.o: $(apple_mod_DEPENDENCIES) apple_mod-partmap_apple.o
	-rm -f $@
	$(TARGET_CC) $(apple_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ apple_mod-partmap_apple.o

mod-apple.o: mod-apple.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(apple_mod_CFLAGS) -c -o $@ $<

mod-apple.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'apple' $< > $@ || (rm -f $@; exit 1)

ifneq ($(apple_mod_EXPORTS),no)
def-apple.lst: pre-apple.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 apple/' > $@
endif

und-apple.lst: pre-apple.o
	echo 'apple' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

apple_mod-partmap_apple.o: partmap/apple.c $(partmap/apple.c_DEPENDENCIES)
	$(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(apple_mod_CFLAGS) -MD -c -o $@ $<
-include apple_mod-partmap_apple.d

CLEANFILES += cmd-apple_mod-partmap_apple.lst fs-apple_mod-partmap_apple.lst
COMMANDFILES += cmd-apple_mod-partmap_apple.lst
FSFILES += fs-apple_mod-partmap_apple.lst

cmd-apple_mod-partmap_apple.lst: partmap/apple.c $(partmap/apple.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(apple_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh apple > $@ || (rm -f $@; exit 1)

fs-apple_mod-partmap_apple.lst: partmap/apple.c $(partmap/apple.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(apple_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh apple > $@ || (rm -f $@; exit 1)


apple_mod_CFLAGS = $(COMMON_CFLAGS)
apple_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For pc.mod
pc_mod_SOURCES = partmap/pc.c
CLEANFILES += pc.mod mod-pc.o mod-pc.c pre-pc.o pc_mod-partmap_pc.o und-pc.lst
ifneq ($(pc_mod_EXPORTS),no)
CLEANFILES += def-pc.lst
DEFSYMFILES += def-pc.lst
endif
MOSTLYCLEANFILES += pc_mod-partmap_pc.d
UNDSYMFILES += und-pc.lst

pc.mod: pre-pc.o mod-pc.o
	-rm -f $@
	$(TARGET_CC) $(pc_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-pc.o: $(pc_mod_DEPENDENCIES) pc_mod-partmap_pc.o
	-rm -f $@
	$(TARGET_CC) $(pc_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ pc_mod-partmap_pc.o

mod-pc.o: mod-pc.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(pc_mod_CFLAGS) -c -o $@ $<

mod-pc.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'pc' $< > $@ || (rm -f $@; exit 1)

ifneq ($(pc_mod_EXPORTS),no)
def-pc.lst: pre-pc.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 pc/' > $@
endif

und-pc.lst: pre-pc.o
	echo 'pc' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

pc_mod-partmap_pc.o: partmap/pc.c $(partmap/pc.c_DEPENDENCIES)
	$(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(pc_mod_CFLAGS) -MD -c -o $@ $<
-include pc_mod-partmap_pc.d

CLEANFILES += cmd-pc_mod-partmap_pc.lst fs-pc_mod-partmap_pc.lst
COMMANDFILES += cmd-pc_mod-partmap_pc.lst
FSFILES += fs-pc_mod-partmap_pc.lst

cmd-pc_mod-partmap_pc.lst: partmap/pc.c $(partmap/pc.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(pc_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh pc > $@ || (rm -f $@; exit 1)

fs-pc_mod-partmap_pc.lst: partmap/pc.c $(partmap/pc.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(pc_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh pc > $@ || (rm -f $@; exit 1)


pc_mod_CFLAGS = $(COMMON_CFLAGS)
pc_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For sun.mod
sun_mod_SOURCES = partmap/sun.c
CLEANFILES += sun.mod mod-sun.o mod-sun.c pre-sun.o sun_mod-partmap_sun.o und-sun.lst
ifneq ($(sun_mod_EXPORTS),no)
CLEANFILES += def-sun.lst
DEFSYMFILES += def-sun.lst
endif
MOSTLYCLEANFILES += sun_mod-partmap_sun.d
UNDSYMFILES += und-sun.lst

sun.mod: pre-sun.o mod-sun.o
	-rm -f $@
	$(TARGET_CC) $(sun_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-sun.o: $(sun_mod_DEPENDENCIES) sun_mod-partmap_sun.o
	-rm -f $@
	$(TARGET_CC) $(sun_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ sun_mod-partmap_sun.o

mod-sun.o: mod-sun.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(sun_mod_CFLAGS) -c -o $@ $<

mod-sun.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'sun' $< > $@ || (rm -f $@; exit 1)

ifneq ($(sun_mod_EXPORTS),no)
def-sun.lst: pre-sun.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 sun/' > $@
endif

und-sun.lst: pre-sun.o
	echo 'sun' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

sun_mod-partmap_sun.o: partmap/sun.c $(partmap/sun.c_DEPENDENCIES)
	$(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(sun_mod_CFLAGS) -MD -c -o $@ $<
-include sun_mod-partmap_sun.d

CLEANFILES += cmd-sun_mod-partmap_sun.lst fs-sun_mod-partmap_sun.lst
COMMANDFILES += cmd-sun_mod-partmap_sun.lst
FSFILES += fs-sun_mod-partmap_sun.lst

cmd-sun_mod-partmap_sun.lst: partmap/sun.c $(partmap/sun.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(sun_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh sun > $@ || (rm -f $@; exit 1)

fs-sun_mod-partmap_sun.lst: partmap/sun.c $(partmap/sun.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(sun_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh sun > $@ || (rm -f $@; exit 1)


sun_mod_CFLAGS = $(COMMON_CFLAGS)
sun_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For acorn.mod
acorn_mod_SOURCES = partmap/acorn.c
CLEANFILES += acorn.mod mod-acorn.o mod-acorn.c pre-acorn.o acorn_mod-partmap_acorn.o und-acorn.lst
ifneq ($(acorn_mod_EXPORTS),no)
CLEANFILES += def-acorn.lst
DEFSYMFILES += def-acorn.lst
endif
MOSTLYCLEANFILES += acorn_mod-partmap_acorn.d
UNDSYMFILES += und-acorn.lst

acorn.mod: pre-acorn.o mod-acorn.o
	-rm -f $@
	$(TARGET_CC) $(acorn_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-acorn.o: $(acorn_mod_DEPENDENCIES) acorn_mod-partmap_acorn.o
	-rm -f $@
	$(TARGET_CC) $(acorn_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ acorn_mod-partmap_acorn.o

mod-acorn.o: mod-acorn.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(acorn_mod_CFLAGS) -c -o $@ $<

mod-acorn.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'acorn' $< > $@ || (rm -f $@; exit 1)

ifneq ($(acorn_mod_EXPORTS),no)
def-acorn.lst: pre-acorn.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 acorn/' > $@
endif

und-acorn.lst: pre-acorn.o
	echo 'acorn' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

acorn_mod-partmap_acorn.o: partmap/acorn.c $(partmap/acorn.c_DEPENDENCIES)
	$(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(acorn_mod_CFLAGS) -MD -c -o $@ $<
-include acorn_mod-partmap_acorn.d

CLEANFILES += cmd-acorn_mod-partmap_acorn.lst fs-acorn_mod-partmap_acorn.lst
COMMANDFILES += cmd-acorn_mod-partmap_acorn.lst
FSFILES += fs-acorn_mod-partmap_acorn.lst

cmd-acorn_mod-partmap_acorn.lst: partmap/acorn.c $(partmap/acorn.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(acorn_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh acorn > $@ || (rm -f $@; exit 1)

fs-acorn_mod-partmap_acorn.lst: partmap/acorn.c $(partmap/acorn.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(acorn_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh acorn > $@ || (rm -f $@; exit 1)


acorn_mod_CFLAGS = $(COMMON_CFLAGS)

# For loopback.mod
loopback_mod_SOURCES = disk/loopback.c
CLEANFILES += loopback.mod mod-loopback.o mod-loopback.c pre-loopback.o loopback_mod-disk_loopback.o und-loopback.lst
ifneq ($(loopback_mod_EXPORTS),no)
CLEANFILES += def-loopback.lst
DEFSYMFILES += def-loopback.lst
endif
MOSTLYCLEANFILES += loopback_mod-disk_loopback.d
UNDSYMFILES += und-loopback.lst

loopback.mod: pre-loopback.o mod-loopback.o
	-rm -f $@
	$(TARGET_CC) $(loopback_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-loopback.o: $(loopback_mod_DEPENDENCIES) loopback_mod-disk_loopback.o
	-rm -f $@
	$(TARGET_CC) $(loopback_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ loopback_mod-disk_loopback.o

mod-loopback.o: mod-loopback.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(loopback_mod_CFLAGS) -c -o $@ $<

mod-loopback.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'loopback' $< > $@ || (rm -f $@; exit 1)

ifneq ($(loopback_mod_EXPORTS),no)
def-loopback.lst: pre-loopback.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 loopback/' > $@
endif

und-loopback.lst: pre-loopback.o
	echo 'loopback' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

loopback_mod-disk_loopback.o: disk/loopback.c $(disk/loopback.c_DEPENDENCIES)
	$(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(loopback_mod_CFLAGS) -MD -c -o $@ $<
-include loopback_mod-disk_loopback.d

CLEANFILES += cmd-loopback_mod-disk_loopback.lst fs-loopback_mod-disk_loopback.lst
COMMANDFILES += cmd-loopback_mod-disk_loopback.lst
FSFILES += fs-loopback_mod-disk_loopback.lst

cmd-loopback_mod-disk_loopback.lst: disk/loopback.c $(disk/loopback.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(loopback_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh loopback > $@ || (rm -f $@; exit 1)

fs-loopback_mod-disk_loopback.lst: disk/loopback.c $(disk/loopback.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(loopback_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh loopback > $@ || (rm -f $@; exit 1)


loopback_mod_CFLAGS = $(COMMON_CFLAGS)
loopback_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For suspend.mod
suspend_mod_SOURCES = commands/ieee1275/suspend.c
CLEANFILES += suspend.mod mod-suspend.o mod-suspend.c pre-suspend.o suspend_mod-commands_ieee1275_suspend.o und-suspend.lst
ifneq ($(suspend_mod_EXPORTS),no)
CLEANFILES += def-suspend.lst
DEFSYMFILES += def-suspend.lst
endif
MOSTLYCLEANFILES += suspend_mod-commands_ieee1275_suspend.d
UNDSYMFILES += und-suspend.lst

suspend.mod: pre-suspend.o mod-suspend.o
	-rm -f $@
	$(TARGET_CC) $(suspend_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-suspend.o: $(suspend_mod_DEPENDENCIES) suspend_mod-commands_ieee1275_suspend.o
	-rm -f $@
	$(TARGET_CC) $(suspend_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ suspend_mod-commands_ieee1275_suspend.o

mod-suspend.o: mod-suspend.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(suspend_mod_CFLAGS) -c -o $@ $<

mod-suspend.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'suspend' $< > $@ || (rm -f $@; exit 1)

ifneq ($(suspend_mod_EXPORTS),no)
def-suspend.lst: pre-suspend.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 suspend/' > $@
endif

und-suspend.lst: pre-suspend.o
	echo 'suspend' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

suspend_mod-commands_ieee1275_suspend.o: commands/ieee1275/suspend.c $(commands/ieee1275/suspend.c_DEPENDENCIES)
	$(TARGET_CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(suspend_mod_CFLAGS) -MD -c -o $@ $<
-include suspend_mod-commands_ieee1275_suspend.d

CLEANFILES += cmd-suspend_mod-commands_ieee1275_suspend.lst fs-suspend_mod-commands_ieee1275_suspend.lst
COMMANDFILES += cmd-suspend_mod-commands_ieee1275_suspend.lst
FSFILES += fs-suspend_mod-commands_ieee1275_suspend.lst

cmd-suspend_mod-commands_ieee1275_suspend.lst: commands/ieee1275/suspend.c $(commands/ieee1275/suspend.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(suspend_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh suspend > $@ || (rm -f $@; exit 1)

fs-suspend_mod-commands_ieee1275_suspend.lst: commands/ieee1275/suspend.c $(commands/ieee1275/suspend.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(suspend_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh suspend > $@ || (rm -f $@; exit 1)


suspend_mod_CFLAGS = $(COMMON_CFLAGS)
suspend_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For reboot.mod
reboot_mod_SOURCES = commands/reboot.c
CLEANFILES += reboot.mod mod-reboot.o mod-reboot.c pre-reboot.o reboot_mod-commands_reboot.o und-reboot.lst
ifneq ($(reboot_mod_EXPORTS),no)
CLEANFILES += def-reboot.lst
DEFSYMFILES += def-reboot.lst
endif
MOSTLYCLEANFILES += reboot_mod-commands_reboot.d
UNDSYMFILES += und-reboot.lst

reboot.mod: pre-reboot.o mod-reboot.o
	-rm -f $@
	$(TARGET_CC) $(reboot_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-reboot.o: $(reboot_mod_DEPENDENCIES) reboot_mod-commands_reboot.o
	-rm -f $@
	$(TARGET_CC) $(reboot_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ reboot_mod-commands_reboot.o

mod-reboot.o: mod-reboot.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(reboot_mod_CFLAGS) -c -o $@ $<

mod-reboot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'reboot' $< > $@ || (rm -f $@; exit 1)

ifneq ($(reboot_mod_EXPORTS),no)
def-reboot.lst: pre-reboot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 reboot/' > $@
endif

und-reboot.lst: pre-reboot.o
	echo 'reboot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

reboot_mod-commands_reboot.o: commands/reboot.c $(commands/reboot.c_DEPENDENCIES)
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(reboot_mod_CFLAGS) -MD -c -o $@ $<
-include reboot_mod-commands_reboot.d

CLEANFILES += cmd-reboot_mod-commands_reboot.lst fs-reboot_mod-commands_reboot.lst
COMMANDFILES += cmd-reboot_mod-commands_reboot.lst
FSFILES += fs-reboot_mod-commands_reboot.lst

cmd-reboot_mod-commands_reboot.lst: commands/reboot.c $(commands/reboot.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(reboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh reboot > $@ || (rm -f $@; exit 1)

fs-reboot_mod-commands_reboot.lst: commands/reboot.c $(commands/reboot.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(reboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh reboot > $@ || (rm -f $@; exit 1)


reboot_mod_CFLAGS = $(COMMON_CFLAGS)
reboot_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For halt.mod
halt_mod_SOURCES = commands/halt.c
CLEANFILES += halt.mod mod-halt.o mod-halt.c pre-halt.o halt_mod-commands_halt.o und-halt.lst
ifneq ($(halt_mod_EXPORTS),no)
CLEANFILES += def-halt.lst
DEFSYMFILES += def-halt.lst
endif
MOSTLYCLEANFILES += halt_mod-commands_halt.d
UNDSYMFILES += und-halt.lst

halt.mod: pre-halt.o mod-halt.o
	-rm -f $@
	$(TARGET_CC) $(halt_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-halt.o: $(halt_mod_DEPENDENCIES) halt_mod-commands_halt.o
	-rm -f $@
	$(TARGET_CC) $(halt_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ halt_mod-commands_halt.o

mod-halt.o: mod-halt.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(halt_mod_CFLAGS) -c -o $@ $<

mod-halt.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'halt' $< > $@ || (rm -f $@; exit 1)

ifneq ($(halt_mod_EXPORTS),no)
def-halt.lst: pre-halt.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 halt/' > $@
endif

und-halt.lst: pre-halt.o
	echo 'halt' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

halt_mod-commands_halt.o: commands/halt.c $(commands/halt.c_DEPENDENCIES)
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(halt_mod_CFLAGS) -MD -c -o $@ $<
-include halt_mod-commands_halt.d

CLEANFILES += cmd-halt_mod-commands_halt.lst fs-halt_mod-commands_halt.lst
COMMANDFILES += cmd-halt_mod-commands_halt.lst
FSFILES += fs-halt_mod-commands_halt.lst

cmd-halt_mod-commands_halt.lst: commands/halt.c $(commands/halt.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(halt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh halt > $@ || (rm -f $@; exit 1)

fs-halt_mod-commands_halt.lst: commands/halt.c $(commands/halt.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(halt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh halt > $@ || (rm -f $@; exit 1)


halt_mod_CFLAGS = $(COMMON_CFLAGS)
halt_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For help.mod.
help_mod_SOURCES = commands/help.c
CLEANFILES += help.mod mod-help.o mod-help.c pre-help.o help_mod-commands_help.o und-help.lst
ifneq ($(help_mod_EXPORTS),no)
CLEANFILES += def-help.lst
DEFSYMFILES += def-help.lst
endif
MOSTLYCLEANFILES += help_mod-commands_help.d
UNDSYMFILES += und-help.lst

help.mod: pre-help.o mod-help.o
	-rm -f $@
	$(TARGET_CC) $(help_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-help.o: $(help_mod_DEPENDENCIES) help_mod-commands_help.o
	-rm -f $@
	$(TARGET_CC) $(help_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ help_mod-commands_help.o

mod-help.o: mod-help.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(help_mod_CFLAGS) -c -o $@ $<

mod-help.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'help' $< > $@ || (rm -f $@; exit 1)

ifneq ($(help_mod_EXPORTS),no)
def-help.lst: pre-help.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 help/' > $@
endif

und-help.lst: pre-help.o
	echo 'help' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

help_mod-commands_help.o: commands/help.c $(commands/help.c_DEPENDENCIES)
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(help_mod_CFLAGS) -MD -c -o $@ $<
-include help_mod-commands_help.d

CLEANFILES += cmd-help_mod-commands_help.lst fs-help_mod-commands_help.lst
COMMANDFILES += cmd-help_mod-commands_help.lst
FSFILES += fs-help_mod-commands_help.lst

cmd-help_mod-commands_help.lst: commands/help.c $(commands/help.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(help_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh help > $@ || (rm -f $@; exit 1)

fs-help_mod-commands_help.lst: commands/help.c $(commands/help.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(help_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh help > $@ || (rm -f $@; exit 1)


help_mod_CFLAGS = $(COMMON_CFLAGS)
help_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For default.mod
default_mod_SOURCES = commands/default.c
default_mod_CFLAGS =  $(COMMON_CFLAGS)
default_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For timeout.mod
timeout_mod_SOURCES = commands/timeout.c
timeout_mod_CFLAGS =  $(COMMON_CFLAGS)
timeout_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For configfile.mod
configfile_mod_SOURCES = commands/configfile.c
CLEANFILES += configfile.mod mod-configfile.o mod-configfile.c pre-configfile.o configfile_mod-commands_configfile.o und-configfile.lst
ifneq ($(configfile_mod_EXPORTS),no)
CLEANFILES += def-configfile.lst
DEFSYMFILES += def-configfile.lst
endif
MOSTLYCLEANFILES += configfile_mod-commands_configfile.d
UNDSYMFILES += und-configfile.lst

configfile.mod: pre-configfile.o mod-configfile.o
	-rm -f $@
	$(TARGET_CC) $(configfile_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-configfile.o: $(configfile_mod_DEPENDENCIES) configfile_mod-commands_configfile.o
	-rm -f $@
	$(TARGET_CC) $(configfile_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ configfile_mod-commands_configfile.o

mod-configfile.o: mod-configfile.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(configfile_mod_CFLAGS) -c -o $@ $<

mod-configfile.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'configfile' $< > $@ || (rm -f $@; exit 1)

ifneq ($(configfile_mod_EXPORTS),no)
def-configfile.lst: pre-configfile.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 configfile/' > $@
endif

und-configfile.lst: pre-configfile.o
	echo 'configfile' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

configfile_mod-commands_configfile.o: commands/configfile.c $(commands/configfile.c_DEPENDENCIES)
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(configfile_mod_CFLAGS) -MD -c -o $@ $<
-include configfile_mod-commands_configfile.d

CLEANFILES += cmd-configfile_mod-commands_configfile.lst fs-configfile_mod-commands_configfile.lst
COMMANDFILES += cmd-configfile_mod-commands_configfile.lst
FSFILES += fs-configfile_mod-commands_configfile.lst

cmd-configfile_mod-commands_configfile.lst: commands/configfile.c $(commands/configfile.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(configfile_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh configfile > $@ || (rm -f $@; exit 1)

fs-configfile_mod-commands_configfile.lst: commands/configfile.c $(commands/configfile.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(configfile_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh configfile > $@ || (rm -f $@; exit 1)


configfile_mod_CFLAGS = $(COMMON_CFLAGS)
configfile_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For search.mod.
search_mod_SOURCES = commands/search.c
CLEANFILES += search.mod mod-search.o mod-search.c pre-search.o search_mod-commands_search.o und-search.lst
ifneq ($(search_mod_EXPORTS),no)
CLEANFILES += def-search.lst
DEFSYMFILES += def-search.lst
endif
MOSTLYCLEANFILES += search_mod-commands_search.d
UNDSYMFILES += und-search.lst

search.mod: pre-search.o mod-search.o
	-rm -f $@
	$(TARGET_CC) $(search_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-search.o: $(search_mod_DEPENDENCIES) search_mod-commands_search.o
	-rm -f $@
	$(TARGET_CC) $(search_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ search_mod-commands_search.o

mod-search.o: mod-search.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(search_mod_CFLAGS) -c -o $@ $<

mod-search.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'search' $< > $@ || (rm -f $@; exit 1)

ifneq ($(search_mod_EXPORTS),no)
def-search.lst: pre-search.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 search/' > $@
endif

und-search.lst: pre-search.o
	echo 'search' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

search_mod-commands_search.o: commands/search.c $(commands/search.c_DEPENDENCIES)
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(search_mod_CFLAGS) -MD -c -o $@ $<
-include search_mod-commands_search.d

CLEANFILES += cmd-search_mod-commands_search.lst fs-search_mod-commands_search.lst
COMMANDFILES += cmd-search_mod-commands_search.lst
FSFILES += fs-search_mod-commands_search.lst

cmd-search_mod-commands_search.lst: commands/search.c $(commands/search.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(search_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh search > $@ || (rm -f $@; exit 1)

fs-search_mod-commands_search.lst: commands/search.c $(commands/search.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(search_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh search > $@ || (rm -f $@; exit 1)


search_mod_CFLAGS = $(COMMON_CFLAGS)
search_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For gzio.mod.
gzio_mod_SOURCES = io/gzio.c
CLEANFILES += gzio.mod mod-gzio.o mod-gzio.c pre-gzio.o gzio_mod-io_gzio.o und-gzio.lst
ifneq ($(gzio_mod_EXPORTS),no)
CLEANFILES += def-gzio.lst
DEFSYMFILES += def-gzio.lst
endif
MOSTLYCLEANFILES += gzio_mod-io_gzio.d
UNDSYMFILES += und-gzio.lst

gzio.mod: pre-gzio.o mod-gzio.o
	-rm -f $@
	$(TARGET_CC) $(gzio_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-gzio.o: $(gzio_mod_DEPENDENCIES) gzio_mod-io_gzio.o
	-rm -f $@
	$(TARGET_CC) $(gzio_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ gzio_mod-io_gzio.o

mod-gzio.o: mod-gzio.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gzio_mod_CFLAGS) -c -o $@ $<

mod-gzio.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'gzio' $< > $@ || (rm -f $@; exit 1)

ifneq ($(gzio_mod_EXPORTS),no)
def-gzio.lst: pre-gzio.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 gzio/' > $@
endif

und-gzio.lst: pre-gzio.o
	echo 'gzio' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

gzio_mod-io_gzio.o: io/gzio.c $(io/gzio.c_DEPENDENCIES)
	$(TARGET_CC) -Iio -I$(srcdir)/io $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(gzio_mod_CFLAGS) -MD -c -o $@ $<
-include gzio_mod-io_gzio.d

CLEANFILES += cmd-gzio_mod-io_gzio.lst fs-gzio_mod-io_gzio.lst
COMMANDFILES += cmd-gzio_mod-io_gzio.lst
FSFILES += fs-gzio_mod-io_gzio.lst

cmd-gzio_mod-io_gzio.lst: io/gzio.c $(io/gzio.c_DEPENDENCIES) gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iio -I$(srcdir)/io $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gzio_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh gzio > $@ || (rm -f $@; exit 1)

fs-gzio_mod-io_gzio.lst: io/gzio.c $(io/gzio.c_DEPENDENCIES) genfslist.sh
	set -e; 	  $(TARGET_CC) -Iio -I$(srcdir)/io $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gzio_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh gzio > $@ || (rm -f $@; exit 1)


gzio_mod_CFLAGS = $(COMMON_CFLAGS)
gzio_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For test.mod.
test_mod_SOURCES = commands/test.c
test_mod_CFLAGS = $(COMMON_CFLAGS)
test_mod_LDFLAGS = $(COMMON_LDFLAGS)
