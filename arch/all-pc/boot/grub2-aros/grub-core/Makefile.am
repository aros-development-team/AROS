AUTOMAKE_OPTIONS = subdir-objects -Wno-portability

DEPDIR=.deps-core

include $(top_srcdir)/conf/Makefile.common

CC=$(TARGET_CC)
CPP=$(TARGET_CC)
CCAS=$(TARGET_CC)

# AROS FIX
# This allows to build GRUB2 under 64-bit MacOS X and Cygwin using x86_64-elf cross-compiler
# for building core binaries (./configure --target=x86_64-elf).
# Without this MacOS X ranlib is used for indexing libgnulib.a which contains ELF objects,
# breaking the build. On Cygwin cygwin's strip is used on ELF files.
# ar could also be overriden here, but this seems to be not a problem because it's just an
# archiver.
RANLIB=$(TARGET_RANLIB)
STRIP=$(TARGET_STRIP)

if COND_GRUB_MKFONT
if COND_HAVE_FONT_SOURCE
TARGET_CFLAGS += -DUSE_ASCII_FAILBACK=1 -DHAVE_UNIFONT_WIDTHSPEC=1
endif
endif

MACHO2IMG=$(top_builddir)/grub-macho2img

AM_CFLAGS = $(TARGET_CFLAGS)
AM_LDFLAGS = $(TARGET_LDFLAGS)
AM_CPPFLAGS = $(TARGET_CPPFLAGS) $(CPPFLAGS_DEFAULT)
AM_CCASFLAGS = $(TARGET_CCASFLAGS) $(CCASFLAGS_DEFAULT)

CFLAGS_PROGRAM += $(CFLAGS_PLATFORM)
LDFLAGS_PROGRAM += $(LDFLAGS_PLATFORM)
CPPFLAGS_PROGRAM += $(CPPFLAGS_PLATFORM)
CCASFLAGS_PROGRAM += $(CCASFLAGS_PLATFORM)

CFLAGS_LIBRARY += $(CFLAGS_PLATFORM) -fno-builtin
CPPFLAGS_LIBRARY += $(CPPFLAGS_PLATFORM)
CCASFLAGS_LIBRARY += $(CCASFLAGS_PLATFORM)

# gentrigtables
gentrigtables: gentrigtables.c
	$(BUILD_CC) -o $@ -I$(top_srcdir)/include $(CPPFLAGS) $< -lm
CLEANFILES += gentrigtables

# trigtables.c
trigtables.c: gentrigtables gentrigtables.c $(top_srcdir)/configure.ac
	$(builddir)/gentrigtables > $@
CLEANFILES += trigtables.c

gensm712: video/sm712.c
	$(BUILD_CC) -DGENINIT -o $@ -I$(top_builddir) -I$(top_builddir)/include -I$(top_srcdir)/include $(CPPFLAGS) $<
CLEANFILES += gensm712

# trigtables.c
sm712_start.S: gensm712 video/sm712.c $(top_srcdir)/configure.ac
	$(builddir)/gensm712 > $@
CLEANFILES += sm712_start.S

# XXX Use Automake's LEX & YACC support
grub_script.tab.h: script/parser.y
	$(YACC) -d -p grub_script_yy -b grub_script $<
grub_script.tab.c: grub_script.tab.h
CLEANFILES += grub_script.tab.c grub_script.tab.h

# For the lexer.
grub_script.yy.h: script/yylex.l
	$(LEX) -o grub_script.yy.c --header-file=grub_script.yy.h $<
grub_script.yy.c: grub_script.yy.h

rs_decoder.S: $(srcdir)/lib/reed_solomon.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) -Os -I$(top_builddir) -S -DSTANDALONE -o $@ $< -g0 -mregparm=3 -ffreestanding

kern/i386/pc/startup.S: $(builddir)/rs_decoder.S
boot/mips/loongson/fwstart.S: $(builddir)/sm712_start.S

CLEANFILES += grub_script.yy.c grub_script.yy.h

include $(srcdir)/Makefile.core.am

KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/cache.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/command.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/device.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/dl.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/env.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/env_private.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/err.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/file.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/fs.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i18n.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/kernel.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/list.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/misc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/mm.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/parser.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/partition.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/term.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/time.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/mm_private.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/net.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/libgcc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/memory.h

if COND_i386_pc
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/kernel.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/pxe.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/int.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_i386_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/pci.h
endif

if COND_i386_coreboot
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/i386/pc/int.h
endif

if COND_i386_multiboot
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/i386/pc/int.h
endif

if COND_i386_qemu
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/pci.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_i386_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/lib/arg.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
endif

if COND_x86_64_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/i386/pit.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/pci.h
endif

if COND_ia64_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/efi/disk.h
endif

if COND_mips
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/cpu/kernel.h
endif

if COND_mips_arc
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/arc/arc.h
endif

if COND_mips_qemu_mips
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/keyboard_layouts.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/kernel.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/cpu/cache.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/bitmap.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/video.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/gfxterm.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/font.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/bitmap_scale.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/bufio.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/serial.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/lib/arg.h
endif

if COND_mips_loongson
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/keyboard_layouts.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/kernel.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/time.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/cpu/cache.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/bitmap.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/video.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/gfxterm.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/font.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/bitmap_scale.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/bufio.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/pci.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/cs5536.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/pci.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/serial.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/lib/arg.h
endif

if COND_mips_qemu_mips
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/memory.h
KERNEL_HEADER_FILES += $(top_builddir)/include/grub/machine/kernel.h
endif

if COND_powerpc_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/lib/arg.h
endif

if COND_sparc64_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/sparc64/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/lib/arg.h
endif

if COND_emu
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/datetime.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/emu/misc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/emu/export.h
if COND_GRUB_EMU_SDL
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/sdl.h
endif
if COND_GRUB_EMU_USB
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/libusb.h
endif
if COND_GRUB_EMU_PCI
KERNEL_HEADER_FILES += $(top_srcdir)/include/grub/libpciaccess.h
endif
endif

symlist.h: $(top_builddir)/config.h $(KERNEL_HEADER_FILES)
	@list='$^'; \
	for p in $$list; do \
	  echo "#include <$$p>" >> $@ || (rm -f $@; exit 1); \
	done
CLEANFILES += symlist.h
BUILT_SOURCES += symlist.h

symlist.c: symlist.h gensymlist.sh
	$(TARGET_CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS_KERNEL) $(CPPFLAGS) -DGRUB_SYMBOL_GENERATOR=1 symlist.h > symlist.p || (rm -f symlist.p; exit 1)
	cat symlist.p | /bin/sh $(srcdir)/gensymlist.sh $(top_builddir)/config.h $(KERNEL_HEADER_FILES) >$@ || (rm -f $@; exit 1)
	rm -f symlist.p
CLEANFILES += symlist.c
BUILT_SOURCES += symlist.c

if COND_HAVE_ASM_USCORE
ASM_PREFIX=_
else
ASM_PREFIX=
endif

noinst_DATA += kernel_syms.lst

kernel_syms.lst: $(KERNEL_HEADER_FILES) $(top_builddir)/config.h
	$(TARGET_CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS_KERNEL) $(CPPFLAGS) $(CFLAGS) -DGRUB_SYMBOL_GENERATOR=1 $^ >kernel_syms.input
	cat kernel_syms.input | grep -v '^#' | sed -n \
	  -e '/EXPORT_FUNC *([a-zA-Z0-9_]*)/{s/.*EXPORT_FUNC *(\([a-zA-Z0-9_]*\)).*/defined kernel '"$(ASM_PREFIX)"'\1/;p;}' \
	  -e '/EXPORT_VAR *([a-zA-Z0-9_]*)/{s/.*EXPORT_VAR *(\([a-zA-Z0-9_]*\)).*/defined kernel '"$(ASM_PREFIX)"'\1/;p;}' \
	  | sort -u >$@
	rm -f kernel_syms.input
CLEANFILES += kernel_syms.lst

if COND_emu
kern/emu/grub_emu-main.$(OBJEXT):grub_emu_init.h
grub_emu-grub_emu_init.$(OBJEXT):grub_emu_init.h
kern/emu/grub_emu_dyn-main.$(OBJEXT):grub_emu_init.h
grub_emu_dyn-grub_emu_init.$(OBJEXT):grub_emu_init.h

grub_emu_init.h: genemuinitheader.sh $(MOD_FILES)
	rm -f $@; echo $(MOD_FILES) | sh $(srcdir)/genemuinitheader.sh $(NM) > $@
CLEANFILES += grub_emu_init.h

grub_emu_init.c: grub_emu_init.h genemuinit.sh $(MOD_FILES)
	rm -f $@; echo $(MOD_FILES) | sh $(srcdir)/genemuinit.sh $(NM) > $@
CLEANFILES += grub_emu_init.c
endif

# List files

fs.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  if grep 'FS_LIST_MARKER' $$pp >/dev/null 2>&1; then \
	    echo $$b; \
	  fi; \
	done) | sort -u > $@
platform_DATA += fs.lst
CLEANFILES += fs.lst

command.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  sed -n \
	    -e "/EXTCOMMAND_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/*\1: $$b/;p;}" \
	    -e "/P1COMMAND_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/*\1: $$b/;p;}" \
	    -e "/COMMAND_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/\1: $$b/;p;}" $$pp; \
	done) | sort -u > $@
platform_DATA += command.lst
CLEANFILES += command.lst

partmap.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  if grep 'PARTMAP_LIST_MARKER' $$pp >/dev/null 2>&1; then \
	    echo $$b; \
	  fi; \
	done) | sort -u > $@
platform_DATA += partmap.lst
CLEANFILES += partmap.lst

terminal.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  sed -n \
	    -e "/INPUT_TERMINAL_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/i\1: $$b/;p;}" \
	    -e "/OUTPUT_TERMINAL_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/o\1: $$b/;p;}" $$pp; \
	done) | sort -u > $@
platform_DATA += terminal.lst
CLEANFILES += terminal.lst

parttool.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  sed -n \
	    -e "/PARTTOOL_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/\1: $$b/;p;}" $$pp; \
	done) | sort -u > $@
platform_DATA += parttool.lst
CLEANFILES += parttool.lst

video.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  if grep 'VIDEO_LIST_MARKER' $$pp >/dev/null 2>&1; then \
	    echo $$b; \
	  fi; \
	done) | sort -u > $@
platform_DATA += video.lst
CLEANFILES += video.lst

# but, crypto.lst is simply copied
crypto.lst: $(srcdir)/lib/libgcrypt-grub/cipher/crypto.lst
	cp $^ $@
platform_DATA += crypto.lst
CLEANFILES += crypto.lst

syminfo.lst: gensyminfo.sh kernel_syms.lst $(MODULE_FILES)
	cat kernel_syms.lst > $@.new
	for m in $(MODULE_FILES); do \
	  sh $< $$m >> $@.new || exit 1; \
	done
	mv $@.new $@

# generate global module dependencies list
moddep.lst: syminfo.lst genmoddep.awk video.lst
	cat $< | sort | awk -f $(srcdir)/genmoddep.awk > $@ || (rm -f $@; exit 1)
platform_DATA += moddep.lst
CLEANFILES += config.log syminfo.lst moddep.lst

$(MOD_FILES): %.mod : genmod.sh moddep.lst %.module$(EXEEXT)
	TARGET_OBJ2ELF=@TARGET_OBJ2ELF@ sh $^ $@
platform_DATA += $(MOD_FILES)
platform_DATA += modinfo.sh
CLEANFILES += $(MOD_FILES)

if COND_ENABLE_EFIEMU
efiemu32.o: efiemu/runtime/efiemu.c $(TARGET_OBJ2ELF)
	-rm -f $@; \
	if test "x$(TARGET_APPLE_CC)" = x1; then \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF32 -DAPPLE_CC -m32 -Wall -Werror -nostdlib -O2 -c -o $@.bin $< || exit 1; \
	  $(OBJCONV) -felf32 -nu -nd $@.bin $@ || exit 1; \
	  rm -f $@.bin; \
	else \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF32 -m32 -Wall -Werror -nostdlib -O2 -c -o $@ $< || exit 1; \
	  if test ! -z "$(TARGET_OBJ2ELF)"; then $(TARGET_OBJ2ELF) $@ || (rm -f $@; exit 1); fi; \
	fi

efiemu64_c.o: efiemu/runtime/efiemu.c
	if test "x$(TARGET_APPLE_CC)" = x1; then \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF64 -DAPPLE_CC=1 -m64 -nostdlib -Wall -Werror -mno-red-zone -c -o $@ $< || exit 1; \
	else \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF64 -m64 -nostdlib -Wall -Werror -O2 -mcmodel=large -mno-red-zone -c -o $@ $< || exit 1; \
	fi

efiemu64_s.o: efiemu/runtime/efiemu.S
	-rm -f $@
	if test "x$(TARGET_APPLE_CC)" = x1; then \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF64 -DAPPLE_CC=1 -m64 -Wall -Werror -nostdlib -O2 -mno-red-zone -c -o $@ $< || exit 1; \
	else \
	  $(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -DELF64 -m64 -Wall -Werror -nostdlib -O2 -mcmodel=large -mno-red-zone -c -o $@ $< || exit 1; \
	fi

efiemu64.o: efiemu64_c.o efiemu64_s.o $(TARGET_OBJ2ELEF)
	-rm -f $@; \
	if test "x$(TARGET_APPLE_CC)" = x1; then \
	  rm -f $@.bin; \
	  $(TARGET_CC) -m64 -Wl,-r -nostdlib -o $@.bin $^ || exit 1; \
	  $(OBJCONV) -felf64 -nu -nd $@.bin $@ || exit 1; \
	  rm -f $@.bin; \
	else \
	  $(TARGET_CC) -m64 -nostdlib -Wl,-r -o $@ $^ || exit 1; \
	  if test ! -z "$(TARGET_OBJ2ELF)"; then $(TARGET_OBJ2ELF) $@ || (rm -f $@; exit 1); fi; \
	fi

platform_DATA += efiemu32.o efiemu64.o
CLEANFILES += efiemu32.o efiemu64.o efiemu64_c.o efiemu64_s.o
endif
