
# -*- makefile -*-

COMMON_ASFLAGS = -nostdinc -D__ASSEMBLY__
COMMON_CFLAGS = -ffreestanding -msoft-float

# Images.

MOSTLYCLEANFILES += grubof_symlist.c kernel_syms.lst
DEFSYMFILES += kernel_syms.lst

grubof_HEADERS = arg.h boot.h device.h disk.h dl.h elf.h env.h err.h \
	file.h fs.h kernel.h misc.h mm.h net.h rescue.h symbol.h \
	term.h types.h powerpc/libgcc.h loader.h \
	partition.h pc_partition.h ieee1275/ieee1275.h machine/time.h \
	machine/kernel.h

grubof_symlist.c: $(addprefix include/grub/,$(grubof_HEADERS)) gensymlist.sh
	sh $(srcdir)/gensymlist.sh $(filter %.h,$^) > $@

kernel_syms.lst: $(addprefix include/grub/,$(grubof_HEADERS)) genkernsyms.sh
	sh $(srcdir)/genkernsyms.sh $(filter %h,$^) > $@

# Programs
pkgdata_PROGRAMS = grubof

# Utilities.
bin_UTILITIES = grub-emu grub-mkimage
noinst_UTILITIES = genmoddep

# For grub-mkimage.
grub_mkimage_SOURCES = util/powerpc/ieee1275/grub-mkimage.c util/misc.c \
        util/resolve.c 
CLEANFILES += grub-mkimage grub_mkimage-util_powerpc_ieee1275_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o
MOSTLYCLEANFILES += grub_mkimage-util_powerpc_ieee1275_grub_mkimage.d grub_mkimage-util_misc.d grub_mkimage-util_resolve.d

grub-mkimage: grub_mkimage-util_powerpc_ieee1275_grub_mkimage.o grub_mkimage-util_misc.o grub_mkimage-util_resolve.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(grub_mkimage_LDFLAGS)

grub_mkimage-util_powerpc_ieee1275_grub_mkimage.o: util/powerpc/ieee1275/grub-mkimage.c
	$(BUILD_CC) -Iutil/powerpc/ieee1275 -I$(srcdir)/util/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -c -o $@ $<

grub_mkimage-util_powerpc_ieee1275_grub_mkimage.d: util/powerpc/ieee1275/grub-mkimage.c
	set -e; 	  $(BUILD_CC) -Iutil/powerpc/ieee1275 -I$(srcdir)/util/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -M $< 	  | sed 's,grub\-mkimage\.o[ :]*,grub_mkimage-util_powerpc_ieee1275_grub_mkimage.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_mkimage-util_powerpc_ieee1275_grub_mkimage.d

grub_mkimage-util_misc.o: util/misc.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -c -o $@ $<

grub_mkimage-util_misc.d: util/misc.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_mkimage-util_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_mkimage-util_misc.d

grub_mkimage-util_resolve.o: util/resolve.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -c -o $@ $<

grub_mkimage-util_resolve.d: util/resolve.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_mkimage_CFLAGS) -M $< 	  | sed 's,resolve\.o[ :]*,grub_mkimage-util_resolve.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_mkimage-util_resolve.d


# For grub-emu
grub_emu_SOURCES = commands/boot.c commands/cat.c commands/cmp.c 	\
	commands/configfile.c commands/default.c commands/help.c	\
	commands/search.c commands/terminal.c commands/ls.c		\
	commands/timeout.c						\
	commands/ieee1275/halt.c commands/ieee1275/reboot.c		\
	disk/loopback.c							\
	fs/affs.c fs/ext2.c fs/fat.c fs/fshelp.c fs/hfs.c fs/iso9660.c	\
	fs/jfs.c fs/minix.c fs/sfs.c fs/ufs.c fs/xfs.c			\
	io/gzio.c							\
	kern/device.c kern/disk.c kern/dl.c kern/env.c kern/err.c 	\
	kern/file.c kern/fs.c kern/loader.c kern/main.c kern/misc.c	\
	kern/partition.c kern/rescue.c kern/term.c			\
	normal/arg.c normal/cmdline.c normal/command.c			\
	normal/completion.c normal/context.c	\
	normal/main.c normal/menu.c normal/menu_entry.c normal/misc.c	\
	partmap/amiga.c	partmap/apple.c partmap/pc.c partmap/sun.c	\
	util/console.c util/grub-emu.c util/misc.c			\
	util/i386/pc/biosdisk.c util/i386/pc/getroot.c			\
	util/powerpc/ieee1275/misc.c
CLEANFILES += grub-emu grub_emu-commands_boot.o grub_emu-commands_cat.o grub_emu-commands_cmp.o grub_emu-commands_configfile.o grub_emu-commands_default.o grub_emu-commands_help.o grub_emu-commands_search.o grub_emu-commands_terminal.o grub_emu-commands_ls.o grub_emu-commands_timeout.o grub_emu-commands_ieee1275_halt.o grub_emu-commands_ieee1275_reboot.o grub_emu-disk_loopback.o grub_emu-fs_affs.o grub_emu-fs_ext2.o grub_emu-fs_fat.o grub_emu-fs_fshelp.o grub_emu-fs_hfs.o grub_emu-fs_iso9660.o grub_emu-fs_jfs.o grub_emu-fs_minix.o grub_emu-fs_sfs.o grub_emu-fs_ufs.o grub_emu-fs_xfs.o grub_emu-io_gzio.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_env.o grub_emu-kern_err.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-kern_loader.o grub_emu-kern_main.o grub_emu-kern_misc.o grub_emu-kern_partition.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-normal_arg.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_completion.o grub_emu-normal_context.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_menu_entry.o grub_emu-normal_misc.o grub_emu-partmap_amiga.o grub_emu-partmap_apple.o grub_emu-partmap_pc.o grub_emu-partmap_sun.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_biosdisk.o grub_emu-util_i386_pc_getroot.o grub_emu-util_powerpc_ieee1275_misc.o
MOSTLYCLEANFILES += grub_emu-commands_boot.d grub_emu-commands_cat.d grub_emu-commands_cmp.d grub_emu-commands_configfile.d grub_emu-commands_default.d grub_emu-commands_help.d grub_emu-commands_search.d grub_emu-commands_terminal.d grub_emu-commands_ls.d grub_emu-commands_timeout.d grub_emu-commands_ieee1275_halt.d grub_emu-commands_ieee1275_reboot.d grub_emu-disk_loopback.d grub_emu-fs_affs.d grub_emu-fs_ext2.d grub_emu-fs_fat.d grub_emu-fs_fshelp.d grub_emu-fs_hfs.d grub_emu-fs_iso9660.d grub_emu-fs_jfs.d grub_emu-fs_minix.d grub_emu-fs_sfs.d grub_emu-fs_ufs.d grub_emu-fs_xfs.d grub_emu-io_gzio.d grub_emu-kern_device.d grub_emu-kern_disk.d grub_emu-kern_dl.d grub_emu-kern_env.d grub_emu-kern_err.d grub_emu-kern_file.d grub_emu-kern_fs.d grub_emu-kern_loader.d grub_emu-kern_main.d grub_emu-kern_misc.d grub_emu-kern_partition.d grub_emu-kern_rescue.d grub_emu-kern_term.d grub_emu-normal_arg.d grub_emu-normal_cmdline.d grub_emu-normal_command.d grub_emu-normal_completion.d grub_emu-normal_context.d grub_emu-normal_main.d grub_emu-normal_menu.d grub_emu-normal_menu_entry.d grub_emu-normal_misc.d grub_emu-partmap_amiga.d grub_emu-partmap_apple.d grub_emu-partmap_pc.d grub_emu-partmap_sun.d grub_emu-util_console.d grub_emu-util_grub_emu.d grub_emu-util_misc.d grub_emu-util_i386_pc_biosdisk.d grub_emu-util_i386_pc_getroot.d grub_emu-util_powerpc_ieee1275_misc.d

grub-emu: grub_emu-commands_boot.o grub_emu-commands_cat.o grub_emu-commands_cmp.o grub_emu-commands_configfile.o grub_emu-commands_default.o grub_emu-commands_help.o grub_emu-commands_search.o grub_emu-commands_terminal.o grub_emu-commands_ls.o grub_emu-commands_timeout.o grub_emu-commands_ieee1275_halt.o grub_emu-commands_ieee1275_reboot.o grub_emu-disk_loopback.o grub_emu-fs_affs.o grub_emu-fs_ext2.o grub_emu-fs_fat.o grub_emu-fs_fshelp.o grub_emu-fs_hfs.o grub_emu-fs_iso9660.o grub_emu-fs_jfs.o grub_emu-fs_minix.o grub_emu-fs_sfs.o grub_emu-fs_ufs.o grub_emu-fs_xfs.o grub_emu-io_gzio.o grub_emu-kern_device.o grub_emu-kern_disk.o grub_emu-kern_dl.o grub_emu-kern_env.o grub_emu-kern_err.o grub_emu-kern_file.o grub_emu-kern_fs.o grub_emu-kern_loader.o grub_emu-kern_main.o grub_emu-kern_misc.o grub_emu-kern_partition.o grub_emu-kern_rescue.o grub_emu-kern_term.o grub_emu-normal_arg.o grub_emu-normal_cmdline.o grub_emu-normal_command.o grub_emu-normal_completion.o grub_emu-normal_context.o grub_emu-normal_main.o grub_emu-normal_menu.o grub_emu-normal_menu_entry.o grub_emu-normal_misc.o grub_emu-partmap_amiga.o grub_emu-partmap_apple.o grub_emu-partmap_pc.o grub_emu-partmap_sun.o grub_emu-util_console.o grub_emu-util_grub_emu.o grub_emu-util_misc.o grub_emu-util_i386_pc_biosdisk.o grub_emu-util_i386_pc_getroot.o grub_emu-util_powerpc_ieee1275_misc.o
	$(BUILD_CC) -o $@ $^ $(BUILD_LDFLAGS) $(grub_emu_LDFLAGS)

grub_emu-commands_boot.o: commands/boot.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_boot.d: commands/boot.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,boot\.o[ :]*,grub_emu-commands_boot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_boot.d

grub_emu-commands_cat.o: commands/cat.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_cat.d: commands/cat.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,cat\.o[ :]*,grub_emu-commands_cat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_cat.d

grub_emu-commands_cmp.o: commands/cmp.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_cmp.d: commands/cmp.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,cmp\.o[ :]*,grub_emu-commands_cmp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_cmp.d

grub_emu-commands_configfile.o: commands/configfile.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_configfile.d: commands/configfile.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,configfile\.o[ :]*,grub_emu-commands_configfile.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_configfile.d

grub_emu-commands_default.o: commands/default.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_default.d: commands/default.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,default\.o[ :]*,grub_emu-commands_default.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_default.d

grub_emu-commands_help.o: commands/help.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_help.d: commands/help.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,help\.o[ :]*,grub_emu-commands_help.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_help.d

grub_emu-commands_search.o: commands/search.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_search.d: commands/search.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,search\.o[ :]*,grub_emu-commands_search.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_search.d

grub_emu-commands_terminal.o: commands/terminal.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_terminal.d: commands/terminal.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,terminal\.o[ :]*,grub_emu-commands_terminal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_terminal.d

grub_emu-commands_ls.o: commands/ls.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_ls.d: commands/ls.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,ls\.o[ :]*,grub_emu-commands_ls.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_ls.d

grub_emu-commands_timeout.o: commands/timeout.c
	$(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_timeout.d: commands/timeout.c
	set -e; 	  $(BUILD_CC) -Icommands -I$(srcdir)/commands $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,timeout\.o[ :]*,grub_emu-commands_timeout.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_timeout.d

grub_emu-commands_ieee1275_halt.o: commands/ieee1275/halt.c
	$(BUILD_CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_ieee1275_halt.d: commands/ieee1275/halt.c
	set -e; 	  $(BUILD_CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,halt\.o[ :]*,grub_emu-commands_ieee1275_halt.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_ieee1275_halt.d

grub_emu-commands_ieee1275_reboot.o: commands/ieee1275/reboot.c
	$(BUILD_CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-commands_ieee1275_reboot.d: commands/ieee1275/reboot.c
	set -e; 	  $(BUILD_CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,reboot\.o[ :]*,grub_emu-commands_ieee1275_reboot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-commands_ieee1275_reboot.d

grub_emu-disk_loopback.o: disk/loopback.c
	$(BUILD_CC) -Idisk -I$(srcdir)/disk $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-disk_loopback.d: disk/loopback.c
	set -e; 	  $(BUILD_CC) -Idisk -I$(srcdir)/disk $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,loopback\.o[ :]*,grub_emu-disk_loopback.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-disk_loopback.d

grub_emu-fs_affs.o: fs/affs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_affs.d: fs/affs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,affs\.o[ :]*,grub_emu-fs_affs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_affs.d

grub_emu-fs_ext2.o: fs/ext2.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_ext2.d: fs/ext2.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,ext2\.o[ :]*,grub_emu-fs_ext2.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_ext2.d

grub_emu-fs_fat.o: fs/fat.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_fat.d: fs/fat.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,fat\.o[ :]*,grub_emu-fs_fat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_fat.d

grub_emu-fs_fshelp.o: fs/fshelp.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_fshelp.d: fs/fshelp.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,fshelp\.o[ :]*,grub_emu-fs_fshelp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_fshelp.d

grub_emu-fs_hfs.o: fs/hfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_hfs.d: fs/hfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,hfs\.o[ :]*,grub_emu-fs_hfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_hfs.d

grub_emu-fs_iso9660.o: fs/iso9660.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_iso9660.d: fs/iso9660.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,iso9660\.o[ :]*,grub_emu-fs_iso9660.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_iso9660.d

grub_emu-fs_jfs.o: fs/jfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_jfs.d: fs/jfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,jfs\.o[ :]*,grub_emu-fs_jfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_jfs.d

grub_emu-fs_minix.o: fs/minix.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_minix.d: fs/minix.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,minix\.o[ :]*,grub_emu-fs_minix.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_minix.d

grub_emu-fs_sfs.o: fs/sfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_sfs.d: fs/sfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,sfs\.o[ :]*,grub_emu-fs_sfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_sfs.d

grub_emu-fs_ufs.o: fs/ufs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_ufs.d: fs/ufs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,ufs\.o[ :]*,grub_emu-fs_ufs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_ufs.d

grub_emu-fs_xfs.o: fs/xfs.c
	$(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-fs_xfs.d: fs/xfs.c
	set -e; 	  $(BUILD_CC) -Ifs -I$(srcdir)/fs $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,xfs\.o[ :]*,grub_emu-fs_xfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-fs_xfs.d

grub_emu-io_gzio.o: io/gzio.c
	$(BUILD_CC) -Iio -I$(srcdir)/io $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-io_gzio.d: io/gzio.c
	set -e; 	  $(BUILD_CC) -Iio -I$(srcdir)/io $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,gzio\.o[ :]*,grub_emu-io_gzio.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-io_gzio.d

grub_emu-kern_device.o: kern/device.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_device.d: kern/device.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,device\.o[ :]*,grub_emu-kern_device.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_device.d

grub_emu-kern_disk.o: kern/disk.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_disk.d: kern/disk.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,disk\.o[ :]*,grub_emu-kern_disk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_disk.d

grub_emu-kern_dl.o: kern/dl.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_dl.d: kern/dl.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,grub_emu-kern_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_dl.d

grub_emu-kern_env.o: kern/env.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_env.d: kern/env.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,env\.o[ :]*,grub_emu-kern_env.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_env.d

grub_emu-kern_err.o: kern/err.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_err.d: kern/err.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,err\.o[ :]*,grub_emu-kern_err.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_err.d

grub_emu-kern_file.o: kern/file.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_file.d: kern/file.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,file\.o[ :]*,grub_emu-kern_file.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_file.d

grub_emu-kern_fs.o: kern/fs.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_fs.d: kern/fs.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,fs\.o[ :]*,grub_emu-kern_fs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_fs.d

grub_emu-kern_loader.o: kern/loader.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_loader.d: kern/loader.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,loader\.o[ :]*,grub_emu-kern_loader.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_loader.d

grub_emu-kern_main.o: kern/main.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_main.d: kern/main.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,grub_emu-kern_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_main.d

grub_emu-kern_misc.o: kern/misc.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_misc.d: kern/misc.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_emu-kern_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_misc.d

grub_emu-kern_partition.o: kern/partition.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_partition.d: kern/partition.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,partition\.o[ :]*,grub_emu-kern_partition.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_partition.d

grub_emu-kern_rescue.o: kern/rescue.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_rescue.d: kern/rescue.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,rescue\.o[ :]*,grub_emu-kern_rescue.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_rescue.d

grub_emu-kern_term.o: kern/term.c
	$(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-kern_term.d: kern/term.c
	set -e; 	  $(BUILD_CC) -Ikern -I$(srcdir)/kern $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,term\.o[ :]*,grub_emu-kern_term.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-kern_term.d

grub_emu-normal_arg.o: normal/arg.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_arg.d: normal/arg.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,arg\.o[ :]*,grub_emu-normal_arg.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_arg.d

grub_emu-normal_cmdline.o: normal/cmdline.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_cmdline.d: normal/cmdline.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,cmdline\.o[ :]*,grub_emu-normal_cmdline.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_cmdline.d

grub_emu-normal_command.o: normal/command.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_command.d: normal/command.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,command\.o[ :]*,grub_emu-normal_command.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_command.d

grub_emu-normal_completion.o: normal/completion.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_completion.d: normal/completion.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,completion\.o[ :]*,grub_emu-normal_completion.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_completion.d

grub_emu-normal_context.o: normal/context.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_context.d: normal/context.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,context\.o[ :]*,grub_emu-normal_context.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_context.d

grub_emu-normal_main.o: normal/main.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_main.d: normal/main.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,grub_emu-normal_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_main.d

grub_emu-normal_menu.o: normal/menu.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_menu.d: normal/menu.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,menu\.o[ :]*,grub_emu-normal_menu.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_menu.d

grub_emu-normal_menu_entry.o: normal/menu_entry.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_menu_entry.d: normal/menu_entry.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,menu_entry\.o[ :]*,grub_emu-normal_menu_entry.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_menu_entry.d

grub_emu-normal_misc.o: normal/misc.c
	$(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-normal_misc.d: normal/misc.c
	set -e; 	  $(BUILD_CC) -Inormal -I$(srcdir)/normal $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_emu-normal_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-normal_misc.d

grub_emu-partmap_amiga.o: partmap/amiga.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-partmap_amiga.d: partmap/amiga.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,amiga\.o[ :]*,grub_emu-partmap_amiga.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-partmap_amiga.d

grub_emu-partmap_apple.o: partmap/apple.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-partmap_apple.d: partmap/apple.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,apple\.o[ :]*,grub_emu-partmap_apple.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-partmap_apple.d

grub_emu-partmap_pc.o: partmap/pc.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-partmap_pc.d: partmap/pc.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,pc\.o[ :]*,grub_emu-partmap_pc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-partmap_pc.d

grub_emu-partmap_sun.o: partmap/sun.c
	$(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-partmap_sun.d: partmap/sun.c
	set -e; 	  $(BUILD_CC) -Ipartmap -I$(srcdir)/partmap $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,sun\.o[ :]*,grub_emu-partmap_sun.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-partmap_sun.d

grub_emu-util_console.o: util/console.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_console.d: util/console.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,console\.o[ :]*,grub_emu-util_console.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_console.d

grub_emu-util_grub_emu.o: util/grub-emu.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_grub_emu.d: util/grub-emu.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,grub\-emu\.o[ :]*,grub_emu-util_grub_emu.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_grub_emu.d

grub_emu-util_misc.o: util/misc.c
	$(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_misc.d: util/misc.c
	set -e; 	  $(BUILD_CC) -Iutil -I$(srcdir)/util $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_emu-util_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_misc.d

grub_emu-util_i386_pc_biosdisk.o: util/i386/pc/biosdisk.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_i386_pc_biosdisk.d: util/i386/pc/biosdisk.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,biosdisk\.o[ :]*,grub_emu-util_i386_pc_biosdisk.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_i386_pc_biosdisk.d

grub_emu-util_i386_pc_getroot.o: util/i386/pc/getroot.c
	$(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_i386_pc_getroot.d: util/i386/pc/getroot.c
	set -e; 	  $(BUILD_CC) -Iutil/i386/pc -I$(srcdir)/util/i386/pc $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,getroot\.o[ :]*,grub_emu-util_i386_pc_getroot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_i386_pc_getroot.d

grub_emu-util_powerpc_ieee1275_misc.o: util/powerpc/ieee1275/misc.c
	$(BUILD_CC) -Iutil/powerpc/ieee1275 -I$(srcdir)/util/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -c -o $@ $<

grub_emu-util_powerpc_ieee1275_misc.d: util/powerpc/ieee1275/misc.c
	set -e; 	  $(BUILD_CC) -Iutil/powerpc/ieee1275 -I$(srcdir)/util/powerpc/ieee1275 $(BUILD_CPPFLAGS) $(BUILD_CFLAGS) -DGRUB_UTIL=1 $(grub_emu_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,grub_emu-util_powerpc_ieee1275_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grub_emu-util_powerpc_ieee1275_misc.d


grub_emu_LDFLAGS = $(LIBCURSES)

grubof_SOURCES = kern/powerpc/ieee1275/crt0.S kern/powerpc/ieee1275/cmain.c \
	kern/ieee1275/ieee1275.c kern/main.c kern/device.c \
	kern/disk.c kern/dl.c kern/file.c kern/fs.c kern/err.c \
	kern/misc.c kern/mm.c kern/loader.c kern/rescue.c kern/term.c \
	kern/powerpc/ieee1275/init.c term/ieee1275/ofconsole.c \
	kern/powerpc/ieee1275/openfw.c disk/ieee1275/ofdisk.c \
	kern/partition.c kern/env.c kern/powerpc/dl.c grubof_symlist.c \
	kern/powerpc/cache.S
CLEANFILES += grubof grubof-kern_powerpc_ieee1275_crt0.o grubof-kern_powerpc_ieee1275_cmain.o grubof-kern_ieee1275_ieee1275.o grubof-kern_main.o grubof-kern_device.o grubof-kern_disk.o grubof-kern_dl.o grubof-kern_file.o grubof-kern_fs.o grubof-kern_err.o grubof-kern_misc.o grubof-kern_mm.o grubof-kern_loader.o grubof-kern_rescue.o grubof-kern_term.o grubof-kern_powerpc_ieee1275_init.o grubof-term_ieee1275_ofconsole.o grubof-kern_powerpc_ieee1275_openfw.o grubof-disk_ieee1275_ofdisk.o grubof-kern_partition.o grubof-kern_env.o grubof-kern_powerpc_dl.o grubof-grubof_symlist.o grubof-kern_powerpc_cache.o
MOSTLYCLEANFILES += grubof-kern_powerpc_ieee1275_crt0.d grubof-kern_powerpc_ieee1275_cmain.d grubof-kern_ieee1275_ieee1275.d grubof-kern_main.d grubof-kern_device.d grubof-kern_disk.d grubof-kern_dl.d grubof-kern_file.d grubof-kern_fs.d grubof-kern_err.d grubof-kern_misc.d grubof-kern_mm.d grubof-kern_loader.d grubof-kern_rescue.d grubof-kern_term.d grubof-kern_powerpc_ieee1275_init.d grubof-term_ieee1275_ofconsole.d grubof-kern_powerpc_ieee1275_openfw.d grubof-disk_ieee1275_ofdisk.d grubof-kern_partition.d grubof-kern_env.d grubof-kern_powerpc_dl.d grubof-grubof_symlist.d grubof-kern_powerpc_cache.d

grubof: grubof-kern_powerpc_ieee1275_crt0.o grubof-kern_powerpc_ieee1275_cmain.o grubof-kern_ieee1275_ieee1275.o grubof-kern_main.o grubof-kern_device.o grubof-kern_disk.o grubof-kern_dl.o grubof-kern_file.o grubof-kern_fs.o grubof-kern_err.o grubof-kern_misc.o grubof-kern_mm.o grubof-kern_loader.o grubof-kern_rescue.o grubof-kern_term.o grubof-kern_powerpc_ieee1275_init.o grubof-term_ieee1275_ofconsole.o grubof-kern_powerpc_ieee1275_openfw.o grubof-disk_ieee1275_ofdisk.o grubof-kern_partition.o grubof-kern_env.o grubof-kern_powerpc_dl.o grubof-grubof_symlist.o grubof-kern_powerpc_cache.o
	$(CC) -o $@ $^ $(LDFLAGS) $(grubof_LDFLAGS)

grubof-kern_powerpc_ieee1275_crt0.o: kern/powerpc/ieee1275/crt0.S
	$(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_ieee1275_crt0.d: kern/powerpc/ieee1275/crt0.S
	set -e; 	  $(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,crt0\.o[ :]*,grubof-kern_powerpc_ieee1275_crt0.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_ieee1275_crt0.d

grubof-kern_powerpc_ieee1275_cmain.o: kern/powerpc/ieee1275/cmain.c
	$(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_ieee1275_cmain.d: kern/powerpc/ieee1275/cmain.c
	set -e; 	  $(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,cmain\.o[ :]*,grubof-kern_powerpc_ieee1275_cmain.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_ieee1275_cmain.d

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

grubof-kern_powerpc_ieee1275_init.o: kern/powerpc/ieee1275/init.c
	$(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_ieee1275_init.d: kern/powerpc/ieee1275/init.c
	set -e; 	  $(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,init\.o[ :]*,grubof-kern_powerpc_ieee1275_init.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_ieee1275_init.d

grubof-term_ieee1275_ofconsole.o: term/ieee1275/ofconsole.c
	$(CC) -Iterm/ieee1275 -I$(srcdir)/term/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-term_ieee1275_ofconsole.d: term/ieee1275/ofconsole.c
	set -e; 	  $(CC) -Iterm/ieee1275 -I$(srcdir)/term/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,ofconsole\.o[ :]*,grubof-term_ieee1275_ofconsole.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-term_ieee1275_ofconsole.d

grubof-kern_powerpc_ieee1275_openfw.o: kern/powerpc/ieee1275/openfw.c
	$(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_ieee1275_openfw.d: kern/powerpc/ieee1275/openfw.c
	set -e; 	  $(CC) -Ikern/powerpc/ieee1275 -I$(srcdir)/kern/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,openfw\.o[ :]*,grubof-kern_powerpc_ieee1275_openfw.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_ieee1275_openfw.d

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

grubof-kern_powerpc_dl.o: kern/powerpc/dl.c
	$(CC) -Ikern/powerpc -I$(srcdir)/kern/powerpc $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_dl.d: kern/powerpc/dl.c
	set -e; 	  $(CC) -Ikern/powerpc -I$(srcdir)/kern/powerpc $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,dl\.o[ :]*,grubof-kern_powerpc_dl.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_dl.d

grubof-grubof_symlist.o: grubof_symlist.c
	$(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-grubof_symlist.d: grubof_symlist.c
	set -e; 	  $(CC) -I. -I$(srcdir)/. $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,grubof_symlist\.o[ :]*,grubof-grubof_symlist.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-grubof_symlist.d

grubof-kern_powerpc_cache.o: kern/powerpc/cache.S
	$(CC) -Ikern/powerpc -I$(srcdir)/kern/powerpc $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -c -o $@ $<

grubof-kern_powerpc_cache.d: kern/powerpc/cache.S
	set -e; 	  $(CC) -Ikern/powerpc -I$(srcdir)/kern/powerpc $(CPPFLAGS) $(CFLAGS) $(grubof_CFLAGS) -M $< 	  | sed 's,cache\.o[ :]*,grubof-kern_powerpc_cache.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include grubof-kern_powerpc_cache.d

grubof_HEADERS = grub/powerpc/ieee1275/ieee1275.h
grubof_CFLAGS = $(COMMON_CFLAGS)
grubof_ASFLAGS = $(COMMON_ASFLAGS)
grubof_LDFLAGS = -nostdlib -static-libgcc -lgcc -Wl,-N,-S,-Ttext,0x200000,-Bstatic

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
pkgdata_MODULES = _linux.mod linux.mod fat.mod ufs.mod ext2.mod minix.mod \
	hfs.mod jfs.mod normal.mod hello.mod font.mod ls.mod \
	boot.mod cmp.mod cat.mod terminal.mod fshelp.mod amiga.mod apple.mod \
	pc.mod suspend.mod loopback.mod help.mod reboot.mod halt.mod sun.mod \
	default.mod timeout.mod configfile.mod search.mod gzio.mod xfs.mod \
	affs.mod sfs.mod

# For fshelp.mod.
fshelp_mod_SOURCES = fs/fshelp.c
CLEANFILES += fshelp.mod mod-fshelp.o mod-fshelp.c pre-fshelp.o fshelp_mod-fs_fshelp.o def-fshelp.lst und-fshelp.lst
MOSTLYCLEANFILES += fshelp_mod-fs_fshelp.d
DEFSYMFILES += def-fshelp.lst
UNDSYMFILES += und-fshelp.lst

fshelp.mod: pre-fshelp.o mod-fshelp.o
	-rm -f $@
	$(LD) $(fshelp_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-fshelp.o: fshelp_mod-fs_fshelp.o
	-rm -f $@
	$(LD) $(fshelp_mod_LDFLAGS) -r -d -o $@ $^

mod-fshelp.o: mod-fshelp.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(fshelp_mod_CFLAGS) -c -o $@ $<

mod-fshelp.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'fshelp' $< > $@ || (rm -f $@; exit 1)

def-fshelp.lst: pre-fshelp.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 fshelp/' > $@

und-fshelp.lst: pre-fshelp.o
	echo 'fshelp' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

fshelp_mod-fs_fshelp.o: fs/fshelp.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fshelp_mod_CFLAGS) -c -o $@ $<

fshelp_mod-fs_fshelp.d: fs/fshelp.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fshelp_mod_CFLAGS) -M $< 	  | sed 's,fshelp\.o[ :]*,fshelp_mod-fs_fshelp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include fshelp_mod-fs_fshelp.d

CLEANFILES += cmd-fshelp.lst fs-fshelp.lst
COMMANDFILES += cmd-fshelp.lst
FSFILES += fs-fshelp.lst

cmd-fshelp.lst: fs/fshelp.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fshelp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh fshelp > $@ || (rm -f $@; exit 1)

fs-fshelp.lst: fs/fshelp.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fshelp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh fshelp > $@ || (rm -f $@; exit 1)


fshelp_mod_CFLAGS = $(COMMON_CFLAGS)

# For fat.mod.
fat_mod_SOURCES = fs/fat.c
CLEANFILES += fat.mod mod-fat.o mod-fat.c pre-fat.o fat_mod-fs_fat.o def-fat.lst und-fat.lst
MOSTLYCLEANFILES += fat_mod-fs_fat.d
DEFSYMFILES += def-fat.lst
UNDSYMFILES += und-fat.lst

fat.mod: pre-fat.o mod-fat.o
	-rm -f $@
	$(LD) $(fat_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-fat.o: fat_mod-fs_fat.o
	-rm -f $@
	$(LD) $(fat_mod_LDFLAGS) -r -d -o $@ $^

mod-fat.o: mod-fat.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -c -o $@ $<

mod-fat.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'fat' $< > $@ || (rm -f $@; exit 1)

def-fat.lst: pre-fat.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 fat/' > $@

und-fat.lst: pre-fat.o
	echo 'fat' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

fat_mod-fs_fat.o: fs/fat.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -c -o $@ $<

fat_mod-fs_fat.d: fs/fat.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -M $< 	  | sed 's,fat\.o[ :]*,fat_mod-fs_fat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include fat_mod-fs_fat.d

CLEANFILES += cmd-fat.lst fs-fat.lst
COMMANDFILES += cmd-fat.lst
FSFILES += fs-fat.lst

cmd-fat.lst: fs/fat.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh fat > $@ || (rm -f $@; exit 1)

fs-fat.lst: fs/fat.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(fat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh fat > $@ || (rm -f $@; exit 1)


fat_mod_CFLAGS = $(COMMON_CFLAGS)

# For ext2.mod.
ext2_mod_SOURCES = fs/ext2.c
CLEANFILES += ext2.mod mod-ext2.o mod-ext2.c pre-ext2.o ext2_mod-fs_ext2.o def-ext2.lst und-ext2.lst
MOSTLYCLEANFILES += ext2_mod-fs_ext2.d
DEFSYMFILES += def-ext2.lst
UNDSYMFILES += und-ext2.lst

ext2.mod: pre-ext2.o mod-ext2.o
	-rm -f $@
	$(LD) $(ext2_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ext2.o: ext2_mod-fs_ext2.o
	-rm -f $@
	$(LD) $(ext2_mod_LDFLAGS) -r -d -o $@ $^

mod-ext2.o: mod-ext2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ext2_mod_CFLAGS) -c -o $@ $<

mod-ext2.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'ext2' $< > $@ || (rm -f $@; exit 1)

def-ext2.lst: pre-ext2.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 ext2/' > $@

und-ext2.lst: pre-ext2.o
	echo 'ext2' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

ext2_mod-fs_ext2.o: fs/ext2.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ext2_mod_CFLAGS) -c -o $@ $<

ext2_mod-fs_ext2.d: fs/ext2.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ext2_mod_CFLAGS) -M $< 	  | sed 's,ext2\.o[ :]*,ext2_mod-fs_ext2.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include ext2_mod-fs_ext2.d

CLEANFILES += cmd-ext2.lst fs-ext2.lst
COMMANDFILES += cmd-ext2.lst
FSFILES += fs-ext2.lst

cmd-ext2.lst: fs/ext2.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ext2_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ext2 > $@ || (rm -f $@; exit 1)

fs-ext2.lst: fs/ext2.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ext2_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ext2 > $@ || (rm -f $@; exit 1)


ext2_mod_CFLAGS = $(COMMON_CFLAGS)

# For ufs.mod.
ufs_mod_SOURCES = fs/ufs.c
CLEANFILES += ufs.mod mod-ufs.o mod-ufs.c pre-ufs.o ufs_mod-fs_ufs.o def-ufs.lst und-ufs.lst
MOSTLYCLEANFILES += ufs_mod-fs_ufs.d
DEFSYMFILES += def-ufs.lst
UNDSYMFILES += und-ufs.lst

ufs.mod: pre-ufs.o mod-ufs.o
	-rm -f $@
	$(LD) $(ufs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ufs.o: ufs_mod-fs_ufs.o
	-rm -f $@
	$(LD) $(ufs_mod_LDFLAGS) -r -d -o $@ $^

mod-ufs.o: mod-ufs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ufs_mod_CFLAGS) -c -o $@ $<

mod-ufs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'ufs' $< > $@ || (rm -f $@; exit 1)

def-ufs.lst: pre-ufs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 ufs/' > $@

und-ufs.lst: pre-ufs.o
	echo 'ufs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

ufs_mod-fs_ufs.o: fs/ufs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ufs_mod_CFLAGS) -c -o $@ $<

ufs_mod-fs_ufs.d: fs/ufs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ufs_mod_CFLAGS) -M $< 	  | sed 's,ufs\.o[ :]*,ufs_mod-fs_ufs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include ufs_mod-fs_ufs.d

CLEANFILES += cmd-ufs.lst fs-ufs.lst
COMMANDFILES += cmd-ufs.lst
FSFILES += fs-ufs.lst

cmd-ufs.lst: fs/ufs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ufs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ufs > $@ || (rm -f $@; exit 1)

fs-ufs.lst: fs/ufs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(ufs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ufs > $@ || (rm -f $@; exit 1)


ufs_mod_CFLAGS = $(COMMON_CFLAGS)

# For minix.mod.
minix_mod_SOURCES = fs/minix.c
CLEANFILES += minix.mod mod-minix.o mod-minix.c pre-minix.o minix_mod-fs_minix.o def-minix.lst und-minix.lst
MOSTLYCLEANFILES += minix_mod-fs_minix.d
DEFSYMFILES += def-minix.lst
UNDSYMFILES += und-minix.lst

minix.mod: pre-minix.o mod-minix.o
	-rm -f $@
	$(LD) $(minix_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-minix.o: minix_mod-fs_minix.o
	-rm -f $@
	$(LD) $(minix_mod_LDFLAGS) -r -d -o $@ $^

mod-minix.o: mod-minix.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(minix_mod_CFLAGS) -c -o $@ $<

mod-minix.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'minix' $< > $@ || (rm -f $@; exit 1)

def-minix.lst: pre-minix.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 minix/' > $@

und-minix.lst: pre-minix.o
	echo 'minix' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

minix_mod-fs_minix.o: fs/minix.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(minix_mod_CFLAGS) -c -o $@ $<

minix_mod-fs_minix.d: fs/minix.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(minix_mod_CFLAGS) -M $< 	  | sed 's,minix\.o[ :]*,minix_mod-fs_minix.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include minix_mod-fs_minix.d

CLEANFILES += cmd-minix.lst fs-minix.lst
COMMANDFILES += cmd-minix.lst
FSFILES += fs-minix.lst

cmd-minix.lst: fs/minix.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(minix_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh minix > $@ || (rm -f $@; exit 1)

fs-minix.lst: fs/minix.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(minix_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh minix > $@ || (rm -f $@; exit 1)


minix_mod_CFLAGS = $(COMMON_CFLAGS)

# For hfs.mod.
hfs_mod_SOURCES = fs/hfs.c
CLEANFILES += hfs.mod mod-hfs.o mod-hfs.c pre-hfs.o hfs_mod-fs_hfs.o def-hfs.lst und-hfs.lst
MOSTLYCLEANFILES += hfs_mod-fs_hfs.d
DEFSYMFILES += def-hfs.lst
UNDSYMFILES += und-hfs.lst

hfs.mod: pre-hfs.o mod-hfs.o
	-rm -f $@
	$(LD) $(hfs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-hfs.o: hfs_mod-fs_hfs.o
	-rm -f $@
	$(LD) $(hfs_mod_LDFLAGS) -r -d -o $@ $^

mod-hfs.o: mod-hfs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(hfs_mod_CFLAGS) -c -o $@ $<

mod-hfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'hfs' $< > $@ || (rm -f $@; exit 1)

def-hfs.lst: pre-hfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 hfs/' > $@

und-hfs.lst: pre-hfs.o
	echo 'hfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

hfs_mod-fs_hfs.o: fs/hfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(hfs_mod_CFLAGS) -c -o $@ $<

hfs_mod-fs_hfs.d: fs/hfs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(hfs_mod_CFLAGS) -M $< 	  | sed 's,hfs\.o[ :]*,hfs_mod-fs_hfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include hfs_mod-fs_hfs.d

CLEANFILES += cmd-hfs.lst fs-hfs.lst
COMMANDFILES += cmd-hfs.lst
FSFILES += fs-hfs.lst

cmd-hfs.lst: fs/hfs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(hfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh hfs > $@ || (rm -f $@; exit 1)

fs-hfs.lst: fs/hfs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(hfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh hfs > $@ || (rm -f $@; exit 1)


hfs_mod_CFLAGS = $(COMMON_CFLAGS)

# For jfs.mod.
jfs_mod_SOURCES = fs/jfs.c
CLEANFILES += jfs.mod mod-jfs.o mod-jfs.c pre-jfs.o jfs_mod-fs_jfs.o def-jfs.lst und-jfs.lst
MOSTLYCLEANFILES += jfs_mod-fs_jfs.d
DEFSYMFILES += def-jfs.lst
UNDSYMFILES += und-jfs.lst

jfs.mod: pre-jfs.o mod-jfs.o
	-rm -f $@
	$(LD) $(jfs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-jfs.o: jfs_mod-fs_jfs.o
	-rm -f $@
	$(LD) $(jfs_mod_LDFLAGS) -r -d -o $@ $^

mod-jfs.o: mod-jfs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(jfs_mod_CFLAGS) -c -o $@ $<

mod-jfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'jfs' $< > $@ || (rm -f $@; exit 1)

def-jfs.lst: pre-jfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 jfs/' > $@

und-jfs.lst: pre-jfs.o
	echo 'jfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

jfs_mod-fs_jfs.o: fs/jfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(jfs_mod_CFLAGS) -c -o $@ $<

jfs_mod-fs_jfs.d: fs/jfs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(jfs_mod_CFLAGS) -M $< 	  | sed 's,jfs\.o[ :]*,jfs_mod-fs_jfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include jfs_mod-fs_jfs.d

CLEANFILES += cmd-jfs.lst fs-jfs.lst
COMMANDFILES += cmd-jfs.lst
FSFILES += fs-jfs.lst

cmd-jfs.lst: fs/jfs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(jfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh jfs > $@ || (rm -f $@; exit 1)

fs-jfs.lst: fs/jfs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(jfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh jfs > $@ || (rm -f $@; exit 1)


jfs_mod_CFLAGS = $(COMMON_CFLAGS)

# For iso9660.mod.
iso9660_mod_SOURCES = fs/iso9660.c
iso9660_mod_CFLAGS = $(COMMON_CFLAGS)

# For xfs.mod.
xfs_mod_SOURCES = fs/xfs.c
CLEANFILES += xfs.mod mod-xfs.o mod-xfs.c pre-xfs.o xfs_mod-fs_xfs.o def-xfs.lst und-xfs.lst
MOSTLYCLEANFILES += xfs_mod-fs_xfs.d
DEFSYMFILES += def-xfs.lst
UNDSYMFILES += und-xfs.lst

xfs.mod: pre-xfs.o mod-xfs.o
	-rm -f $@
	$(LD) $(xfs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-xfs.o: xfs_mod-fs_xfs.o
	-rm -f $@
	$(LD) $(xfs_mod_LDFLAGS) -r -d -o $@ $^

mod-xfs.o: mod-xfs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(xfs_mod_CFLAGS) -c -o $@ $<

mod-xfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'xfs' $< > $@ || (rm -f $@; exit 1)

def-xfs.lst: pre-xfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 xfs/' > $@

und-xfs.lst: pre-xfs.o
	echo 'xfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

xfs_mod-fs_xfs.o: fs/xfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(xfs_mod_CFLAGS) -c -o $@ $<

xfs_mod-fs_xfs.d: fs/xfs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(xfs_mod_CFLAGS) -M $< 	  | sed 's,xfs\.o[ :]*,xfs_mod-fs_xfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include xfs_mod-fs_xfs.d

CLEANFILES += cmd-xfs.lst fs-xfs.lst
COMMANDFILES += cmd-xfs.lst
FSFILES += fs-xfs.lst

cmd-xfs.lst: fs/xfs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(xfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh xfs > $@ || (rm -f $@; exit 1)

fs-xfs.lst: fs/xfs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(xfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh xfs > $@ || (rm -f $@; exit 1)


xfs_mod_CFLAGS = $(COMMON_CFLAGS)

# For affs.mod.
affs_mod_SOURCES = fs/affs.c
CLEANFILES += affs.mod mod-affs.o mod-affs.c pre-affs.o affs_mod-fs_affs.o def-affs.lst und-affs.lst
MOSTLYCLEANFILES += affs_mod-fs_affs.d
DEFSYMFILES += def-affs.lst
UNDSYMFILES += und-affs.lst

affs.mod: pre-affs.o mod-affs.o
	-rm -f $@
	$(LD) $(affs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-affs.o: affs_mod-fs_affs.o
	-rm -f $@
	$(LD) $(affs_mod_LDFLAGS) -r -d -o $@ $^

mod-affs.o: mod-affs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(affs_mod_CFLAGS) -c -o $@ $<

mod-affs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'affs' $< > $@ || (rm -f $@; exit 1)

def-affs.lst: pre-affs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 affs/' > $@

und-affs.lst: pre-affs.o
	echo 'affs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

affs_mod-fs_affs.o: fs/affs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(affs_mod_CFLAGS) -c -o $@ $<

affs_mod-fs_affs.d: fs/affs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(affs_mod_CFLAGS) -M $< 	  | sed 's,affs\.o[ :]*,affs_mod-fs_affs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include affs_mod-fs_affs.d

CLEANFILES += cmd-affs.lst fs-affs.lst
COMMANDFILES += cmd-affs.lst
FSFILES += fs-affs.lst

cmd-affs.lst: fs/affs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(affs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh affs > $@ || (rm -f $@; exit 1)

fs-affs.lst: fs/affs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(affs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh affs > $@ || (rm -f $@; exit 1)


affs_mod_CFLAGS = $(COMMON_CFLAGS)

# For sfs.mod.
sfs_mod_SOURCES = fs/sfs.c
CLEANFILES += sfs.mod mod-sfs.o mod-sfs.c pre-sfs.o sfs_mod-fs_sfs.o def-sfs.lst und-sfs.lst
MOSTLYCLEANFILES += sfs_mod-fs_sfs.d
DEFSYMFILES += def-sfs.lst
UNDSYMFILES += und-sfs.lst

sfs.mod: pre-sfs.o mod-sfs.o
	-rm -f $@
	$(LD) $(sfs_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-sfs.o: sfs_mod-fs_sfs.o
	-rm -f $@
	$(LD) $(sfs_mod_LDFLAGS) -r -d -o $@ $^

mod-sfs.o: mod-sfs.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(sfs_mod_CFLAGS) -c -o $@ $<

mod-sfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'sfs' $< > $@ || (rm -f $@; exit 1)

def-sfs.lst: pre-sfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 sfs/' > $@

und-sfs.lst: pre-sfs.o
	echo 'sfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

sfs_mod-fs_sfs.o: fs/sfs.c
	$(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(sfs_mod_CFLAGS) -c -o $@ $<

sfs_mod-fs_sfs.d: fs/sfs.c
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(sfs_mod_CFLAGS) -M $< 	  | sed 's,sfs\.o[ :]*,sfs_mod-fs_sfs.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include sfs_mod-fs_sfs.d

CLEANFILES += cmd-sfs.lst fs-sfs.lst
COMMANDFILES += cmd-sfs.lst
FSFILES += fs-sfs.lst

cmd-sfs.lst: fs/sfs.c gencmdlist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(sfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh sfs > $@ || (rm -f $@; exit 1)

fs-sfs.lst: fs/sfs.c genfslist.sh
	set -e; 	  $(CC) -Ifs -I$(srcdir)/fs $(CPPFLAGS) $(CFLAGS) $(sfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh sfs > $@ || (rm -f $@; exit 1)


sfs_mod_CFLAGS = $(COMMON_CFLAGS)

# For _linux.mod.
_linux_mod_SOURCES = loader/powerpc/ieee1275/linux.c
CLEANFILES += _linux.mod mod-_linux.o mod-_linux.c pre-_linux.o _linux_mod-loader_powerpc_ieee1275_linux.o def-_linux.lst und-_linux.lst
MOSTLYCLEANFILES += _linux_mod-loader_powerpc_ieee1275_linux.d
DEFSYMFILES += def-_linux.lst
UNDSYMFILES += und-_linux.lst

_linux.mod: pre-_linux.o mod-_linux.o
	-rm -f $@
	$(LD) $(_linux_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-_linux.o: _linux_mod-loader_powerpc_ieee1275_linux.o
	-rm -f $@
	$(LD) $(_linux_mod_LDFLAGS) -r -d -o $@ $^

mod-_linux.o: mod-_linux.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -c -o $@ $<

mod-_linux.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh '_linux' $< > $@ || (rm -f $@; exit 1)

def-_linux.lst: pre-_linux.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 _linux/' > $@

und-_linux.lst: pre-_linux.o
	echo '_linux' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

_linux_mod-loader_powerpc_ieee1275_linux.o: loader/powerpc/ieee1275/linux.c
	$(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -c -o $@ $<

_linux_mod-loader_powerpc_ieee1275_linux.d: loader/powerpc/ieee1275/linux.c
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -M $< 	  | sed 's,linux\.o[ :]*,_linux_mod-loader_powerpc_ieee1275_linux.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include _linux_mod-loader_powerpc_ieee1275_linux.d

CLEANFILES += cmd-linux.lst fs-linux.lst
COMMANDFILES += cmd-linux.lst
FSFILES += fs-linux.lst

cmd-linux.lst: loader/powerpc/ieee1275/linux.c gencmdlist.sh
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh _linux > $@ || (rm -f $@; exit 1)

fs-linux.lst: loader/powerpc/ieee1275/linux.c genfslist.sh
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(_linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh _linux > $@ || (rm -f $@; exit 1)


_linux_mod_CFLAGS = $(COMMON_CFLAGS)

# For linux.mod.
linux_mod_SOURCES = loader/powerpc/ieee1275/linux_normal.c
CLEANFILES += linux.mod mod-linux.o mod-linux.c pre-linux.o linux_mod-loader_powerpc_ieee1275_linux_normal.o def-linux.lst und-linux.lst
MOSTLYCLEANFILES += linux_mod-loader_powerpc_ieee1275_linux_normal.d
DEFSYMFILES += def-linux.lst
UNDSYMFILES += und-linux.lst

linux.mod: pre-linux.o mod-linux.o
	-rm -f $@
	$(LD) $(linux_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-linux.o: linux_mod-loader_powerpc_ieee1275_linux_normal.o
	-rm -f $@
	$(LD) $(linux_mod_LDFLAGS) -r -d -o $@ $^

mod-linux.o: mod-linux.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -c -o $@ $<

mod-linux.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'linux' $< > $@ || (rm -f $@; exit 1)

def-linux.lst: pre-linux.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 linux/' > $@

und-linux.lst: pre-linux.o
	echo 'linux' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

linux_mod-loader_powerpc_ieee1275_linux_normal.o: loader/powerpc/ieee1275/linux_normal.c
	$(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -c -o $@ $<

linux_mod-loader_powerpc_ieee1275_linux_normal.d: loader/powerpc/ieee1275/linux_normal.c
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -M $< 	  | sed 's,linux_normal\.o[ :]*,linux_mod-loader_powerpc_ieee1275_linux_normal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include linux_mod-loader_powerpc_ieee1275_linux_normal.d

CLEANFILES += cmd-linux_normal.lst fs-linux_normal.lst
COMMANDFILES += cmd-linux_normal.lst
FSFILES += fs-linux_normal.lst

cmd-linux_normal.lst: loader/powerpc/ieee1275/linux_normal.c gencmdlist.sh
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh linux > $@ || (rm -f $@; exit 1)

fs-linux_normal.lst: loader/powerpc/ieee1275/linux_normal.c genfslist.sh
	set -e; 	  $(CC) -Iloader/powerpc/ieee1275 -I$(srcdir)/loader/powerpc/ieee1275 $(CPPFLAGS) $(CFLAGS) $(linux_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh linux > $@ || (rm -f $@; exit 1)


linux_mod_CFLAGS = $(COMMON_CFLAGS)

# For normal.mod.
normal_mod_SOURCES = normal/arg.c normal/cmdline.c normal/command.c	\
	normal/completion.c normal/context.c normal/main.c		\
	normal/menu.c normal/menu_entry.c normal/misc.c			\
	normal/powerpc/setjmp.S
CLEANFILES += normal.mod mod-normal.o mod-normal.c pre-normal.o normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_context.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-normal_powerpc_setjmp.o def-normal.lst und-normal.lst
MOSTLYCLEANFILES += normal_mod-normal_arg.d normal_mod-normal_cmdline.d normal_mod-normal_command.d normal_mod-normal_completion.d normal_mod-normal_context.d normal_mod-normal_main.d normal_mod-normal_menu.d normal_mod-normal_menu_entry.d normal_mod-normal_misc.d normal_mod-normal_powerpc_setjmp.d
DEFSYMFILES += def-normal.lst
UNDSYMFILES += und-normal.lst

normal.mod: pre-normal.o mod-normal.o
	-rm -f $@
	$(LD) $(normal_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-normal.o: normal_mod-normal_arg.o normal_mod-normal_cmdline.o normal_mod-normal_command.o normal_mod-normal_completion.o normal_mod-normal_context.o normal_mod-normal_main.o normal_mod-normal_menu.o normal_mod-normal_menu_entry.o normal_mod-normal_misc.o normal_mod-normal_powerpc_setjmp.o
	-rm -f $@
	$(LD) $(normal_mod_LDFLAGS) -r -d -o $@ $^

mod-normal.o: mod-normal.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

mod-normal.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'normal' $< > $@ || (rm -f $@; exit 1)

def-normal.lst: pre-normal.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 normal/' > $@

und-normal.lst: pre-normal.o
	echo 'normal' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

normal_mod-normal_arg.o: normal/arg.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_arg.d: normal/arg.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,arg\.o[ :]*,normal_mod-normal_arg.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_arg.d

CLEANFILES += cmd-arg.lst fs-arg.lst
COMMANDFILES += cmd-arg.lst
FSFILES += fs-arg.lst

cmd-arg.lst: normal/arg.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-arg.lst: normal/arg.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_cmdline.o: normal/cmdline.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_cmdline.d: normal/cmdline.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,cmdline\.o[ :]*,normal_mod-normal_cmdline.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_cmdline.d

CLEANFILES += cmd-cmdline.lst fs-cmdline.lst
COMMANDFILES += cmd-cmdline.lst
FSFILES += fs-cmdline.lst

cmd-cmdline.lst: normal/cmdline.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-cmdline.lst: normal/cmdline.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_command.o: normal/command.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_command.d: normal/command.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,command\.o[ :]*,normal_mod-normal_command.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_command.d

CLEANFILES += cmd-command.lst fs-command.lst
COMMANDFILES += cmd-command.lst
FSFILES += fs-command.lst

cmd-command.lst: normal/command.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-command.lst: normal/command.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_completion.o: normal/completion.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_completion.d: normal/completion.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,completion\.o[ :]*,normal_mod-normal_completion.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_completion.d

CLEANFILES += cmd-completion.lst fs-completion.lst
COMMANDFILES += cmd-completion.lst
FSFILES += fs-completion.lst

cmd-completion.lst: normal/completion.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-completion.lst: normal/completion.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_context.o: normal/context.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_context.d: normal/context.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,context\.o[ :]*,normal_mod-normal_context.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_context.d

CLEANFILES += cmd-context.lst fs-context.lst
COMMANDFILES += cmd-context.lst
FSFILES += fs-context.lst

cmd-context.lst: normal/context.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-context.lst: normal/context.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_main.o: normal/main.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_main.d: normal/main.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,main\.o[ :]*,normal_mod-normal_main.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_main.d

CLEANFILES += cmd-main.lst fs-main.lst
COMMANDFILES += cmd-main.lst
FSFILES += fs-main.lst

cmd-main.lst: normal/main.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-main.lst: normal/main.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_menu.o: normal/menu.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_menu.d: normal/menu.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,menu\.o[ :]*,normal_mod-normal_menu.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_menu.d

CLEANFILES += cmd-menu.lst fs-menu.lst
COMMANDFILES += cmd-menu.lst
FSFILES += fs-menu.lst

cmd-menu.lst: normal/menu.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-menu.lst: normal/menu.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_menu_entry.o: normal/menu_entry.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_menu_entry.d: normal/menu_entry.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,menu_entry\.o[ :]*,normal_mod-normal_menu_entry.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_menu_entry.d

CLEANFILES += cmd-menu_entry.lst fs-menu_entry.lst
COMMANDFILES += cmd-menu_entry.lst
FSFILES += fs-menu_entry.lst

cmd-menu_entry.lst: normal/menu_entry.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-menu_entry.lst: normal/menu_entry.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_misc.o: normal/misc.c
	$(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -c -o $@ $<

normal_mod-normal_misc.d: normal/misc.c
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -M $< 	  | sed 's,misc\.o[ :]*,normal_mod-normal_misc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_misc.d

CLEANFILES += cmd-misc.lst fs-misc.lst
COMMANDFILES += cmd-misc.lst
FSFILES += fs-misc.lst

cmd-misc.lst: normal/misc.c gencmdlist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-misc.lst: normal/misc.c genfslist.sh
	set -e; 	  $(CC) -Inormal -I$(srcdir)/normal $(CPPFLAGS) $(CFLAGS) $(normal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod-normal_powerpc_setjmp.o: normal/powerpc/setjmp.S
	$(CC) -Inormal/powerpc -I$(srcdir)/normal/powerpc $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -c -o $@ $<

normal_mod-normal_powerpc_setjmp.d: normal/powerpc/setjmp.S
	set -e; 	  $(CC) -Inormal/powerpc -I$(srcdir)/normal/powerpc $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -M $< 	  | sed 's,setjmp\.o[ :]*,normal_mod-normal_powerpc_setjmp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include normal_mod-normal_powerpc_setjmp.d

CLEANFILES += cmd-setjmp.lst fs-setjmp.lst
COMMANDFILES += cmd-setjmp.lst
FSFILES += fs-setjmp.lst

cmd-setjmp.lst: normal/powerpc/setjmp.S gencmdlist.sh
	set -e; 	  $(CC) -Inormal/powerpc -I$(srcdir)/normal/powerpc $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh normal > $@ || (rm -f $@; exit 1)

fs-setjmp.lst: normal/powerpc/setjmp.S genfslist.sh
	set -e; 	  $(CC) -Inormal/powerpc -I$(srcdir)/normal/powerpc $(CPPFLAGS) $(ASFLAGS) $(normal_mod_ASFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh normal > $@ || (rm -f $@; exit 1)


normal_mod_CFLAGS = $(COMMON_CFLAGS)
normal_mod_ASFLAGS = $(COMMON_ASFLAGS)

# For hello.mod.
hello_mod_SOURCES = hello/hello.c
CLEANFILES += hello.mod mod-hello.o mod-hello.c pre-hello.o hello_mod-hello_hello.o def-hello.lst und-hello.lst
MOSTLYCLEANFILES += hello_mod-hello_hello.d
DEFSYMFILES += def-hello.lst
UNDSYMFILES += und-hello.lst

hello.mod: pre-hello.o mod-hello.o
	-rm -f $@
	$(LD) $(hello_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-hello.o: hello_mod-hello_hello.o
	-rm -f $@
	$(LD) $(hello_mod_LDFLAGS) -r -d -o $@ $^

mod-hello.o: mod-hello.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(hello_mod_CFLAGS) -c -o $@ $<

mod-hello.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'hello' $< > $@ || (rm -f $@; exit 1)

def-hello.lst: pre-hello.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 hello/' > $@

und-hello.lst: pre-hello.o
	echo 'hello' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

hello_mod-hello_hello.o: hello/hello.c
	$(CC) -Ihello -I$(srcdir)/hello $(CPPFLAGS) $(CFLAGS) $(hello_mod_CFLAGS) -c -o $@ $<

hello_mod-hello_hello.d: hello/hello.c
	set -e; 	  $(CC) -Ihello -I$(srcdir)/hello $(CPPFLAGS) $(CFLAGS) $(hello_mod_CFLAGS) -M $< 	  | sed 's,hello\.o[ :]*,hello_mod-hello_hello.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include hello_mod-hello_hello.d

CLEANFILES += cmd-hello.lst fs-hello.lst
COMMANDFILES += cmd-hello.lst
FSFILES += fs-hello.lst

cmd-hello.lst: hello/hello.c gencmdlist.sh
	set -e; 	  $(CC) -Ihello -I$(srcdir)/hello $(CPPFLAGS) $(CFLAGS) $(hello_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh hello > $@ || (rm -f $@; exit 1)

fs-hello.lst: hello/hello.c genfslist.sh
	set -e; 	  $(CC) -Ihello -I$(srcdir)/hello $(CPPFLAGS) $(CFLAGS) $(hello_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh hello > $@ || (rm -f $@; exit 1)


hello_mod_CFLAGS = $(COMMON_CFLAGS)

# For boot.mod.
boot_mod_SOURCES = commands/boot.c
CLEANFILES += boot.mod mod-boot.o mod-boot.c pre-boot.o boot_mod-commands_boot.o def-boot.lst und-boot.lst
MOSTLYCLEANFILES += boot_mod-commands_boot.d
DEFSYMFILES += def-boot.lst
UNDSYMFILES += und-boot.lst

boot.mod: pre-boot.o mod-boot.o
	-rm -f $@
	$(LD) $(boot_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-boot.o: boot_mod-commands_boot.o
	-rm -f $@
	$(LD) $(boot_mod_LDFLAGS) -r -d -o $@ $^

mod-boot.o: mod-boot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(boot_mod_CFLAGS) -c -o $@ $<

mod-boot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'boot' $< > $@ || (rm -f $@; exit 1)

def-boot.lst: pre-boot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 boot/' > $@

und-boot.lst: pre-boot.o
	echo 'boot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

boot_mod-commands_boot.o: commands/boot.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(boot_mod_CFLAGS) -c -o $@ $<

boot_mod-commands_boot.d: commands/boot.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(boot_mod_CFLAGS) -M $< 	  | sed 's,boot\.o[ :]*,boot_mod-commands_boot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include boot_mod-commands_boot.d

CLEANFILES += cmd-boot.lst fs-boot.lst
COMMANDFILES += cmd-boot.lst
FSFILES += fs-boot.lst

cmd-boot.lst: commands/boot.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(boot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh boot > $@ || (rm -f $@; exit 1)

fs-boot.lst: commands/boot.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(boot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh boot > $@ || (rm -f $@; exit 1)


boot_mod_CFLAGS = $(COMMON_CFLAGS)

# For terminal.mod.
terminal_mod_SOURCES = commands/terminal.c
CLEANFILES += terminal.mod mod-terminal.o mod-terminal.c pre-terminal.o terminal_mod-commands_terminal.o def-terminal.lst und-terminal.lst
MOSTLYCLEANFILES += terminal_mod-commands_terminal.d
DEFSYMFILES += def-terminal.lst
UNDSYMFILES += und-terminal.lst

terminal.mod: pre-terminal.o mod-terminal.o
	-rm -f $@
	$(LD) $(terminal_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-terminal.o: terminal_mod-commands_terminal.o
	-rm -f $@
	$(LD) $(terminal_mod_LDFLAGS) -r -d -o $@ $^

mod-terminal.o: mod-terminal.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(terminal_mod_CFLAGS) -c -o $@ $<

mod-terminal.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'terminal' $< > $@ || (rm -f $@; exit 1)

def-terminal.lst: pre-terminal.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 terminal/' > $@

und-terminal.lst: pre-terminal.o
	echo 'terminal' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

terminal_mod-commands_terminal.o: commands/terminal.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(terminal_mod_CFLAGS) -c -o $@ $<

terminal_mod-commands_terminal.d: commands/terminal.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(terminal_mod_CFLAGS) -M $< 	  | sed 's,terminal\.o[ :]*,terminal_mod-commands_terminal.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include terminal_mod-commands_terminal.d

CLEANFILES += cmd-terminal.lst fs-terminal.lst
COMMANDFILES += cmd-terminal.lst
FSFILES += fs-terminal.lst

cmd-terminal.lst: commands/terminal.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(terminal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh terminal > $@ || (rm -f $@; exit 1)

fs-terminal.lst: commands/terminal.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(terminal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh terminal > $@ || (rm -f $@; exit 1)


terminal_mod_CFLAGS = $(COMMON_CFLAGS)

# For ls.mod.
ls_mod_SOURCES = commands/ls.c
CLEANFILES += ls.mod mod-ls.o mod-ls.c pre-ls.o ls_mod-commands_ls.o def-ls.lst und-ls.lst
MOSTLYCLEANFILES += ls_mod-commands_ls.d
DEFSYMFILES += def-ls.lst
UNDSYMFILES += und-ls.lst

ls.mod: pre-ls.o mod-ls.o
	-rm -f $@
	$(LD) $(ls_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ls.o: ls_mod-commands_ls.o
	-rm -f $@
	$(LD) $(ls_mod_LDFLAGS) -r -d -o $@ $^

mod-ls.o: mod-ls.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ls_mod_CFLAGS) -c -o $@ $<

mod-ls.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'ls' $< > $@ || (rm -f $@; exit 1)

def-ls.lst: pre-ls.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 ls/' > $@

und-ls.lst: pre-ls.o
	echo 'ls' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

ls_mod-commands_ls.o: commands/ls.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(ls_mod_CFLAGS) -c -o $@ $<

ls_mod-commands_ls.d: commands/ls.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(ls_mod_CFLAGS) -M $< 	  | sed 's,ls\.o[ :]*,ls_mod-commands_ls.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include ls_mod-commands_ls.d

CLEANFILES += cmd-ls.lst fs-ls.lst
COMMANDFILES += cmd-ls.lst
FSFILES += fs-ls.lst

cmd-ls.lst: commands/ls.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(ls_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ls > $@ || (rm -f $@; exit 1)

fs-ls.lst: commands/ls.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(ls_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ls > $@ || (rm -f $@; exit 1)


ls_mod_CFLAGS = $(COMMON_CFLAGS)

# For cmp.mod.
cmp_mod_SOURCES = commands/cmp.c
CLEANFILES += cmp.mod mod-cmp.o mod-cmp.c pre-cmp.o cmp_mod-commands_cmp.o def-cmp.lst und-cmp.lst
MOSTLYCLEANFILES += cmp_mod-commands_cmp.d
DEFSYMFILES += def-cmp.lst
UNDSYMFILES += und-cmp.lst

cmp.mod: pre-cmp.o mod-cmp.o
	-rm -f $@
	$(LD) $(cmp_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-cmp.o: cmp_mod-commands_cmp.o
	-rm -f $@
	$(LD) $(cmp_mod_LDFLAGS) -r -d -o $@ $^

mod-cmp.o: mod-cmp.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(cmp_mod_CFLAGS) -c -o $@ $<

mod-cmp.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'cmp' $< > $@ || (rm -f $@; exit 1)

def-cmp.lst: pre-cmp.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 cmp/' > $@

und-cmp.lst: pre-cmp.o
	echo 'cmp' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

cmp_mod-commands_cmp.o: commands/cmp.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cmp_mod_CFLAGS) -c -o $@ $<

cmp_mod-commands_cmp.d: commands/cmp.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cmp_mod_CFLAGS) -M $< 	  | sed 's,cmp\.o[ :]*,cmp_mod-commands_cmp.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include cmp_mod-commands_cmp.d

CLEANFILES += cmd-cmp.lst fs-cmp.lst
COMMANDFILES += cmd-cmp.lst
FSFILES += fs-cmp.lst

cmd-cmp.lst: commands/cmp.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cmp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh cmp > $@ || (rm -f $@; exit 1)

fs-cmp.lst: commands/cmp.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cmp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh cmp > $@ || (rm -f $@; exit 1)


cmp_mod_CFLAGS = $(COMMON_CFLAGS)

# For cat.mod.
cat_mod_SOURCES = commands/cat.c
CLEANFILES += cat.mod mod-cat.o mod-cat.c pre-cat.o cat_mod-commands_cat.o def-cat.lst und-cat.lst
MOSTLYCLEANFILES += cat_mod-commands_cat.d
DEFSYMFILES += def-cat.lst
UNDSYMFILES += und-cat.lst

cat.mod: pre-cat.o mod-cat.o
	-rm -f $@
	$(LD) $(cat_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-cat.o: cat_mod-commands_cat.o
	-rm -f $@
	$(LD) $(cat_mod_LDFLAGS) -r -d -o $@ $^

mod-cat.o: mod-cat.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(cat_mod_CFLAGS) -c -o $@ $<

mod-cat.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'cat' $< > $@ || (rm -f $@; exit 1)

def-cat.lst: pre-cat.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 cat/' > $@

und-cat.lst: pre-cat.o
	echo 'cat' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

cat_mod-commands_cat.o: commands/cat.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cat_mod_CFLAGS) -c -o $@ $<

cat_mod-commands_cat.d: commands/cat.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cat_mod_CFLAGS) -M $< 	  | sed 's,cat\.o[ :]*,cat_mod-commands_cat.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include cat_mod-commands_cat.d

CLEANFILES += cmd-cat.lst fs-cat.lst
COMMANDFILES += cmd-cat.lst
FSFILES += fs-cat.lst

cmd-cat.lst: commands/cat.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh cat > $@ || (rm -f $@; exit 1)

fs-cat.lst: commands/cat.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(cat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh cat > $@ || (rm -f $@; exit 1)


cat_mod_CFLAGS = $(COMMON_CFLAGS)

# For font.mod.
font_mod_SOURCES = font/manager.c
CLEANFILES += font.mod mod-font.o mod-font.c pre-font.o font_mod-font_manager.o def-font.lst und-font.lst
MOSTLYCLEANFILES += font_mod-font_manager.d
DEFSYMFILES += def-font.lst
UNDSYMFILES += und-font.lst

font.mod: pre-font.o mod-font.o
	-rm -f $@
	$(LD) $(font_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-font.o: font_mod-font_manager.o
	-rm -f $@
	$(LD) $(font_mod_LDFLAGS) -r -d -o $@ $^

mod-font.o: mod-font.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(font_mod_CFLAGS) -c -o $@ $<

mod-font.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'font' $< > $@ || (rm -f $@; exit 1)

def-font.lst: pre-font.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 font/' > $@

und-font.lst: pre-font.o
	echo 'font' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

font_mod-font_manager.o: font/manager.c
	$(CC) -Ifont -I$(srcdir)/font $(CPPFLAGS) $(CFLAGS) $(font_mod_CFLAGS) -c -o $@ $<

font_mod-font_manager.d: font/manager.c
	set -e; 	  $(CC) -Ifont -I$(srcdir)/font $(CPPFLAGS) $(CFLAGS) $(font_mod_CFLAGS) -M $< 	  | sed 's,manager\.o[ :]*,font_mod-font_manager.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include font_mod-font_manager.d

CLEANFILES += cmd-manager.lst fs-manager.lst
COMMANDFILES += cmd-manager.lst
FSFILES += fs-manager.lst

cmd-manager.lst: font/manager.c gencmdlist.sh
	set -e; 	  $(CC) -Ifont -I$(srcdir)/font $(CPPFLAGS) $(CFLAGS) $(font_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh font > $@ || (rm -f $@; exit 1)

fs-manager.lst: font/manager.c genfslist.sh
	set -e; 	  $(CC) -Ifont -I$(srcdir)/font $(CPPFLAGS) $(CFLAGS) $(font_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh font > $@ || (rm -f $@; exit 1)


font_mod_CFLAGS = $(COMMON_CFLAGS)

# For amiga.mod
amiga_mod_SOURCES = partmap/amiga.c
CLEANFILES += amiga.mod mod-amiga.o mod-amiga.c pre-amiga.o amiga_mod-partmap_amiga.o def-amiga.lst und-amiga.lst
MOSTLYCLEANFILES += amiga_mod-partmap_amiga.d
DEFSYMFILES += def-amiga.lst
UNDSYMFILES += und-amiga.lst

amiga.mod: pre-amiga.o mod-amiga.o
	-rm -f $@
	$(LD) $(amiga_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-amiga.o: amiga_mod-partmap_amiga.o
	-rm -f $@
	$(LD) $(amiga_mod_LDFLAGS) -r -d -o $@ $^

mod-amiga.o: mod-amiga.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(amiga_mod_CFLAGS) -c -o $@ $<

mod-amiga.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'amiga' $< > $@ || (rm -f $@; exit 1)

def-amiga.lst: pre-amiga.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 amiga/' > $@

und-amiga.lst: pre-amiga.o
	echo 'amiga' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

amiga_mod-partmap_amiga.o: partmap/amiga.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(amiga_mod_CFLAGS) -c -o $@ $<

amiga_mod-partmap_amiga.d: partmap/amiga.c
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(amiga_mod_CFLAGS) -M $< 	  | sed 's,amiga\.o[ :]*,amiga_mod-partmap_amiga.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include amiga_mod-partmap_amiga.d

CLEANFILES += cmd-amiga.lst fs-amiga.lst
COMMANDFILES += cmd-amiga.lst
FSFILES += fs-amiga.lst

cmd-amiga.lst: partmap/amiga.c gencmdlist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(amiga_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh amiga > $@ || (rm -f $@; exit 1)

fs-amiga.lst: partmap/amiga.c genfslist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(amiga_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh amiga > $@ || (rm -f $@; exit 1)


amiga_mod_CFLAGS = $(COMMON_CFLAGS)

# For apple.mod
apple_mod_SOURCES = partmap/apple.c
CLEANFILES += apple.mod mod-apple.o mod-apple.c pre-apple.o apple_mod-partmap_apple.o def-apple.lst und-apple.lst
MOSTLYCLEANFILES += apple_mod-partmap_apple.d
DEFSYMFILES += def-apple.lst
UNDSYMFILES += und-apple.lst

apple.mod: pre-apple.o mod-apple.o
	-rm -f $@
	$(LD) $(apple_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-apple.o: apple_mod-partmap_apple.o
	-rm -f $@
	$(LD) $(apple_mod_LDFLAGS) -r -d -o $@ $^

mod-apple.o: mod-apple.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(apple_mod_CFLAGS) -c -o $@ $<

mod-apple.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'apple' $< > $@ || (rm -f $@; exit 1)

def-apple.lst: pre-apple.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 apple/' > $@

und-apple.lst: pre-apple.o
	echo 'apple' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

apple_mod-partmap_apple.o: partmap/apple.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(apple_mod_CFLAGS) -c -o $@ $<

apple_mod-partmap_apple.d: partmap/apple.c
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(apple_mod_CFLAGS) -M $< 	  | sed 's,apple\.o[ :]*,apple_mod-partmap_apple.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include apple_mod-partmap_apple.d

CLEANFILES += cmd-apple.lst fs-apple.lst
COMMANDFILES += cmd-apple.lst
FSFILES += fs-apple.lst

cmd-apple.lst: partmap/apple.c gencmdlist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(apple_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh apple > $@ || (rm -f $@; exit 1)

fs-apple.lst: partmap/apple.c genfslist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(apple_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh apple > $@ || (rm -f $@; exit 1)


apple_mod_CFLAGS = $(COMMON_CFLAGS)

# For pc.mod
pc_mod_SOURCES = partmap/pc.c
CLEANFILES += pc.mod mod-pc.o mod-pc.c pre-pc.o pc_mod-partmap_pc.o def-pc.lst und-pc.lst
MOSTLYCLEANFILES += pc_mod-partmap_pc.d
DEFSYMFILES += def-pc.lst
UNDSYMFILES += und-pc.lst

pc.mod: pre-pc.o mod-pc.o
	-rm -f $@
	$(LD) $(pc_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-pc.o: pc_mod-partmap_pc.o
	-rm -f $@
	$(LD) $(pc_mod_LDFLAGS) -r -d -o $@ $^

mod-pc.o: mod-pc.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(pc_mod_CFLAGS) -c -o $@ $<

mod-pc.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'pc' $< > $@ || (rm -f $@; exit 1)

def-pc.lst: pre-pc.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 pc/' > $@

und-pc.lst: pre-pc.o
	echo 'pc' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

pc_mod-partmap_pc.o: partmap/pc.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(pc_mod_CFLAGS) -c -o $@ $<

pc_mod-partmap_pc.d: partmap/pc.c
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(pc_mod_CFLAGS) -M $< 	  | sed 's,pc\.o[ :]*,pc_mod-partmap_pc.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include pc_mod-partmap_pc.d

CLEANFILES += cmd-pc.lst fs-pc.lst
COMMANDFILES += cmd-pc.lst
FSFILES += fs-pc.lst

cmd-pc.lst: partmap/pc.c gencmdlist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(pc_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh pc > $@ || (rm -f $@; exit 1)

fs-pc.lst: partmap/pc.c genfslist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(pc_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh pc > $@ || (rm -f $@; exit 1)


pc_mod_CFLAGS = $(COMMON_CFLAGS)

# For sun.mod
sun_mod_SOURCES = partmap/sun.c
CLEANFILES += sun.mod mod-sun.o mod-sun.c pre-sun.o sun_mod-partmap_sun.o def-sun.lst und-sun.lst
MOSTLYCLEANFILES += sun_mod-partmap_sun.d
DEFSYMFILES += def-sun.lst
UNDSYMFILES += und-sun.lst

sun.mod: pre-sun.o mod-sun.o
	-rm -f $@
	$(LD) $(sun_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-sun.o: sun_mod-partmap_sun.o
	-rm -f $@
	$(LD) $(sun_mod_LDFLAGS) -r -d -o $@ $^

mod-sun.o: mod-sun.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(sun_mod_CFLAGS) -c -o $@ $<

mod-sun.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'sun' $< > $@ || (rm -f $@; exit 1)

def-sun.lst: pre-sun.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 sun/' > $@

und-sun.lst: pre-sun.o
	echo 'sun' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

sun_mod-partmap_sun.o: partmap/sun.c
	$(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(sun_mod_CFLAGS) -c -o $@ $<

sun_mod-partmap_sun.d: partmap/sun.c
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(sun_mod_CFLAGS) -M $< 	  | sed 's,sun\.o[ :]*,sun_mod-partmap_sun.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include sun_mod-partmap_sun.d

CLEANFILES += cmd-sun.lst fs-sun.lst
COMMANDFILES += cmd-sun.lst
FSFILES += fs-sun.lst

cmd-sun.lst: partmap/sun.c gencmdlist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(sun_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh sun > $@ || (rm -f $@; exit 1)

fs-sun.lst: partmap/sun.c genfslist.sh
	set -e; 	  $(CC) -Ipartmap -I$(srcdir)/partmap $(CPPFLAGS) $(CFLAGS) $(sun_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh sun > $@ || (rm -f $@; exit 1)


sun_mod_CFLAGS = $(COMMON_CFLAGS)

# For loopback.mod
loopback_mod_SOURCES = disk/loopback.c
CLEANFILES += loopback.mod mod-loopback.o mod-loopback.c pre-loopback.o loopback_mod-disk_loopback.o def-loopback.lst und-loopback.lst
MOSTLYCLEANFILES += loopback_mod-disk_loopback.d
DEFSYMFILES += def-loopback.lst
UNDSYMFILES += und-loopback.lst

loopback.mod: pre-loopback.o mod-loopback.o
	-rm -f $@
	$(LD) $(loopback_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-loopback.o: loopback_mod-disk_loopback.o
	-rm -f $@
	$(LD) $(loopback_mod_LDFLAGS) -r -d -o $@ $^

mod-loopback.o: mod-loopback.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(loopback_mod_CFLAGS) -c -o $@ $<

mod-loopback.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'loopback' $< > $@ || (rm -f $@; exit 1)

def-loopback.lst: pre-loopback.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 loopback/' > $@

und-loopback.lst: pre-loopback.o
	echo 'loopback' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

loopback_mod-disk_loopback.o: disk/loopback.c
	$(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) $(loopback_mod_CFLAGS) -c -o $@ $<

loopback_mod-disk_loopback.d: disk/loopback.c
	set -e; 	  $(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) $(loopback_mod_CFLAGS) -M $< 	  | sed 's,loopback\.o[ :]*,loopback_mod-disk_loopback.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include loopback_mod-disk_loopback.d

CLEANFILES += cmd-loopback.lst fs-loopback.lst
COMMANDFILES += cmd-loopback.lst
FSFILES += fs-loopback.lst

cmd-loopback.lst: disk/loopback.c gencmdlist.sh
	set -e; 	  $(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) $(loopback_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh loopback > $@ || (rm -f $@; exit 1)

fs-loopback.lst: disk/loopback.c genfslist.sh
	set -e; 	  $(CC) -Idisk -I$(srcdir)/disk $(CPPFLAGS) $(CFLAGS) $(loopback_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh loopback > $@ || (rm -f $@; exit 1)


loopback_mod_CFLAGS = $(COMMON_CFLAGS)

# For suspend.mod
suspend_mod_SOURCES = commands/ieee1275/suspend.c
CLEANFILES += suspend.mod mod-suspend.o mod-suspend.c pre-suspend.o suspend_mod-commands_ieee1275_suspend.o def-suspend.lst und-suspend.lst
MOSTLYCLEANFILES += suspend_mod-commands_ieee1275_suspend.d
DEFSYMFILES += def-suspend.lst
UNDSYMFILES += und-suspend.lst

suspend.mod: pre-suspend.o mod-suspend.o
	-rm -f $@
	$(LD) $(suspend_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-suspend.o: suspend_mod-commands_ieee1275_suspend.o
	-rm -f $@
	$(LD) $(suspend_mod_LDFLAGS) -r -d -o $@ $^

mod-suspend.o: mod-suspend.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(suspend_mod_CFLAGS) -c -o $@ $<

mod-suspend.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'suspend' $< > $@ || (rm -f $@; exit 1)

def-suspend.lst: pre-suspend.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 suspend/' > $@

und-suspend.lst: pre-suspend.o
	echo 'suspend' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

suspend_mod-commands_ieee1275_suspend.o: commands/ieee1275/suspend.c
	$(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(suspend_mod_CFLAGS) -c -o $@ $<

suspend_mod-commands_ieee1275_suspend.d: commands/ieee1275/suspend.c
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(suspend_mod_CFLAGS) -M $< 	  | sed 's,suspend\.o[ :]*,suspend_mod-commands_ieee1275_suspend.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include suspend_mod-commands_ieee1275_suspend.d

CLEANFILES += cmd-suspend.lst fs-suspend.lst
COMMANDFILES += cmd-suspend.lst
FSFILES += fs-suspend.lst

cmd-suspend.lst: commands/ieee1275/suspend.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(suspend_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh suspend > $@ || (rm -f $@; exit 1)

fs-suspend.lst: commands/ieee1275/suspend.c genfslist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(suspend_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh suspend > $@ || (rm -f $@; exit 1)


suspend_mod_CFLAGS = $(COMMON_CFLAGS)

# For reboot.mod
reboot_mod_SOURCES = commands/ieee1275/reboot.c
CLEANFILES += reboot.mod mod-reboot.o mod-reboot.c pre-reboot.o reboot_mod-commands_ieee1275_reboot.o def-reboot.lst und-reboot.lst
MOSTLYCLEANFILES += reboot_mod-commands_ieee1275_reboot.d
DEFSYMFILES += def-reboot.lst
UNDSYMFILES += und-reboot.lst

reboot.mod: pre-reboot.o mod-reboot.o
	-rm -f $@
	$(LD) $(reboot_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-reboot.o: reboot_mod-commands_ieee1275_reboot.o
	-rm -f $@
	$(LD) $(reboot_mod_LDFLAGS) -r -d -o $@ $^

mod-reboot.o: mod-reboot.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -c -o $@ $<

mod-reboot.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'reboot' $< > $@ || (rm -f $@; exit 1)

def-reboot.lst: pre-reboot.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 reboot/' > $@

und-reboot.lst: pre-reboot.o
	echo 'reboot' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

reboot_mod-commands_ieee1275_reboot.o: commands/ieee1275/reboot.c
	$(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -c -o $@ $<

reboot_mod-commands_ieee1275_reboot.d: commands/ieee1275/reboot.c
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -M $< 	  | sed 's,reboot\.o[ :]*,reboot_mod-commands_ieee1275_reboot.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include reboot_mod-commands_ieee1275_reboot.d

CLEANFILES += cmd-reboot.lst fs-reboot.lst
COMMANDFILES += cmd-reboot.lst
FSFILES += fs-reboot.lst

cmd-reboot.lst: commands/ieee1275/reboot.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh reboot > $@ || (rm -f $@; exit 1)

fs-reboot.lst: commands/ieee1275/reboot.c genfslist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(reboot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh reboot > $@ || (rm -f $@; exit 1)


reboot_mod_CFLAGS = $(COMMON_CFLAGS)

# For halt.mod
halt_mod_SOURCES = commands/ieee1275/halt.c
CLEANFILES += halt.mod mod-halt.o mod-halt.c pre-halt.o halt_mod-commands_ieee1275_halt.o def-halt.lst und-halt.lst
MOSTLYCLEANFILES += halt_mod-commands_ieee1275_halt.d
DEFSYMFILES += def-halt.lst
UNDSYMFILES += und-halt.lst

halt.mod: pre-halt.o mod-halt.o
	-rm -f $@
	$(LD) $(halt_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-halt.o: halt_mod-commands_ieee1275_halt.o
	-rm -f $@
	$(LD) $(halt_mod_LDFLAGS) -r -d -o $@ $^

mod-halt.o: mod-halt.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -c -o $@ $<

mod-halt.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'halt' $< > $@ || (rm -f $@; exit 1)

def-halt.lst: pre-halt.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 halt/' > $@

und-halt.lst: pre-halt.o
	echo 'halt' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

halt_mod-commands_ieee1275_halt.o: commands/ieee1275/halt.c
	$(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -c -o $@ $<

halt_mod-commands_ieee1275_halt.d: commands/ieee1275/halt.c
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -M $< 	  | sed 's,halt\.o[ :]*,halt_mod-commands_ieee1275_halt.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include halt_mod-commands_ieee1275_halt.d

CLEANFILES += cmd-halt.lst fs-halt.lst
COMMANDFILES += cmd-halt.lst
FSFILES += fs-halt.lst

cmd-halt.lst: commands/ieee1275/halt.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh halt > $@ || (rm -f $@; exit 1)

fs-halt.lst: commands/ieee1275/halt.c genfslist.sh
	set -e; 	  $(CC) -Icommands/ieee1275 -I$(srcdir)/commands/ieee1275 $(CPPFLAGS) $(CFLAGS) $(halt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh halt > $@ || (rm -f $@; exit 1)


halt_mod_CFLAGS = $(COMMON_CFLAGS)

# For help.mod.
help_mod_SOURCES = commands/help.c
CLEANFILES += help.mod mod-help.o mod-help.c pre-help.o help_mod-commands_help.o def-help.lst und-help.lst
MOSTLYCLEANFILES += help_mod-commands_help.d
DEFSYMFILES += def-help.lst
UNDSYMFILES += und-help.lst

help.mod: pre-help.o mod-help.o
	-rm -f $@
	$(LD) $(help_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-help.o: help_mod-commands_help.o
	-rm -f $@
	$(LD) $(help_mod_LDFLAGS) -r -d -o $@ $^

mod-help.o: mod-help.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(help_mod_CFLAGS) -c -o $@ $<

mod-help.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'help' $< > $@ || (rm -f $@; exit 1)

def-help.lst: pre-help.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 help/' > $@

und-help.lst: pre-help.o
	echo 'help' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

help_mod-commands_help.o: commands/help.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(help_mod_CFLAGS) -c -o $@ $<

help_mod-commands_help.d: commands/help.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(help_mod_CFLAGS) -M $< 	  | sed 's,help\.o[ :]*,help_mod-commands_help.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include help_mod-commands_help.d

CLEANFILES += cmd-help.lst fs-help.lst
COMMANDFILES += cmd-help.lst
FSFILES += fs-help.lst

cmd-help.lst: commands/help.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(help_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh help > $@ || (rm -f $@; exit 1)

fs-help.lst: commands/help.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(help_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh help > $@ || (rm -f $@; exit 1)


help_mod_CFLAGS = $(COMMON_CFLAGS)

# For default.mod
default_mod_SOURCES = commands/default.c
CLEANFILES += default.mod mod-default.o mod-default.c pre-default.o default_mod-commands_default.o def-default.lst und-default.lst
MOSTLYCLEANFILES += default_mod-commands_default.d
DEFSYMFILES += def-default.lst
UNDSYMFILES += und-default.lst

default.mod: pre-default.o mod-default.o
	-rm -f $@
	$(LD) $(default_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-default.o: default_mod-commands_default.o
	-rm -f $@
	$(LD) $(default_mod_LDFLAGS) -r -d -o $@ $^

mod-default.o: mod-default.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(default_mod_CFLAGS) -c -o $@ $<

mod-default.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'default' $< > $@ || (rm -f $@; exit 1)

def-default.lst: pre-default.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 default/' > $@

und-default.lst: pre-default.o
	echo 'default' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

default_mod-commands_default.o: commands/default.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(default_mod_CFLAGS) -c -o $@ $<

default_mod-commands_default.d: commands/default.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(default_mod_CFLAGS) -M $< 	  | sed 's,default\.o[ :]*,default_mod-commands_default.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include default_mod-commands_default.d

CLEANFILES += cmd-default.lst fs-default.lst
COMMANDFILES += cmd-default.lst
FSFILES += fs-default.lst

cmd-default.lst: commands/default.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(default_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh default > $@ || (rm -f $@; exit 1)

fs-default.lst: commands/default.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(default_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh default > $@ || (rm -f $@; exit 1)


default_mod_CFLAGS =  $(COMMON_CFLAGS)

# For timeout.mod
timeout_mod_SOURCES = commands/timeout.c
CLEANFILES += timeout.mod mod-timeout.o mod-timeout.c pre-timeout.o timeout_mod-commands_timeout.o def-timeout.lst und-timeout.lst
MOSTLYCLEANFILES += timeout_mod-commands_timeout.d
DEFSYMFILES += def-timeout.lst
UNDSYMFILES += und-timeout.lst

timeout.mod: pre-timeout.o mod-timeout.o
	-rm -f $@
	$(LD) $(timeout_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-timeout.o: timeout_mod-commands_timeout.o
	-rm -f $@
	$(LD) $(timeout_mod_LDFLAGS) -r -d -o $@ $^

mod-timeout.o: mod-timeout.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(timeout_mod_CFLAGS) -c -o $@ $<

mod-timeout.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'timeout' $< > $@ || (rm -f $@; exit 1)

def-timeout.lst: pre-timeout.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 timeout/' > $@

und-timeout.lst: pre-timeout.o
	echo 'timeout' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

timeout_mod-commands_timeout.o: commands/timeout.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(timeout_mod_CFLAGS) -c -o $@ $<

timeout_mod-commands_timeout.d: commands/timeout.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(timeout_mod_CFLAGS) -M $< 	  | sed 's,timeout\.o[ :]*,timeout_mod-commands_timeout.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include timeout_mod-commands_timeout.d

CLEANFILES += cmd-timeout.lst fs-timeout.lst
COMMANDFILES += cmd-timeout.lst
FSFILES += fs-timeout.lst

cmd-timeout.lst: commands/timeout.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(timeout_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh timeout > $@ || (rm -f $@; exit 1)

fs-timeout.lst: commands/timeout.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(timeout_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh timeout > $@ || (rm -f $@; exit 1)


timeout_mod_CFLAGS =  $(COMMON_CFLAGS)

# For configfile.mod
configfile_mod_SOURCES = commands/configfile.c
CLEANFILES += configfile.mod mod-configfile.o mod-configfile.c pre-configfile.o configfile_mod-commands_configfile.o def-configfile.lst und-configfile.lst
MOSTLYCLEANFILES += configfile_mod-commands_configfile.d
DEFSYMFILES += def-configfile.lst
UNDSYMFILES += und-configfile.lst

configfile.mod: pre-configfile.o mod-configfile.o
	-rm -f $@
	$(LD) $(configfile_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-configfile.o: configfile_mod-commands_configfile.o
	-rm -f $@
	$(LD) $(configfile_mod_LDFLAGS) -r -d -o $@ $^

mod-configfile.o: mod-configfile.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(configfile_mod_CFLAGS) -c -o $@ $<

mod-configfile.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'configfile' $< > $@ || (rm -f $@; exit 1)

def-configfile.lst: pre-configfile.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 configfile/' > $@

und-configfile.lst: pre-configfile.o
	echo 'configfile' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

configfile_mod-commands_configfile.o: commands/configfile.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(configfile_mod_CFLAGS) -c -o $@ $<

configfile_mod-commands_configfile.d: commands/configfile.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(configfile_mod_CFLAGS) -M $< 	  | sed 's,configfile\.o[ :]*,configfile_mod-commands_configfile.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include configfile_mod-commands_configfile.d

CLEANFILES += cmd-configfile.lst fs-configfile.lst
COMMANDFILES += cmd-configfile.lst
FSFILES += fs-configfile.lst

cmd-configfile.lst: commands/configfile.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(configfile_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh configfile > $@ || (rm -f $@; exit 1)

fs-configfile.lst: commands/configfile.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(configfile_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh configfile > $@ || (rm -f $@; exit 1)


configfile_mod_CFLAGS = $(COMMON_CFLAGS)

# For search.mod.
search_mod_SOURCES = commands/search.c
CLEANFILES += search.mod mod-search.o mod-search.c pre-search.o search_mod-commands_search.o def-search.lst und-search.lst
MOSTLYCLEANFILES += search_mod-commands_search.d
DEFSYMFILES += def-search.lst
UNDSYMFILES += und-search.lst

search.mod: pre-search.o mod-search.o
	-rm -f $@
	$(LD) $(search_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-search.o: search_mod-commands_search.o
	-rm -f $@
	$(LD) $(search_mod_LDFLAGS) -r -d -o $@ $^

mod-search.o: mod-search.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(search_mod_CFLAGS) -c -o $@ $<

mod-search.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'search' $< > $@ || (rm -f $@; exit 1)

def-search.lst: pre-search.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 search/' > $@

und-search.lst: pre-search.o
	echo 'search' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

search_mod-commands_search.o: commands/search.c
	$(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(search_mod_CFLAGS) -c -o $@ $<

search_mod-commands_search.d: commands/search.c
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(search_mod_CFLAGS) -M $< 	  | sed 's,search\.o[ :]*,search_mod-commands_search.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include search_mod-commands_search.d

CLEANFILES += cmd-search.lst fs-search.lst
COMMANDFILES += cmd-search.lst
FSFILES += fs-search.lst

cmd-search.lst: commands/search.c gencmdlist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(search_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh search > $@ || (rm -f $@; exit 1)

fs-search.lst: commands/search.c genfslist.sh
	set -e; 	  $(CC) -Icommands -I$(srcdir)/commands $(CPPFLAGS) $(CFLAGS) $(search_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh search > $@ || (rm -f $@; exit 1)


search_mod_CFLAGS = $(COMMON_CFLAGS)

# For gzio.mod.
gzio_mod_SOURCES = io/gzio.c
CLEANFILES += gzio.mod mod-gzio.o mod-gzio.c pre-gzio.o gzio_mod-io_gzio.o def-gzio.lst und-gzio.lst
MOSTLYCLEANFILES += gzio_mod-io_gzio.d
DEFSYMFILES += def-gzio.lst
UNDSYMFILES += und-gzio.lst

gzio.mod: pre-gzio.o mod-gzio.o
	-rm -f $@
	$(LD) $(gzio_mod_LDFLAGS) $(LDFLAGS) -r -d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-gzio.o: gzio_mod-io_gzio.o
	-rm -f $@
	$(LD) $(gzio_mod_LDFLAGS) -r -d -o $@ $^

mod-gzio.o: mod-gzio.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(gzio_mod_CFLAGS) -c -o $@ $<

mod-gzio.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'gzio' $< > $@ || (rm -f $@; exit 1)

def-gzio.lst: pre-gzio.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 gzio/' > $@

und-gzio.lst: pre-gzio.o
	echo 'gzio' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

gzio_mod-io_gzio.o: io/gzio.c
	$(CC) -Iio -I$(srcdir)/io $(CPPFLAGS) $(CFLAGS) $(gzio_mod_CFLAGS) -c -o $@ $<

gzio_mod-io_gzio.d: io/gzio.c
	set -e; 	  $(CC) -Iio -I$(srcdir)/io $(CPPFLAGS) $(CFLAGS) $(gzio_mod_CFLAGS) -M $< 	  | sed 's,gzio\.o[ :]*,gzio_mod-io_gzio.o $@ : ,g' > $@; 	  [ -s $@ ] || rm -f $@

-include gzio_mod-io_gzio.d

CLEANFILES += cmd-gzio.lst fs-gzio.lst
COMMANDFILES += cmd-gzio.lst
FSFILES += fs-gzio.lst

cmd-gzio.lst: io/gzio.c gencmdlist.sh
	set -e; 	  $(CC) -Iio -I$(srcdir)/io $(CPPFLAGS) $(CFLAGS) $(gzio_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh gzio > $@ || (rm -f $@; exit 1)

fs-gzio.lst: io/gzio.c genfslist.sh
	set -e; 	  $(CC) -Iio -I$(srcdir)/io $(CPPFLAGS) $(CFLAGS) $(gzio_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh gzio > $@ || (rm -f $@; exit 1)


gzio_mod_CFLAGS = $(COMMON_CFLAGS)
CLEANFILES += moddep.lst command.lst fs.lst
pkgdata_DATA += moddep.lst command.lst fs.lst
moddep.lst: $(DEFSYMFILES) $(UNDSYMFILES) genmoddep
	cat $(DEFSYMFILES) /dev/null | ./genmoddep $(UNDSYMFILES) > $@ \
	  || (rm -f $@; exit 1)

command.lst: $(COMMANDFILES)
	cat $^ /dev/null | sort > $@

fs.lst: $(FSFILES)
	cat $^ /dev/null | sort > $@
