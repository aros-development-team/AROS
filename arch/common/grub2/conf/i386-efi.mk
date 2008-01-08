# -*- makefile -*-

COMMON_ASFLAGS = -nostdinc -fno-builtin -m32
COMMON_CFLAGS = -fno-builtin -m32
COMMON_LDFLAGS = -melf_i386 -nostdlib

# Utilities.
bin_UTILITIES = grub-mkimage
sbin_UTILITIES = grub-mkdevicemap grub-probe
#ifeq ($(enable_grub_emu), yes)
#sbin_UTILITIES += grub-emu
#endif

# For grub-mkimage.
grub_mkimage_SOURCES = util/i386/efi/grub-mkimage.c util/misc.c \
	util/resolve.c
CLEANFILES += grub-mkimage grub_mkimage-util_i386_efi_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o
MOSTLYCLEANFILES += grub_mkimage-util_i386_efi_grub_mkimage.d grub_mkimage-util_misc.d grub_mkimage-util_resolve.d

grub-mkimage: $(grub_mkimage_DEPENDENCIES) grub_mkimage-util_i386_efi_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o
	$(CC) -o $@ grub_mkimage-util_i386_efi_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o $(LDFLAGS) $(grub_mkimage_LDFLAGS)

grub_mkimage-util_i386_efi_grub_mkimage.o: util/i386/efi/grub-mkimage.c $(util/i386/efi/grub-mkimage.c_DEPENDENCIES)
	$(CC) -Iutil/i386/efi -I$(srcdir)/util/i386/efi $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -MD -c -o $@ $<
-include grub_mkimage-util_i386_efi_grub_mkimage.d

grub_mkimage-util_misc.o: util/misc.c $(util/misc.c_DEPENDENCIES)
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -MD -c -o $@ $<
-include grub_mkimage-util_misc.d

grub_mkimage-util_resolve.o: util/resolve.c $(util/resolve.c_DEPENDENCIES)
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -MD -c -o $@ $<
-include grub_mkimage-util_resolve.d


# For grub-setup.
#grub_setup_SOURCES = util/i386/pc/grub-setup.c util/biosdisk.c	\
#	util/misc.c util/getroot.c kern/device.c kern/disk.c	\
#	kern/err.c kern/misc.c fs/fat.c fs/ext2.c fs/xfs.c fs/affs.c	\
#	fs/sfs.c kern/parser.c kern/partition.c partmap/pc.c		\
#	fs/ufs.c fs/minix.c fs/hfs.c fs/jfs.c fs/hfsplus.c kern/file.c	\
#	kern/fs.c kern/env.c fs/fshelp.c

# For grub-mkdevicemap.
grub_mkdevicemap_SOURCES = util/grub-mkdevicemap.c util/misc.c		\
	util/i386/get_disk_name.c
CLEANFILES += grub-mkdevicemap grub_mkdevicemap-util_grub_mkdevicemap.o grub_mkdevicemap-util_misc.o grub_mkdevicemap-util_i386_get_disk_name.o
MOSTLYCLEANFILES += grub_mkdevicemap-util_grub_mkdevicemap.d grub_mkdevicemap-util_misc.d grub_mkdevicemap-util_i386_get_disk_name.d

grub-mkdevicemap: $(grub_mkdevicemap_DEPENDENCIES) grub_mkdevicemap-util_grub_mkdevicemap.o grub_mkdevicemap-util_misc.o grub_mkdevicemap-util_i386_get_disk_name.o
	$(CC) -o $@ grub_mkdevicemap-util_grub_mkdevicemap.o grub_mkdevicemap-util_misc.o grub_mkdevicemap-util_i386_get_disk_name.o $(LDFLAGS) $(grub_mkdevicemap_LDFLAGS)

grub_mkdevicemap-util_grub_mkdevicemap.o: util/grub-mkdevicemap.c $(util/grub-mkdevicemap.c_DEPENDENCIES)
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_mkdevicemap_CFLAGS) -MD -c -o $@ $<
-include grub_mkdevicemap-util_grub_mkdevicemap.d

grub_mkdevicemap-util_misc.o: util/misc.c $(util/misc.c_DEPENDENCIES)
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_mkdevicemap_CFLAGS) -MD -c -o $@ $<
-include grub_mkdevicemap-util_misc.d

grub_mkdevicemap-util_i386_get_disk_name.o: util/i386/get_disk_name.c $(util/i386/get_disk_name.c_DEPENDENCIES)
	$(CC) -Iutil/i386 -I$(srcdir)/util/i386 $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_mkdevicemap_CFLAGS) -MD -c -o $@ $<
-include grub_mkdevicemap-util_i386_get_disk_name.d


# For grub-probe.
util/grub-probe.c_DEPENDENCIES = grub_probe_init.h
grub_probe_SOURCES = util/grub-probe.c	\
	util/biosdisk.c	util/misc.c util/getroot.c	\
	kern/device.c kern/disk.c kern/err.c kern/misc.c fs/fat.c	\
	fs/ext2.c kern/parser.c kern/partition.c 			\
	partmap/pc.c partmap/apple.c partmap/gpt.c 			\
	fs/ufs.c fs/minix.c fs/hfs.c fs/jfs.c fs/ntfs.c fs/ntfscomp.c	\
	fs/xfs.c fs/affs.c fs/sfs.c fs/hfsplus.c kern/fs.c		\
	kern/env.c fs/fshelp.c 	\
	disk/lvm.c disk/raid.c grub_probe_init.c
CLEANFILES += grub-probe grub_probe-util_grub_probe.o grub_probe-util_biosdisk.o grub_probe-util_misc.o grub_probe-util_getroot.o grub_probe-kern_device.o grub_probe-kern_disk.o grub_probe-kern_err.o grub_probe-kern_misc.o grub_probe-fs_fat.o grub_probe-fs_ext2.o grub_probe-kern_parser.o grub_probe-kern_partition.o grub_probe-partmap_pc.o grub_probe-partmap_apple.o grub_probe-partmap_gpt.o grub_probe-fs_ufs.o grub_probe-fs_minix.o grub_probe-fs_hfs.o grub_probe-fs_jfs.o grub_probe-fs_ntfs.o grub_probe-fs_ntfscomp.o grub_probe-fs_xfs.o grub_probe-fs_affs.o grub_probe-fs_sfs.o grub_probe-fs_hfsplus.o grub_probe-kern_fs.o grub_probe-kern_env.o grub_probe-fs_fshelp.o grub_probe-disk_lvm.o grub_probe-disk_raid.o grub_probe-grub_probe_init.o
MOSTLYCLEANFILES += grub_probe-util_grub_probe.d grub_probe-util_biosdisk.d grub_probe-util_misc.d grub_probe-util_getroot.d grub_probe-kern_device.d grub_probe-kern_disk.d grub_probe-kern_err.d grub_probe-kern_misc.d grub_probe-fs_fat.d grub_probe-fs_ext2.d grub_probe-kern_parser.d grub_probe-kern_partition.d grub_probe-partmap_pc.d grub_probe-partmap_apple.d grub_probe-partmap_gpt.d grub_probe-fs_ufs.d grub_probe-fs_minix.d grub_probe-fs_hfs.d grub_probe-fs_jfs.d grub_probe-fs_ntfs.d grub_probe-fs_ntfscomp.d grub_probe-fs_xfs.d grub_probe-fs_affs.d grub_probe-fs_sfs.d grub_probe-fs_hfsplus.d grub_probe-kern_fs.d grub_probe-kern_env.d grub_probe-fs_fshelp.d grub_probe-disk_lvm.d grub_probe-disk_raid.d grub_probe-grub_probe_init.d

grub-probe: $(grub_probe_DEPENDENCIES) grub_probe-util_grub_probe.o grub_probe-util_biosdisk.o grub_probe-util_misc.o grub_probe-util_getroot.o grub_probe-kern_device.o grub_probe-kern_disk.o grub_probe-kern_err.o grub_probe-kern_misc.o grub_probe-fs_fat.o grub_probe-fs_ext2.o grub_probe-kern_parser.o grub_probe-kern_partition.o grub_probe-partmap_pc.o grub_probe-partmap_apple.o grub_probe-partmap_gpt.o grub_probe-fs_ufs.o grub_probe-fs_minix.o grub_probe-fs_hfs.o grub_probe-fs_jfs.o grub_probe-fs_ntfs.o grub_probe-fs_ntfscomp.o grub_probe-fs_xfs.o grub_probe-fs_affs.o grub_probe-fs_sfs.o grub_probe-fs_hfsplus.o grub_probe-kern_fs.o grub_probe-kern_env.o grub_probe-fs_fshelp.o grub_probe-disk_lvm.o grub_probe-disk_raid.o grub_probe-grub_probe_init.o
	$(CC) -o $@ grub_probe-util_grub_probe.o grub_probe-util_biosdisk.o grub_probe-util_misc.o grub_probe-util_getroot.o grub_probe-kern_device.o grub_probe-kern_disk.o grub_probe-kern_err.o grub_probe-kern_misc.o grub_probe-fs_fat.o grub_probe-fs_ext2.o grub_probe-kern_parser.o grub_probe-kern_partition.o grub_probe-partmap_pc.o grub_probe-partmap_apple.o grub_probe-partmap_gpt.o grub_probe-fs_ufs.o grub_probe-fs_minix.o grub_probe-fs_hfs.o grub_probe-fs_jfs.o grub_probe-fs_ntfs.o grub_probe-fs_ntfscomp.o grub_probe-fs_xfs.o grub_probe-fs_affs.o grub_probe-fs_sfs.o grub_probe-fs_hfsplus.o grub_probe-kern_fs.o grub_probe-kern_env.o grub_probe-fs_fshelp.o grub_probe-disk_lvm.o grub_probe-disk_raid.o grub_probe-grub_probe_init.o $(LDFLAGS) $(grub_probe_LDFLAGS)

grub_probe-util_grub_probe.o: util/grub-probe.c $(util/grub-probe.c_DEPENDENCIES)
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-util_grub_probe.d

grub_probe-util_biosdisk.o: util/biosdisk.c $(util/biosdisk.c_DEPENDENCIES)
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-util_biosdisk.d

grub_probe-util_misc.o: util/misc.c $(util/misc.c_DEPENDENCIES)
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-util_misc.d

grub_probe-util_getroot.o: util/getroot.c $(util/getroot.c_DEPENDENCIES)
	$(CC) -Iutil -I$(srcdir)/util $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-util_getroot.d

grub_probe-kern_device.o: kern/device.c $(kern/device.c_DEPENDENCIES)
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_device.d

grub_probe-kern_disk.o: kern/disk.c $(kern/disk.c_DEPENDENCIES)
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_disk.d

grub_probe-kern_err.o: kern/err.c $(kern/err.c_DEPENDENCIES)
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_err.d

grub_probe-kern_misc.o: kern/misc.c $(kern/misc.c_DEPENDENCIES)
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_misc.d

grub_probe-fs_fat.o: fs/fat.c $(fs/fat.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_fat.d

grub_probe-fs_ext2.o: fs/ext2.c $(fs/ext2.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_ext2.d

grub_probe-kern_parser.o: kern/parser.c $(kern/parser.c_DEPENDENCIES)
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_parser.d

grub_probe-kern_partition.o: kern/partition.c $(kern/partition.c_DEPENDENCIES)
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_partition.d

grub_probe-partmap_pc.o: partmap/pc.c $(partmap/pc.c_DEPENDENCIES)
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-partmap_pc.d

grub_probe-partmap_apple.o: partmap/apple.c $(partmap/apple.c_DEPENDENCIES)
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-partmap_apple.d

grub_probe-partmap_gpt.o: partmap/gpt.c $(partmap/gpt.c_DEPENDENCIES)
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-partmap_gpt.d

grub_probe-fs_ufs.o: fs/ufs.c $(fs/ufs.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_ufs.d

grub_probe-fs_minix.o: fs/minix.c $(fs/minix.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_minix.d

grub_probe-fs_hfs.o: fs/hfs.c $(fs/hfs.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_hfs.d

grub_probe-fs_jfs.o: fs/jfs.c $(fs/jfs.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_jfs.d

grub_probe-fs_ntfs.o: fs/ntfs.c $(fs/ntfs.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_ntfs.d

grub_probe-fs_ntfscomp.o: fs/ntfscomp.c $(fs/ntfscomp.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_ntfscomp.d

grub_probe-fs_xfs.o: fs/xfs.c $(fs/xfs.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_xfs.d

grub_probe-fs_affs.o: fs/affs.c $(fs/affs.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_affs.d

grub_probe-fs_sfs.o: fs/sfs.c $(fs/sfs.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_sfs.d

grub_probe-fs_hfsplus.o: fs/hfsplus.c $(fs/hfsplus.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_hfsplus.d

grub_probe-kern_fs.o: kern/fs.c $(kern/fs.c_DEPENDENCIES)
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_fs.d

grub_probe-kern_env.o: kern/env.c $(kern/env.c_DEPENDENCIES)
	$(CC) -Ikern -I$(srcdir)/kern $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-kern_env.d

grub_probe-fs_fshelp.o: fs/fshelp.c $(fs/fshelp.c_DEPENDENCIES)
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-fs_fshelp.d

grub_probe-disk_lvm.o: disk/lvm.c $(disk/lvm.c_DEPENDENCIES)
	$(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-disk_lvm.d

grub_probe-disk_raid.o: disk/raid.c $(disk/raid.c_DEPENDENCIES)
	$(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-disk_raid.d

grub_probe-grub_probe_init.o: grub_probe_init.c $(grub_probe_init.c_DEPENDENCIES)
	$(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(grub_probe_CFLAGS) -MD -c -o $@ $<
-include grub_probe-grub_probe_init.d


# For grub-emu.
grub_emu_DEPENDENCIES = grub_script.tab.c grub_script.tab.h		\
	grub_emu_init.h
grub_emu_SOURCES = commands/boot.c commands/cat.c commands/cmp.c 	\
	commands/configfile.c commands/help.c				\
	commands/terminal.c commands/ls.c commands/test.c 		\
	commands/search.c commands/hexdump.c				\
	commands/i386/pc/halt.c commands/i386/pc/reboot.c		\
	commands/i386/cpuid.c						\
	disk/loopback.c							\
	fs/affs.c fs/ext2.c fs/fat.c fs/fshelp.c fs/hfs.c fs/iso9660.c	\
	fs/jfs.c fs/minix.c fs/sfs.c fs/ufs.c fs/xfs.c fs/hfsplus.c	\
	fs/ntfs.c fs/ntfscomp.c	fs/cpio.c				\
	io/gzio.c							\
	kern/device.c kern/disk.c kern/dl.c kern/elf.c kern/env.c	\
	kern/err.c							\
	normal/execute.c kern/file.c kern/fs.c normal/lexer.c 		\
	kern/loader.c kern/main.c kern/misc.c kern/parser.c		\
	grub_script.tab.c kern/partition.c kern/rescue.c kern/term.c	\
	normal/arg.c normal/cmdline.c normal/command.c normal/function.c\
	normal/completion.c normal/context.c normal/main.c		\
	normal/menu.c normal/menu_entry.c normal/misc.c normal/script.c	\
	normal/color.c							\
	partmap/amiga.c	partmap/apple.c partmap/pc.c partmap/sun.c	\
	partmap/acorn.c partmap/gpt.c					\
	util/console.c util/hostfs.c util/grub-emu.c util/misc.c	\
	util/biosdisk.c util/getroot.c			\
	util/i386/pc/misc.c grub_emu_init.c

grub_emu_LDFLAGS = $(LIBCURSES)

# Scripts.
sbin_SCRIPTS = grub-install

# For grub-install.
grub_install_SOURCES = util/i386/efi/grub-install.in
CLEANFILES += grub-install

grub-install: util/i386/efi/grub-install.in config.status
	./config.status --file=grub-install:util/i386/efi/grub-install.in
	chmod +x $@


# Modules.
pkglib_MODULES = kernel.mod normal.mod _chain.mod chain.mod \
	_linux.mod linux.mod cpuid.mod

# For kernel.mod.
kernel_mod_EXPORTS = no
kernel_mod_SOURCES = kern/i386/efi/startup.S kern/main.c kern/device.c \
	kern/disk.c kern/dl.c kern/file.c kern/fs.c kern/err.c \
	kern/misc.c kern/mm.c kern/loader.c kern/rescue.c kern/term.c \
	kern/i386/dl.c kern/i386/efi/init.c kern/parser.c kern/partition.c \
	kern/env.c symlist.c kern/efi/efi.c kern/efi/init.c kern/efi/mm.c \
	term/efi/console.c disk/efi/efidisk.c
CLEANFILES += kernel.mod mod-kernel.o mod-kernel.c pre-kernel.o kernel_mod-kern_i386_efi_startup.o kernel_mod-kern_main.o kernel_mod-kern_device.o kernel_mod-kern_disk.o kernel_mod-kern_dl.o kernel_mod-kern_file.o kernel_mod-kern_fs.o kernel_mod-kern_err.o kernel_mod-kern_misc.o kernel_mod-kern_mm.o kernel_mod-kern_loader.o kernel_mod-kern_rescue.o kernel_mod-kern_term.o kernel_mod-kern_i386_dl.o kernel_mod-kern_i386_efi_init.o kernel_mod-kern_parser.o kernel_mod-kern_partition.o kernel_mod-kern_env.o kernel_mod-symlist.o kernel_mod-kern_efi_efi.o kernel_mod-kern_efi_init.o kernel_mod-kern_efi_mm.o kernel_mod-term_efi_console.o kernel_mod-disk_efi_efidisk.o und-kernel.lst
ifneq ($(kernel_mod_EXPORTS),no)
CLEANFILES += def-kernel.lst
DEFSYMFILES += def-kernel.lst
endif
MOSTLYCLEANFILES += kernel_mod-kern_i386_efi_startup.d kernel_mod-kern_main.d kernel_mod-kern_device.d kernel_mod-kern_disk.d kernel_mod-kern_dl.d kernel_mod-kern_file.d kernel_mod-kern_fs.d kernel_mod-kern_err.d kernel_mod-kern_misc.d kernel_mod-kern_mm.d kernel_mod-kern_loader.d kernel_mod-kern_rescue.d kernel_mod-kern_term.d kernel_mod-kern_i386_dl.d kernel_mod-kern_i386_efi_init.d kernel_mod-kern_parser.d kernel_mod-kern_partition.d kernel_mod-kern_env.d kernel_mod-symlist.d kernel_mod-kern_efi_efi.d kernel_mod-kern_efi_init.d kernel_mod-kern_efi_mm.d kernel_mod-term_efi_console.d kernel_mod-disk_efi_efidisk.d
UNDSYMFILES += und-kernel.lst

kernel.mod: pre-kernel.o mod-kernel.o
	-rm -f $@
	$(TARGET_CC) $(kernel_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-kernel.o: $(kernel_mod_DEPENDENCIES) kernel_mod-kern_i386_efi_startup.o kernel_mod-kern_main.o kernel_mod-kern_device.o kernel_mod-kern_disk.o kernel_mod-kern_dl.o kernel_mod-kern_file.o kernel_mod-kern_fs.o kernel_mod-kern_err.o kernel_mod-kern_misc.o kernel_mod-kern_mm.o kernel_mod-kern_loader.o kernel_mod-kern_rescue.o kernel_mod-kern_term.o kernel_mod-kern_i386_dl.o kernel_mod-kern_i386_efi_init.o kernel_mod-kern_parser.o kernel_mod-kern_partition.o kernel_mod-kern_env.o kernel_mod-symlist.o kernel_mod-kern_efi_efi.o kernel_mod-kern_efi_init.o kernel_mod-kern_efi_mm.o kernel_mod-term_efi_console.o kernel_mod-disk_efi_efidisk.o
	-rm -f $@
	$(TARGET_CC) $(kernel_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ kernel_mod-kern_i386_efi_startup.o kernel_mod-kern_main.o kernel_mod-kern_device.o kernel_mod-kern_disk.o kernel_mod-kern_dl.o kernel_mod-kern_file.o kernel_mod-kern_fs.o kernel_mod-kern_err.o kernel_mod-kern_misc.o kernel_mod-kern_mm.o kernel_mod-kern_loader.o kernel_mod-kern_rescue.o kernel_mod-kern_term.o kernel_mod-kern_i386_dl.o kernel_mod-kern_i386_efi_init.o kernel_mod-kern_parser.o kernel_mod-kern_partition.o kernel_mod-kern_env.o kernel_mod-symlist.o kernel_mod-kern_efi_efi.o kernel_mod-kern_efi_init.o kernel_mod-kern_efi_mm.o kernel_mod-term_efi_console.o kernel_mod-disk_efi_efidisk.o

mod-kernel.o: mod-kernel.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -c -o $@ $<

mod-kernel.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'kernel' $< > $@ || (rm -f $@; exit 1)

ifneq ($(kernel_mod_EXPORTS),no)
def-kernel.lst: pre-kernel.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 kernel/' > $@
endif

und-kernel.lst: pre-kernel.o
	echo 'kernel' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

kernel_mod-kern_i386_efi_startup.o: kern/i386/efi/startup.S
	$(TARGET_CC) -Ikern/i386/efi -I$(srcdir)/kern/i386/efi $(TARGET_CPPFLAGS) -DASM_FILE=1 $(TARGET_ASFLAGS) $(kernel_mod_ASFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_i386_efi_startup.d

CLEANFILES += cmd-kernel_mod-kern_i386_efi_startup.lst fs-kernel_mod-kern_i386_efi_startup.lst
COMMANDFILES += cmd-kernel_mod-kern_i386_efi_startup.lst
FSFILES += fs-kernel_mod-kern_i386_efi_startup.lst

cmd-kernel_mod-kern_i386_efi_startup.lst: kern/i386/efi/startup.S gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern/i386/efi -I$(srcdir)/kern/i386/efi $(TARGET_CPPFLAGS) $(TARGET_ASFLAGS) $(kernel_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_i386_efi_startup.lst: kern/i386/efi/startup.S genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern/i386/efi -I$(srcdir)/kern/i386/efi $(TARGET_CPPFLAGS) $(TARGET_ASFLAGS) $(kernel_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_main.o: kern/main.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_main.d

CLEANFILES += cmd-kernel_mod-kern_main.lst fs-kernel_mod-kern_main.lst
COMMANDFILES += cmd-kernel_mod-kern_main.lst
FSFILES += fs-kernel_mod-kern_main.lst

cmd-kernel_mod-kern_main.lst: kern/main.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_main.lst: kern/main.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_device.o: kern/device.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_device.d

CLEANFILES += cmd-kernel_mod-kern_device.lst fs-kernel_mod-kern_device.lst
COMMANDFILES += cmd-kernel_mod-kern_device.lst
FSFILES += fs-kernel_mod-kern_device.lst

cmd-kernel_mod-kern_device.lst: kern/device.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_device.lst: kern/device.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_disk.o: kern/disk.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_disk.d

CLEANFILES += cmd-kernel_mod-kern_disk.lst fs-kernel_mod-kern_disk.lst
COMMANDFILES += cmd-kernel_mod-kern_disk.lst
FSFILES += fs-kernel_mod-kern_disk.lst

cmd-kernel_mod-kern_disk.lst: kern/disk.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_disk.lst: kern/disk.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_dl.o: kern/dl.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_dl.d

CLEANFILES += cmd-kernel_mod-kern_dl.lst fs-kernel_mod-kern_dl.lst
COMMANDFILES += cmd-kernel_mod-kern_dl.lst
FSFILES += fs-kernel_mod-kern_dl.lst

cmd-kernel_mod-kern_dl.lst: kern/dl.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_dl.lst: kern/dl.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_file.o: kern/file.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_file.d

CLEANFILES += cmd-kernel_mod-kern_file.lst fs-kernel_mod-kern_file.lst
COMMANDFILES += cmd-kernel_mod-kern_file.lst
FSFILES += fs-kernel_mod-kern_file.lst

cmd-kernel_mod-kern_file.lst: kern/file.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_file.lst: kern/file.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_fs.o: kern/fs.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_fs.d

CLEANFILES += cmd-kernel_mod-kern_fs.lst fs-kernel_mod-kern_fs.lst
COMMANDFILES += cmd-kernel_mod-kern_fs.lst
FSFILES += fs-kernel_mod-kern_fs.lst

cmd-kernel_mod-kern_fs.lst: kern/fs.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_fs.lst: kern/fs.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_err.o: kern/err.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_err.d

CLEANFILES += cmd-kernel_mod-kern_err.lst fs-kernel_mod-kern_err.lst
COMMANDFILES += cmd-kernel_mod-kern_err.lst
FSFILES += fs-kernel_mod-kern_err.lst

cmd-kernel_mod-kern_err.lst: kern/err.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_err.lst: kern/err.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_misc.o: kern/misc.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_misc.d

CLEANFILES += cmd-kernel_mod-kern_misc.lst fs-kernel_mod-kern_misc.lst
COMMANDFILES += cmd-kernel_mod-kern_misc.lst
FSFILES += fs-kernel_mod-kern_misc.lst

cmd-kernel_mod-kern_misc.lst: kern/misc.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_misc.lst: kern/misc.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_mm.o: kern/mm.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_mm.d

CLEANFILES += cmd-kernel_mod-kern_mm.lst fs-kernel_mod-kern_mm.lst
COMMANDFILES += cmd-kernel_mod-kern_mm.lst
FSFILES += fs-kernel_mod-kern_mm.lst

cmd-kernel_mod-kern_mm.lst: kern/mm.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_mm.lst: kern/mm.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_loader.o: kern/loader.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_loader.d

CLEANFILES += cmd-kernel_mod-kern_loader.lst fs-kernel_mod-kern_loader.lst
COMMANDFILES += cmd-kernel_mod-kern_loader.lst
FSFILES += fs-kernel_mod-kern_loader.lst

cmd-kernel_mod-kern_loader.lst: kern/loader.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_loader.lst: kern/loader.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_rescue.o: kern/rescue.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_rescue.d

CLEANFILES += cmd-kernel_mod-kern_rescue.lst fs-kernel_mod-kern_rescue.lst
COMMANDFILES += cmd-kernel_mod-kern_rescue.lst
FSFILES += fs-kernel_mod-kern_rescue.lst

cmd-kernel_mod-kern_rescue.lst: kern/rescue.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_rescue.lst: kern/rescue.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_term.o: kern/term.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_term.d

CLEANFILES += cmd-kernel_mod-kern_term.lst fs-kernel_mod-kern_term.lst
COMMANDFILES += cmd-kernel_mod-kern_term.lst
FSFILES += fs-kernel_mod-kern_term.lst

cmd-kernel_mod-kern_term.lst: kern/term.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_term.lst: kern/term.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_i386_dl.o: kern/i386/dl.c
	$(TARGET_CC) -Ikern/i386 -I$(srcdir)/kern/i386 $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_i386_dl.d

CLEANFILES += cmd-kernel_mod-kern_i386_dl.lst fs-kernel_mod-kern_i386_dl.lst
COMMANDFILES += cmd-kernel_mod-kern_i386_dl.lst
FSFILES += fs-kernel_mod-kern_i386_dl.lst

cmd-kernel_mod-kern_i386_dl.lst: kern/i386/dl.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern/i386 -I$(srcdir)/kern/i386 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_i386_dl.lst: kern/i386/dl.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern/i386 -I$(srcdir)/kern/i386 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_i386_efi_init.o: kern/i386/efi/init.c
	$(TARGET_CC) -Ikern/i386/efi -I$(srcdir)/kern/i386/efi $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_i386_efi_init.d

CLEANFILES += cmd-kernel_mod-kern_i386_efi_init.lst fs-kernel_mod-kern_i386_efi_init.lst
COMMANDFILES += cmd-kernel_mod-kern_i386_efi_init.lst
FSFILES += fs-kernel_mod-kern_i386_efi_init.lst

cmd-kernel_mod-kern_i386_efi_init.lst: kern/i386/efi/init.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern/i386/efi -I$(srcdir)/kern/i386/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_i386_efi_init.lst: kern/i386/efi/init.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern/i386/efi -I$(srcdir)/kern/i386/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_parser.o: kern/parser.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_parser.d

CLEANFILES += cmd-kernel_mod-kern_parser.lst fs-kernel_mod-kern_parser.lst
COMMANDFILES += cmd-kernel_mod-kern_parser.lst
FSFILES += fs-kernel_mod-kern_parser.lst

cmd-kernel_mod-kern_parser.lst: kern/parser.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_parser.lst: kern/parser.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_partition.o: kern/partition.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_partition.d

CLEANFILES += cmd-kernel_mod-kern_partition.lst fs-kernel_mod-kern_partition.lst
COMMANDFILES += cmd-kernel_mod-kern_partition.lst
FSFILES += fs-kernel_mod-kern_partition.lst

cmd-kernel_mod-kern_partition.lst: kern/partition.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_partition.lst: kern/partition.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_env.o: kern/env.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_env.d

CLEANFILES += cmd-kernel_mod-kern_env.lst fs-kernel_mod-kern_env.lst
COMMANDFILES += cmd-kernel_mod-kern_env.lst
FSFILES += fs-kernel_mod-kern_env.lst

cmd-kernel_mod-kern_env.lst: kern/env.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_env.lst: kern/env.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-symlist.o: symlist.c
	$(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-symlist.d

CLEANFILES += cmd-kernel_mod-symlist.lst fs-kernel_mod-symlist.lst
COMMANDFILES += cmd-kernel_mod-symlist.lst
FSFILES += fs-kernel_mod-symlist.lst

cmd-kernel_mod-symlist.lst: symlist.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-symlist.lst: symlist.c genfslist.sh
	set -e; 	  $(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_efi_efi.o: kern/efi/efi.c
	$(TARGET_CC) -Ikern/efi -I$(srcdir)/kern/efi $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_efi_efi.d

CLEANFILES += cmd-kernel_mod-kern_efi_efi.lst fs-kernel_mod-kern_efi_efi.lst
COMMANDFILES += cmd-kernel_mod-kern_efi_efi.lst
FSFILES += fs-kernel_mod-kern_efi_efi.lst

cmd-kernel_mod-kern_efi_efi.lst: kern/efi/efi.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern/efi -I$(srcdir)/kern/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_efi_efi.lst: kern/efi/efi.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern/efi -I$(srcdir)/kern/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_efi_init.o: kern/efi/init.c
	$(TARGET_CC) -Ikern/efi -I$(srcdir)/kern/efi $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_efi_init.d

CLEANFILES += cmd-kernel_mod-kern_efi_init.lst fs-kernel_mod-kern_efi_init.lst
COMMANDFILES += cmd-kernel_mod-kern_efi_init.lst
FSFILES += fs-kernel_mod-kern_efi_init.lst

cmd-kernel_mod-kern_efi_init.lst: kern/efi/init.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern/efi -I$(srcdir)/kern/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_efi_init.lst: kern/efi/init.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern/efi -I$(srcdir)/kern/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-kern_efi_mm.o: kern/efi/mm.c
	$(TARGET_CC) -Ikern/efi -I$(srcdir)/kern/efi $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-kern_efi_mm.d

CLEANFILES += cmd-kernel_mod-kern_efi_mm.lst fs-kernel_mod-kern_efi_mm.lst
COMMANDFILES += cmd-kernel_mod-kern_efi_mm.lst
FSFILES += fs-kernel_mod-kern_efi_mm.lst

cmd-kernel_mod-kern_efi_mm.lst: kern/efi/mm.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern/efi -I$(srcdir)/kern/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-kern_efi_mm.lst: kern/efi/mm.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern/efi -I$(srcdir)/kern/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-term_efi_console.o: term/efi/console.c
	$(TARGET_CC) -Iterm/efi -I$(srcdir)/term/efi $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-term_efi_console.d

CLEANFILES += cmd-kernel_mod-term_efi_console.lst fs-kernel_mod-term_efi_console.lst
COMMANDFILES += cmd-kernel_mod-term_efi_console.lst
FSFILES += fs-kernel_mod-term_efi_console.lst

cmd-kernel_mod-term_efi_console.lst: term/efi/console.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iterm/efi -I$(srcdir)/term/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-term_efi_console.lst: term/efi/console.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iterm/efi -I$(srcdir)/term/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod-disk_efi_efidisk.o: disk/efi/efidisk.c
	$(TARGET_CC) -Idisk/efi -I$(srcdir)/disk/efi $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -MD -c -o $@ $<
-include kernel_mod-disk_efi_efidisk.d

CLEANFILES += cmd-kernel_mod-disk_efi_efidisk.lst fs-kernel_mod-disk_efi_efidisk.lst
COMMANDFILES += cmd-kernel_mod-disk_efi_efidisk.lst
FSFILES += fs-kernel_mod-disk_efi_efidisk.lst

cmd-kernel_mod-disk_efi_efidisk.lst: disk/efi/efidisk.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Idisk/efi -I$(srcdir)/disk/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh kernel > $@ || (rm -f $@; exit 1)

fs-kernel_mod-disk_efi_efidisk.lst: disk/efi/efidisk.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Idisk/efi -I$(srcdir)/disk/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(kernel_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh kernel > $@ || (rm -f $@; exit 1)


kernel_mod_HEADERS = arg.h boot.h cache.h device.h disk.h dl.h elf.h elfload.h \
	env.h err.h file.h fs.h kernel.h loader.h misc.h mm.h net.h parser.h \
	partition.h pc_partition.h rescue.h symbol.h term.h time.h types.h \
	efi/efi.h efi/time.h efi/disk.h
kernel_mod_CFLAGS = $(COMMON_CFLAGS)
kernel_mod_ASFLAGS = $(COMMON_ASFLAGS)
kernel_mod_LDFLAGS = $(COMMON_LDFLAGS)

MOSTLYCLEANFILES += symlist.c
MOSTLYCLEANFILES += symlist.c kernel_syms.lst
DEFSYMFILES += kernel_syms.lst

symlist.c: $(addprefix include/grub/,$(kernel_mod_HEADERS)) config.h gensymlist.sh
	/bin/sh gensymlist.sh $(filter %.h,$^) > $@ || (rm -f $@; exit 1)

kernel_syms.lst: $(addprefix include/grub/,$(kernel_mod_HEADERS)) config.h genkernsyms.sh
	/bin/sh genkernsyms.sh $(filter %.h,$^) > $@ || (rm -f $@; exit 1)

# For normal.mod.
normal_mod_DEPENDENCIES = grub_script.tab.c grub_script.tab.h
normal_mod_SOURCES = normal/arg.c normal/cmdline.c normal/command.c	\
	normal/completion.c normal/execute.c 		\
	normal/function.c normal/lexer.c normal/main.c normal/menu.c	\
	normal/menu_entry.c normal/misc.c grub_script.tab.c 		\
	normal/script.c normal/i386/setjmp.S normal/color.c
CLEANFILES += normal.mod mod-normal.o mod-normal.c pre-normal.o normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-grub_script_tab.o normal_mod-normal_script.o normal_mod-normal_i386_setjmp.o normal_mod-normal_color.o und-normal.lst
ifneq ($(normal_mod_EXPORTS),no)
CLEANFILES += def-normal.lst
DEFSYMFILES += def-normal.lst
endif
MOSTLYCLEANFILES += normal_mod-normal_arg.d normal_mod-normal_cmdline.d normal_mod-normal_command.d normal_mod-normal_completion.d normal_mod-normal_execute.d normal_mod-normal_function.d normal_mod-normal_lexer.d normal_mod-normal_main.d normal_mod-normal_menu.d normal_mod-normal_menu_entry.d normal_mod-normal_misc.d normal_mod-grub_script_tab.d normal_mod-normal_script.d normal_mod-normal_i386_setjmp.d normal_mod-normal_color.d
UNDSYMFILES += und-normal.lst

normal.mod: pre-normal.o mod-normal.o
	-rm -f $@
	$(TARGET_CC) $(normal_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-normal.o: $(normal_mod_DEPENDENCIES) normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-grub_script_tab.o normal_mod-normal_script.o normal_mod-normal_i386_setjmp.o normal_mod-normal_color.o
	-rm -f $@
	$(TARGET_CC) $(normal_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_execute.o normal_mod-normal_function.o normal_mod-normal_lexer.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-grub_script_tab.o normal_mod-normal_script.o normal_mod-normal_i386_setjmp.o normal_mod-normal_color.o

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

normal_mod-normal_arg.o: normal/arg.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_arg.d

CLEANFILES += cmd-normal_mod-normal_arg.lst fs-normal_mod-normal_arg.lst
COMMANDFILES += cmd-normal_mod-normal_arg.lst
FSFILES += fs-normal_mod-normal_arg.lst

cmd-normal_mod-normal_arg.lst: normal/arg.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_arg.lst: normal/arg.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_cmdline.o: normal/cmdline.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_cmdline.d

CLEANFILES += cmd-normal_mod-normal_cmdline.lst fs-normal_mod-normal_cmdline.lst
COMMANDFILES += cmd-normal_mod-normal_cmdline.lst
FSFILES += fs-normal_mod-normal_cmdline.lst

cmd-normal_mod-normal_cmdline.lst: normal/cmdline.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_cmdline.lst: normal/cmdline.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_command.o: normal/command.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_command.d

CLEANFILES += cmd-normal_mod-normal_command.lst fs-normal_mod-normal_command.lst
COMMANDFILES += cmd-normal_mod-normal_command.lst
FSFILES += fs-normal_mod-normal_command.lst

cmd-normal_mod-normal_command.lst: normal/command.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_command.lst: normal/command.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_completion.o: normal/completion.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_completion.d

CLEANFILES += cmd-normal_mod-normal_completion.lst fs-normal_mod-normal_completion.lst
COMMANDFILES += cmd-normal_mod-normal_completion.lst
FSFILES += fs-normal_mod-normal_completion.lst

cmd-normal_mod-normal_completion.lst: normal/completion.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_completion.lst: normal/completion.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_execute.o: normal/execute.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_execute.d

CLEANFILES += cmd-normal_mod-normal_execute.lst fs-normal_mod-normal_execute.lst
COMMANDFILES += cmd-normal_mod-normal_execute.lst
FSFILES += fs-normal_mod-normal_execute.lst

cmd-normal_mod-normal_execute.lst: normal/execute.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_execute.lst: normal/execute.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_function.o: normal/function.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_function.d

CLEANFILES += cmd-normal_mod-normal_function.lst fs-normal_mod-normal_function.lst
COMMANDFILES += cmd-normal_mod-normal_function.lst
FSFILES += fs-normal_mod-normal_function.lst

cmd-normal_mod-normal_function.lst: normal/function.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_function.lst: normal/function.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_lexer.o: normal/lexer.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_lexer.d

CLEANFILES += cmd-normal_mod-normal_lexer.lst fs-normal_mod-normal_lexer.lst
COMMANDFILES += cmd-normal_mod-normal_lexer.lst
FSFILES += fs-normal_mod-normal_lexer.lst

cmd-normal_mod-normal_lexer.lst: normal/lexer.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_lexer.lst: normal/lexer.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_main.o: normal/main.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_main.d

CLEANFILES += cmd-normal_mod-normal_main.lst fs-normal_mod-normal_main.lst
COMMANDFILES += cmd-normal_mod-normal_main.lst
FSFILES += fs-normal_mod-normal_main.lst

cmd-normal_mod-normal_main.lst: normal/main.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_main.lst: normal/main.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_menu.o: normal/menu.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_menu.d

CLEANFILES += cmd-normal_mod-normal_menu.lst fs-normal_mod-normal_menu.lst
COMMANDFILES += cmd-normal_mod-normal_menu.lst
FSFILES += fs-normal_mod-normal_menu.lst

cmd-normal_mod-normal_menu.lst: normal/menu.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_menu.lst: normal/menu.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_menu_entry.o: normal/menu_entry.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_menu_entry.d

CLEANFILES += cmd-normal_mod-normal_menu_entry.lst fs-normal_mod-normal_menu_entry.lst
COMMANDFILES += cmd-normal_mod-normal_menu_entry.lst
FSFILES += fs-normal_mod-normal_menu_entry.lst

cmd-normal_mod-normal_menu_entry.lst: normal/menu_entry.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_menu_entry.lst: normal/menu_entry.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_misc.o: normal/misc.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_misc.d

CLEANFILES += cmd-normal_mod-normal_misc.lst fs-normal_mod-normal_misc.lst
COMMANDFILES += cmd-normal_mod-normal_misc.lst
FSFILES += fs-normal_mod-normal_misc.lst

cmd-normal_mod-normal_misc.lst: normal/misc.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_misc.lst: normal/misc.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-grub_script_tab.o: grub_script.tab.c
	$(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-grub_script_tab.d

CLEANFILES += cmd-normal_mod-grub_script_tab.lst fs-normal_mod-grub_script_tab.lst
COMMANDFILES += cmd-normal_mod-grub_script_tab.lst
FSFILES += fs-normal_mod-grub_script_tab.lst

cmd-normal_mod-grub_script_tab.lst: grub_script.tab.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-grub_script_tab.lst: grub_script.tab.c genfslist.sh
	set -e; 	  $(TARGET_CC) -I. -I$(srcdir)/. $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_script.o: normal/script.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_script.d

CLEANFILES += cmd-normal_mod-normal_script.lst fs-normal_mod-normal_script.lst
COMMANDFILES += cmd-normal_mod-normal_script.lst
FSFILES += fs-normal_mod-normal_script.lst

cmd-normal_mod-normal_script.lst: normal/script.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_script.lst: normal/script.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_i386_setjmp.o: normal/i386/setjmp.S
	$(TARGET_CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(TARGET_CPPFLAGS) -DASM_FILE=1 $(TARGET_ASFLAGS) $(normal_mod_ASFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_i386_setjmp.d

CLEANFILES += cmd-normal_mod-normal_i386_setjmp.lst fs-normal_mod-normal_i386_setjmp.lst
COMMANDFILES += cmd-normal_mod-normal_i386_setjmp.lst
FSFILES += fs-normal_mod-normal_i386_setjmp.lst

cmd-normal_mod-normal_i386_setjmp.lst: normal/i386/setjmp.S gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(TARGET_CPPFLAGS) $(TARGET_ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_i386_setjmp.lst: normal/i386/setjmp.S genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal/i386 -I$(srcdir)/normal/i386 $(TARGET_CPPFLAGS) $(TARGET_ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_color.o: normal/color.c
	$(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -MD -c -o $@ $<
-include normal_mod-normal_color.d

CLEANFILES += cmd-normal_mod-normal_color.lst fs-normal_mod-normal_color.lst
COMMANDFILES += cmd-normal_mod-normal_color.lst
FSFILES += fs-normal_mod-normal_color.lst

cmd-normal_mod-normal_color.lst: normal/color.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-normal_mod-normal_color.lst: normal/color.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Inormal -I$(srcdir)/normal $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod_CFLAGS = $(COMMON_CFLAGS)
normal_mod_ASFLAGS = $(COMMON_ASFLAGS)
normal_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For _chain.mod.
_chain_mod_SOURCES = loader/efi/chainloader.c
CLEANFILES += _chain.mod mod-_chain.o mod-_chain.c pre-_chain.o _chain_mod-loader_efi_chainloader.o und-_chain.lst
ifneq ($(_chain_mod_EXPORTS),no)
CLEANFILES += def-_chain.lst
DEFSYMFILES += def-_chain.lst
endif
MOSTLYCLEANFILES += _chain_mod-loader_efi_chainloader.d
UNDSYMFILES += und-_chain.lst

_chain.mod: pre-_chain.o mod-_chain.o
	-rm -f $@
	$(TARGET_CC) $(_chain_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_chain.o: $(_chain_mod_DEPENDENCIES) _chain_mod-loader_efi_chainloader.o
	-rm -f $@
	$(TARGET_CC) $(_chain_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ _chain_mod-loader_efi_chainloader.o

mod-_chain.o: mod-_chain.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_chain_mod_CFLAGS) -c -o $@ $<

mod-_chain.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh '_chain' $< > $@ || (rm -f $@; exit 1)

ifneq ($(_chain_mod_EXPORTS),no)
def-_chain.lst: pre-_chain.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 _chain/' > $@
endif

und-_chain.lst: pre-_chain.o
	echo '_chain' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

_chain_mod-loader_efi_chainloader.o: loader/efi/chainloader.c
	$(TARGET_CC) -Iloader/efi -I$(srcdir)/loader/efi $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(_chain_mod_CFLAGS) -MD -c -o $@ $<
-include _chain_mod-loader_efi_chainloader.d

CLEANFILES += cmd-_chain_mod-loader_efi_chainloader.lst fs-_chain_mod-loader_efi_chainloader.lst
COMMANDFILES += cmd-_chain_mod-loader_efi_chainloader.lst
FSFILES += fs-_chain_mod-loader_efi_chainloader.lst

cmd-_chain_mod-loader_efi_chainloader.lst: loader/efi/chainloader.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iloader/efi -I$(srcdir)/loader/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh _chain > $@ || (rm -f $@; exit 1)

fs-_chain_mod-loader_efi_chainloader.lst: loader/efi/chainloader.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iloader/efi -I$(srcdir)/loader/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh _chain > $@ || (rm -f $@; exit 1)


_chain_mod_CFLAGS = $(COMMON_CFLAGS)
_chain_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For chain.mod.
chain_mod_SOURCES = loader/efi/chainloader_normal.c
CLEANFILES += chain.mod mod-chain.o mod-chain.c pre-chain.o chain_mod-loader_efi_chainloader_normal.o und-chain.lst
ifneq ($(chain_mod_EXPORTS),no)
CLEANFILES += def-chain.lst
DEFSYMFILES += def-chain.lst
endif
MOSTLYCLEANFILES += chain_mod-loader_efi_chainloader_normal.d
UNDSYMFILES += und-chain.lst

chain.mod: pre-chain.o mod-chain.o
	-rm -f $@
	$(TARGET_CC) $(chain_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-chain.o: $(chain_mod_DEPENDENCIES) chain_mod-loader_efi_chainloader_normal.o
	-rm -f $@
	$(TARGET_CC) $(chain_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ chain_mod-loader_efi_chainloader_normal.o

mod-chain.o: mod-chain.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(chain_mod_CFLAGS) -c -o $@ $<

mod-chain.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'chain' $< > $@ || (rm -f $@; exit 1)

ifneq ($(chain_mod_EXPORTS),no)
def-chain.lst: pre-chain.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 chain/' > $@
endif

und-chain.lst: pre-chain.o
	echo 'chain' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

chain_mod-loader_efi_chainloader_normal.o: loader/efi/chainloader_normal.c
	$(TARGET_CC) -Iloader/efi -I$(srcdir)/loader/efi $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(chain_mod_CFLAGS) -MD -c -o $@ $<
-include chain_mod-loader_efi_chainloader_normal.d

CLEANFILES += cmd-chain_mod-loader_efi_chainloader_normal.lst fs-chain_mod-loader_efi_chainloader_normal.lst
COMMANDFILES += cmd-chain_mod-loader_efi_chainloader_normal.lst
FSFILES += fs-chain_mod-loader_efi_chainloader_normal.lst

cmd-chain_mod-loader_efi_chainloader_normal.lst: loader/efi/chainloader_normal.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iloader/efi -I$(srcdir)/loader/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh chain > $@ || (rm -f $@; exit 1)

fs-chain_mod-loader_efi_chainloader_normal.lst: loader/efi/chainloader_normal.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iloader/efi -I$(srcdir)/loader/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(chain_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh chain > $@ || (rm -f $@; exit 1)


chain_mod_CFLAGS = $(COMMON_CFLAGS)
chain_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For _linux.mod.
_linux_mod_SOURCES = loader/i386/efi/linux.c
CLEANFILES += _linux.mod mod-_linux.o mod-_linux.c pre-_linux.o _linux_mod-loader_i386_efi_linux.o und-_linux.lst
ifneq ($(_linux_mod_EXPORTS),no)
CLEANFILES += def-_linux.lst
DEFSYMFILES += def-_linux.lst
endif
MOSTLYCLEANFILES += _linux_mod-loader_i386_efi_linux.d
UNDSYMFILES += und-_linux.lst

_linux.mod: pre-_linux.o mod-_linux.o
	-rm -f $@
	$(TARGET_CC) $(_linux_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_linux.o: $(_linux_mod_DEPENDENCIES) _linux_mod-loader_i386_efi_linux.o
	-rm -f $@
	$(TARGET_CC) $(_linux_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ _linux_mod-loader_i386_efi_linux.o

mod-_linux.o: mod-_linux.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_linux_mod_CFLAGS) -c -o $@ $<

mod-_linux.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh '_linux' $< > $@ || (rm -f $@; exit 1)

ifneq ($(_linux_mod_EXPORTS),no)
def-_linux.lst: pre-_linux.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 _linux/' > $@
endif

und-_linux.lst: pre-_linux.o
	echo '_linux' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

_linux_mod-loader_i386_efi_linux.o: loader/i386/efi/linux.c
	$(TARGET_CC) -Iloader/i386/efi -I$(srcdir)/loader/i386/efi $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(_linux_mod_CFLAGS) -MD -c -o $@ $<
-include _linux_mod-loader_i386_efi_linux.d

CLEANFILES += cmd-_linux_mod-loader_i386_efi_linux.lst fs-_linux_mod-loader_i386_efi_linux.lst
COMMANDFILES += cmd-_linux_mod-loader_i386_efi_linux.lst
FSFILES += fs-_linux_mod-loader_i386_efi_linux.lst

cmd-_linux_mod-loader_i386_efi_linux.lst: loader/i386/efi/linux.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/efi -I$(srcdir)/loader/i386/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh _linux > $@ || (rm -f $@; exit 1)

fs-_linux_mod-loader_i386_efi_linux.lst: loader/i386/efi/linux.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/efi -I$(srcdir)/loader/i386/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(_linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh _linux > $@ || (rm -f $@; exit 1)


_linux_mod_CFLAGS = $(COMMON_CFLAGS)
_linux_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For linux.mod.
linux_mod_SOURCES = loader/i386/efi/linux_normal.c
CLEANFILES += linux.mod mod-linux.o mod-linux.c pre-linux.o linux_mod-loader_i386_efi_linux_normal.o und-linux.lst
ifneq ($(linux_mod_EXPORTS),no)
CLEANFILES += def-linux.lst
DEFSYMFILES += def-linux.lst
endif
MOSTLYCLEANFILES += linux_mod-loader_i386_efi_linux_normal.d
UNDSYMFILES += und-linux.lst

linux.mod: pre-linux.o mod-linux.o
	-rm -f $@
	$(TARGET_CC) $(linux_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-linux.o: $(linux_mod_DEPENDENCIES) linux_mod-loader_i386_efi_linux_normal.o
	-rm -f $@
	$(TARGET_CC) $(linux_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ linux_mod-loader_i386_efi_linux_normal.o

mod-linux.o: mod-linux.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(linux_mod_CFLAGS) -c -o $@ $<

mod-linux.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'linux' $< > $@ || (rm -f $@; exit 1)

ifneq ($(linux_mod_EXPORTS),no)
def-linux.lst: pre-linux.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 linux/' > $@
endif

und-linux.lst: pre-linux.o
	echo 'linux' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

linux_mod-loader_i386_efi_linux_normal.o: loader/i386/efi/linux_normal.c
	$(TARGET_CC) -Iloader/i386/efi -I$(srcdir)/loader/i386/efi $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(linux_mod_CFLAGS) -MD -c -o $@ $<
-include linux_mod-loader_i386_efi_linux_normal.d

CLEANFILES += cmd-linux_mod-loader_i386_efi_linux_normal.lst fs-linux_mod-loader_i386_efi_linux_normal.lst
COMMANDFILES += cmd-linux_mod-loader_i386_efi_linux_normal.lst
FSFILES += fs-linux_mod-loader_i386_efi_linux_normal.lst

cmd-linux_mod-loader_i386_efi_linux_normal.lst: loader/i386/efi/linux_normal.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/efi -I$(srcdir)/loader/i386/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh linux > $@ || (rm -f $@; exit 1)

fs-linux_mod-loader_i386_efi_linux_normal.lst: loader/i386/efi/linux_normal.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iloader/i386/efi -I$(srcdir)/loader/i386/efi $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh linux > $@ || (rm -f $@; exit 1)


linux_mod_CFLAGS = $(COMMON_CFLAGS)
linux_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For cpuid.mod.
cpuid_mod_SOURCES = commands/i386/cpuid.c
CLEANFILES += cpuid.mod mod-cpuid.o mod-cpuid.c pre-cpuid.o cpuid_mod-commands_i386_cpuid.o und-cpuid.lst
ifneq ($(cpuid_mod_EXPORTS),no)
CLEANFILES += def-cpuid.lst
DEFSYMFILES += def-cpuid.lst
endif
MOSTLYCLEANFILES += cpuid_mod-commands_i386_cpuid.d
UNDSYMFILES += und-cpuid.lst

cpuid.mod: pre-cpuid.o mod-cpuid.o
	-rm -f $@
	$(TARGET_CC) $(cpuid_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-cpuid.o: $(cpuid_mod_DEPENDENCIES) cpuid_mod-commands_i386_cpuid.o
	-rm -f $@
	$(TARGET_CC) $(cpuid_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ cpuid_mod-commands_i386_cpuid.o

mod-cpuid.o: mod-cpuid.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cpuid_mod_CFLAGS) -c -o $@ $<

mod-cpuid.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'cpuid' $< > $@ || (rm -f $@; exit 1)

ifneq ($(cpuid_mod_EXPORTS),no)
def-cpuid.lst: pre-cpuid.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 cpuid/' > $@
endif

und-cpuid.lst: pre-cpuid.o
	echo 'cpuid' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

cpuid_mod-commands_i386_cpuid.o: commands/i386/cpuid.c
	$(TARGET_CC) -Icommands/i386 -I$(srcdir)/commands/i386 $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(cpuid_mod_CFLAGS) -MD -c -o $@ $<
-include cpuid_mod-commands_i386_cpuid.d

CLEANFILES += cmd-cpuid_mod-commands_i386_cpuid.lst fs-cpuid_mod-commands_i386_cpuid.lst
COMMANDFILES += cmd-cpuid_mod-commands_i386_cpuid.lst
FSFILES += fs-cpuid_mod-commands_i386_cpuid.lst

cmd-cpuid_mod-commands_i386_cpuid.lst: commands/i386/cpuid.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386 -I$(srcdir)/commands/i386 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cpuid_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh cpuid > $@ || (rm -f $@; exit 1)

fs-cpuid_mod-commands_i386_cpuid.lst: commands/i386/cpuid.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands/i386 -I$(srcdir)/commands/i386 $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cpuid_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh cpuid > $@ || (rm -f $@; exit 1)


cpuid_mod_CFLAGS = $(COMMON_CFLAGS)
cpuid_mod_LDFLAGS = $(COMMON_LDFLAGS)

include $(srcdir)/conf/common.mk
