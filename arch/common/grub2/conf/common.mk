# -*- makefile -*-

# For the parser.
grub_script.tab.c grub_script.tab.h: normal/parser.y
	$(YACC) -d -p grub_script_yy -b grub_script $(srcdir)/normal/parser.y
DISTCLEANFILES += grub_script.tab.c grub_script.tab.h

# For grub-emu.
grub_emu_init.lst: geninit.sh $(filter-out grub_emu_init.c,$(grub_emu_SOURCES))
	rm -f $@; grep GRUB_MOD_INIT $(filter %.c,$^) /dev/null > $@
DISTCLEANFILES += grub_emu_init.lst

grub_emu_init.h: grub_emu_init.lst $(filter-out grub_emu_init.c,$(grub_emu_SOURCES)) geninitheader.sh
	rm -f $@; sh $(srcdir)/geninitheader.sh $< > $@
DISTCLEANFILES += grub_emu_init.h

grub_emu_init.c: grub_emu_init.lst $(filter-out grub_emu_init.c,$(grub_emu_SOURCES)) geninit.sh grub_emu_init.h
	rm -f $@; sh $(srcdir)/geninit.sh $< $(filter %.c,$^) > $@
DISTCLEANFILES += grub_emu_init.c

# For grub-probe.
grub_probe_init.lst: geninit.sh $(filter-out grub_probe_init.c,$(grub_probe_SOURCES))
	rm -f $@; grep GRUB_MOD_INIT $(filter %.c,$^) /dev/null > $@
DISTCLEANFILES += grub_probe_init.lst

grub_probe_init.h: grub_probe_init.lst $(filter-out grub_probe_init.c,$(grub_probe_SOURCES)) geninitheader.sh
	rm -f $@; sh $(srcdir)/geninitheader.sh $< > $@
DISTCLEANFILES += grub_probe_init.h

grub_probe_init.c: grub_probe_init.lst $(filter-out grub_probe_init.c,$(grub_probe_SOURCES)) geninit.sh grub_probe_init.h
	rm -f $@; sh $(srcdir)/geninit.sh $< $(filter %.c,$^) > $@
DISTCLEANFILES += grub_probe_init.c

# For grub-setup.
grub_setup_init.lst: geninit.sh $(filter-out grub_setup_init.c,$(grub_setup_SOURCES))
	rm -f $@; grep GRUB_MOD_INIT $(filter %.c,$^) /dev/null > $@
DISTCLEANFILES += grub_setup_init.lst

grub_setup_init.h: grub_setup_init.lst $(filter-out grub_setup_init.c,$(grub_setup_SOURCES)) geninitheader.sh
	rm -f $@; sh $(srcdir)/geninitheader.sh $< > $@
DISTCLEANFILES += grub_setup_init.h

grub_setup_init.c: grub_setup_init.lst $(filter-out grub_setup_init.c,$(grub_setup_SOURCES)) geninit.sh grub_setup_init.h
	rm -f $@; sh $(srcdir)/geninit.sh $< $(filter %.c,$^) > $@
DISTCLEANFILES += grub_setup_init.c

# For update-grub
update-grub: util/update-grub.in config.status
	./config.status --file=$@:$<
	chmod +x $@
sbin_SCRIPTS += update-grub
CLEANFILES += update-grub

update-grub_lib: util/update-grub_lib.in config.status
	./config.status --file=$@:$<
	chmod +x $@
lib_DATA += update-grub_lib
CLEANFILES += update-grub_lib

00_header: util/grub.d/00_header.in config.status
	./config.status --file=$@:$<
	chmod +x $@
update-grub_SCRIPTS += 00_header
CLEANFILES += 00_header

10_linux: util/grub.d/10_linux.in config.status
	./config.status --file=$@:$<
	chmod +x $@
update-grub_SCRIPTS += 10_linux
CLEANFILES += 10_linux

10_hurd: util/grub.d/10_hurd.in config.status
	./config.status --file=$@:$<
	chmod +x $@
update-grub_SCRIPTS += 10_hurd
CLEANFILES += 10_hurd

update-grub_DATA += util/grub.d/README


# Filing systems.
pkglib_MODULES += fshelp.mod fat.mod ufs.mod ext2.mod ntfs.mod		\
	ntfscomp.mod minix.mod hfs.mod jfs.mod iso9660.mod xfs.mod	\
	affs.mod sfs.mod hfsplus.mod reiserfs.mod cpio.mod

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

fshelp_mod-fs_fshelp.o: fs/fshelp.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(fshelp_mod_CFLAGS) -MD -c -o $@ $<
-include fshelp_mod-fs_fshelp.d

CLEANFILES += cmd-fshelp_mod-fs_fshelp.lst fs-fshelp_mod-fs_fshelp.lst
COMMANDFILES += cmd-fshelp_mod-fs_fshelp.lst
FSFILES += fs-fshelp_mod-fs_fshelp.lst

cmd-fshelp_mod-fs_fshelp.lst: fs/fshelp.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(fshelp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh fshelp > $@ || (rm -f $@; exit 1)

fs-fshelp_mod-fs_fshelp.lst: fs/fshelp.c genfslist.sh
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

fat_mod-fs_fat.o: fs/fat.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(fat_mod_CFLAGS) -MD -c -o $@ $<
-include fat_mod-fs_fat.d

CLEANFILES += cmd-fat_mod-fs_fat.lst fs-fat_mod-fs_fat.lst
COMMANDFILES += cmd-fat_mod-fs_fat.lst
FSFILES += fs-fat_mod-fs_fat.lst

cmd-fat_mod-fs_fat.lst: fs/fat.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(fat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh fat > $@ || (rm -f $@; exit 1)

fs-fat_mod-fs_fat.lst: fs/fat.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(fat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh fat > $@ || (rm -f $@; exit 1)


fat_mod_CFLAGS = $(COMMON_CFLAGS)
fat_mod_LDFLAGS = $(COMMON_LDFLAGS)

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

ufs_mod-fs_ufs.o: fs/ufs.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(ufs_mod_CFLAGS) -MD -c -o $@ $<
-include ufs_mod-fs_ufs.d

CLEANFILES += cmd-ufs_mod-fs_ufs.lst fs-ufs_mod-fs_ufs.lst
COMMANDFILES += cmd-ufs_mod-fs_ufs.lst
FSFILES += fs-ufs_mod-fs_ufs.lst

cmd-ufs_mod-fs_ufs.lst: fs/ufs.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ufs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ufs > $@ || (rm -f $@; exit 1)

fs-ufs_mod-fs_ufs.lst: fs/ufs.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ufs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ufs > $@ || (rm -f $@; exit 1)


ufs_mod_CFLAGS = $(COMMON_CFLAGS)
ufs_mod_LDFLAGS = $(COMMON_LDFLAGS)

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

ext2_mod-fs_ext2.o: fs/ext2.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(ext2_mod_CFLAGS) -MD -c -o $@ $<
-include ext2_mod-fs_ext2.d

CLEANFILES += cmd-ext2_mod-fs_ext2.lst fs-ext2_mod-fs_ext2.lst
COMMANDFILES += cmd-ext2_mod-fs_ext2.lst
FSFILES += fs-ext2_mod-fs_ext2.lst

cmd-ext2_mod-fs_ext2.lst: fs/ext2.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ext2_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ext2 > $@ || (rm -f $@; exit 1)

fs-ext2_mod-fs_ext2.lst: fs/ext2.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ext2_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ext2 > $@ || (rm -f $@; exit 1)


ext2_mod_CFLAGS = $(COMMON_CFLAGS)
ext2_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For ntfs.mod.
ntfs_mod_SOURCES = fs/ntfs.c
CLEANFILES += ntfs.mod mod-ntfs.o mod-ntfs.c pre-ntfs.o ntfs_mod-fs_ntfs.o und-ntfs.lst
ifneq ($(ntfs_mod_EXPORTS),no)
CLEANFILES += def-ntfs.lst
DEFSYMFILES += def-ntfs.lst
endif
MOSTLYCLEANFILES += ntfs_mod-fs_ntfs.d
UNDSYMFILES += und-ntfs.lst

ntfs.mod: pre-ntfs.o mod-ntfs.o
	-rm -f $@
	$(TARGET_CC) $(ntfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ntfs.o: $(ntfs_mod_DEPENDENCIES) ntfs_mod-fs_ntfs.o
	-rm -f $@
	$(TARGET_CC) $(ntfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ ntfs_mod-fs_ntfs.o

mod-ntfs.o: mod-ntfs.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ntfs_mod_CFLAGS) -c -o $@ $<

mod-ntfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'ntfs' $< > $@ || (rm -f $@; exit 1)

ifneq ($(ntfs_mod_EXPORTS),no)
def-ntfs.lst: pre-ntfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 ntfs/' > $@
endif

und-ntfs.lst: pre-ntfs.o
	echo 'ntfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

ntfs_mod-fs_ntfs.o: fs/ntfs.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(ntfs_mod_CFLAGS) -MD -c -o $@ $<
-include ntfs_mod-fs_ntfs.d

CLEANFILES += cmd-ntfs_mod-fs_ntfs.lst fs-ntfs_mod-fs_ntfs.lst
COMMANDFILES += cmd-ntfs_mod-fs_ntfs.lst
FSFILES += fs-ntfs_mod-fs_ntfs.lst

cmd-ntfs_mod-fs_ntfs.lst: fs/ntfs.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ntfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ntfs > $@ || (rm -f $@; exit 1)

fs-ntfs_mod-fs_ntfs.lst: fs/ntfs.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ntfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ntfs > $@ || (rm -f $@; exit 1)


ntfs_mod_CFLAGS = $(COMMON_CFLAGS)
ntfs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For ntfscomp.mod.
ntfscomp_mod_SOURCES = fs/ntfscomp.c
CLEANFILES += ntfscomp.mod mod-ntfscomp.o mod-ntfscomp.c pre-ntfscomp.o ntfscomp_mod-fs_ntfscomp.o und-ntfscomp.lst
ifneq ($(ntfscomp_mod_EXPORTS),no)
CLEANFILES += def-ntfscomp.lst
DEFSYMFILES += def-ntfscomp.lst
endif
MOSTLYCLEANFILES += ntfscomp_mod-fs_ntfscomp.d
UNDSYMFILES += und-ntfscomp.lst

ntfscomp.mod: pre-ntfscomp.o mod-ntfscomp.o
	-rm -f $@
	$(TARGET_CC) $(ntfscomp_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-ntfscomp.o: $(ntfscomp_mod_DEPENDENCIES) ntfscomp_mod-fs_ntfscomp.o
	-rm -f $@
	$(TARGET_CC) $(ntfscomp_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ ntfscomp_mod-fs_ntfscomp.o

mod-ntfscomp.o: mod-ntfscomp.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ntfscomp_mod_CFLAGS) -c -o $@ $<

mod-ntfscomp.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'ntfscomp' $< > $@ || (rm -f $@; exit 1)

ifneq ($(ntfscomp_mod_EXPORTS),no)
def-ntfscomp.lst: pre-ntfscomp.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 ntfscomp/' > $@
endif

und-ntfscomp.lst: pre-ntfscomp.o
	echo 'ntfscomp' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

ntfscomp_mod-fs_ntfscomp.o: fs/ntfscomp.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(ntfscomp_mod_CFLAGS) -MD -c -o $@ $<
-include ntfscomp_mod-fs_ntfscomp.d

CLEANFILES += cmd-ntfscomp_mod-fs_ntfscomp.lst fs-ntfscomp_mod-fs_ntfscomp.lst
COMMANDFILES += cmd-ntfscomp_mod-fs_ntfscomp.lst
FSFILES += fs-ntfscomp_mod-fs_ntfscomp.lst

cmd-ntfscomp_mod-fs_ntfscomp.lst: fs/ntfscomp.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ntfscomp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ntfscomp > $@ || (rm -f $@; exit 1)

fs-ntfscomp_mod-fs_ntfscomp.lst: fs/ntfscomp.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ntfscomp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh ntfscomp > $@ || (rm -f $@; exit 1)


ntfscomp_mod_CFLAGS = $(COMMON_CFLAGS)
ntfscomp_mod_LDFLAGS = $(COMMON_LDFLAGS)

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

minix_mod-fs_minix.o: fs/minix.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(minix_mod_CFLAGS) -MD -c -o $@ $<
-include minix_mod-fs_minix.d

CLEANFILES += cmd-minix_mod-fs_minix.lst fs-minix_mod-fs_minix.lst
COMMANDFILES += cmd-minix_mod-fs_minix.lst
FSFILES += fs-minix_mod-fs_minix.lst

cmd-minix_mod-fs_minix.lst: fs/minix.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(minix_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh minix > $@ || (rm -f $@; exit 1)

fs-minix_mod-fs_minix.lst: fs/minix.c genfslist.sh
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

hfs_mod-fs_hfs.o: fs/hfs.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(hfs_mod_CFLAGS) -MD -c -o $@ $<
-include hfs_mod-fs_hfs.d

CLEANFILES += cmd-hfs_mod-fs_hfs.lst fs-hfs_mod-fs_hfs.lst
COMMANDFILES += cmd-hfs_mod-fs_hfs.lst
FSFILES += fs-hfs_mod-fs_hfs.lst

cmd-hfs_mod-fs_hfs.lst: fs/hfs.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh hfs > $@ || (rm -f $@; exit 1)

fs-hfs_mod-fs_hfs.lst: fs/hfs.c genfslist.sh
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

jfs_mod-fs_jfs.o: fs/jfs.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(jfs_mod_CFLAGS) -MD -c -o $@ $<
-include jfs_mod-fs_jfs.d

CLEANFILES += cmd-jfs_mod-fs_jfs.lst fs-jfs_mod-fs_jfs.lst
COMMANDFILES += cmd-jfs_mod-fs_jfs.lst
FSFILES += fs-jfs_mod-fs_jfs.lst

cmd-jfs_mod-fs_jfs.lst: fs/jfs.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(jfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh jfs > $@ || (rm -f $@; exit 1)

fs-jfs_mod-fs_jfs.lst: fs/jfs.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(jfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh jfs > $@ || (rm -f $@; exit 1)


jfs_mod_CFLAGS = $(COMMON_CFLAGS)
jfs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For iso9660.mod.
iso9660_mod_SOURCES = fs/iso9660.c
CLEANFILES += iso9660.mod mod-iso9660.o mod-iso9660.c pre-iso9660.o iso9660_mod-fs_iso9660.o und-iso9660.lst
ifneq ($(iso9660_mod_EXPORTS),no)
CLEANFILES += def-iso9660.lst
DEFSYMFILES += def-iso9660.lst
endif
MOSTLYCLEANFILES += iso9660_mod-fs_iso9660.d
UNDSYMFILES += und-iso9660.lst

iso9660.mod: pre-iso9660.o mod-iso9660.o
	-rm -f $@
	$(TARGET_CC) $(iso9660_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-iso9660.o: $(iso9660_mod_DEPENDENCIES) iso9660_mod-fs_iso9660.o
	-rm -f $@
	$(TARGET_CC) $(iso9660_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ iso9660_mod-fs_iso9660.o

mod-iso9660.o: mod-iso9660.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(iso9660_mod_CFLAGS) -c -o $@ $<

mod-iso9660.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'iso9660' $< > $@ || (rm -f $@; exit 1)

ifneq ($(iso9660_mod_EXPORTS),no)
def-iso9660.lst: pre-iso9660.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 iso9660/' > $@
endif

und-iso9660.lst: pre-iso9660.o
	echo 'iso9660' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

iso9660_mod-fs_iso9660.o: fs/iso9660.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(iso9660_mod_CFLAGS) -MD -c -o $@ $<
-include iso9660_mod-fs_iso9660.d

CLEANFILES += cmd-iso9660_mod-fs_iso9660.lst fs-iso9660_mod-fs_iso9660.lst
COMMANDFILES += cmd-iso9660_mod-fs_iso9660.lst
FSFILES += fs-iso9660_mod-fs_iso9660.lst

cmd-iso9660_mod-fs_iso9660.lst: fs/iso9660.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(iso9660_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh iso9660 > $@ || (rm -f $@; exit 1)

fs-iso9660_mod-fs_iso9660.lst: fs/iso9660.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(iso9660_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh iso9660 > $@ || (rm -f $@; exit 1)


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

xfs_mod-fs_xfs.o: fs/xfs.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(xfs_mod_CFLAGS) -MD -c -o $@ $<
-include xfs_mod-fs_xfs.d

CLEANFILES += cmd-xfs_mod-fs_xfs.lst fs-xfs_mod-fs_xfs.lst
COMMANDFILES += cmd-xfs_mod-fs_xfs.lst
FSFILES += fs-xfs_mod-fs_xfs.lst

cmd-xfs_mod-fs_xfs.lst: fs/xfs.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(xfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh xfs > $@ || (rm -f $@; exit 1)

fs-xfs_mod-fs_xfs.lst: fs/xfs.c genfslist.sh
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

affs_mod-fs_affs.o: fs/affs.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(affs_mod_CFLAGS) -MD -c -o $@ $<
-include affs_mod-fs_affs.d

CLEANFILES += cmd-affs_mod-fs_affs.lst fs-affs_mod-fs_affs.lst
COMMANDFILES += cmd-affs_mod-fs_affs.lst
FSFILES += fs-affs_mod-fs_affs.lst

cmd-affs_mod-fs_affs.lst: fs/affs.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(affs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh affs > $@ || (rm -f $@; exit 1)

fs-affs_mod-fs_affs.lst: fs/affs.c genfslist.sh
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

sfs_mod-fs_sfs.o: fs/sfs.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(sfs_mod_CFLAGS) -MD -c -o $@ $<
-include sfs_mod-fs_sfs.d

CLEANFILES += cmd-sfs_mod-fs_sfs.lst fs-sfs_mod-fs_sfs.lst
COMMANDFILES += cmd-sfs_mod-fs_sfs.lst
FSFILES += fs-sfs_mod-fs_sfs.lst

cmd-sfs_mod-fs_sfs.lst: fs/sfs.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(sfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh sfs > $@ || (rm -f $@; exit 1)

fs-sfs_mod-fs_sfs.lst: fs/sfs.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(sfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh sfs > $@ || (rm -f $@; exit 1)


sfs_mod_CFLAGS = $(COMMON_CFLAGS)
sfs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For hfsplus.mod.
hfsplus_mod_SOURCES = fs/hfsplus.c
CLEANFILES += hfsplus.mod mod-hfsplus.o mod-hfsplus.c pre-hfsplus.o hfsplus_mod-fs_hfsplus.o und-hfsplus.lst
ifneq ($(hfsplus_mod_EXPORTS),no)
CLEANFILES += def-hfsplus.lst
DEFSYMFILES += def-hfsplus.lst
endif
MOSTLYCLEANFILES += hfsplus_mod-fs_hfsplus.d
UNDSYMFILES += und-hfsplus.lst

hfsplus.mod: pre-hfsplus.o mod-hfsplus.o
	-rm -f $@
	$(TARGET_CC) $(hfsplus_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-hfsplus.o: $(hfsplus_mod_DEPENDENCIES) hfsplus_mod-fs_hfsplus.o
	-rm -f $@
	$(TARGET_CC) $(hfsplus_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ hfsplus_mod-fs_hfsplus.o

mod-hfsplus.o: mod-hfsplus.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hfsplus_mod_CFLAGS) -c -o $@ $<

mod-hfsplus.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'hfsplus' $< > $@ || (rm -f $@; exit 1)

ifneq ($(hfsplus_mod_EXPORTS),no)
def-hfsplus.lst: pre-hfsplus.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 hfsplus/' > $@
endif

und-hfsplus.lst: pre-hfsplus.o
	echo 'hfsplus' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

hfsplus_mod-fs_hfsplus.o: fs/hfsplus.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(hfsplus_mod_CFLAGS) -MD -c -o $@ $<
-include hfsplus_mod-fs_hfsplus.d

CLEANFILES += cmd-hfsplus_mod-fs_hfsplus.lst fs-hfsplus_mod-fs_hfsplus.lst
COMMANDFILES += cmd-hfsplus_mod-fs_hfsplus.lst
FSFILES += fs-hfsplus_mod-fs_hfsplus.lst

cmd-hfsplus_mod-fs_hfsplus.lst: fs/hfsplus.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hfsplus_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh hfsplus > $@ || (rm -f $@; exit 1)

fs-hfsplus_mod-fs_hfsplus.lst: fs/hfsplus.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hfsplus_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh hfsplus > $@ || (rm -f $@; exit 1)


hfsplus_mod_CFLAGS = $(COMMON_CFLAGS)
hfsplus_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For reiserfs.mod.
reiserfs_mod_SOURCES = fs/reiserfs.c
CLEANFILES += reiserfs.mod mod-reiserfs.o mod-reiserfs.c pre-reiserfs.o reiserfs_mod-fs_reiserfs.o und-reiserfs.lst
ifneq ($(reiserfs_mod_EXPORTS),no)
CLEANFILES += def-reiserfs.lst
DEFSYMFILES += def-reiserfs.lst
endif
MOSTLYCLEANFILES += reiserfs_mod-fs_reiserfs.d
UNDSYMFILES += und-reiserfs.lst

reiserfs.mod: pre-reiserfs.o mod-reiserfs.o
	-rm -f $@
	$(TARGET_CC) $(reiserfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-reiserfs.o: $(reiserfs_mod_DEPENDENCIES) reiserfs_mod-fs_reiserfs.o
	-rm -f $@
	$(TARGET_CC) $(reiserfs_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ reiserfs_mod-fs_reiserfs.o

mod-reiserfs.o: mod-reiserfs.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(reiserfs_mod_CFLAGS) -c -o $@ $<

mod-reiserfs.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'reiserfs' $< > $@ || (rm -f $@; exit 1)

ifneq ($(reiserfs_mod_EXPORTS),no)
def-reiserfs.lst: pre-reiserfs.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 reiserfs/' > $@
endif

und-reiserfs.lst: pre-reiserfs.o
	echo 'reiserfs' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

reiserfs_mod-fs_reiserfs.o: fs/reiserfs.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(reiserfs_mod_CFLAGS) -MD -c -o $@ $<
-include reiserfs_mod-fs_reiserfs.d

CLEANFILES += cmd-reiserfs_mod-fs_reiserfs.lst fs-reiserfs_mod-fs_reiserfs.lst
COMMANDFILES += cmd-reiserfs_mod-fs_reiserfs.lst
FSFILES += fs-reiserfs_mod-fs_reiserfs.lst

cmd-reiserfs_mod-fs_reiserfs.lst: fs/reiserfs.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(reiserfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh reiserfs > $@ || (rm -f $@; exit 1)

fs-reiserfs_mod-fs_reiserfs.lst: fs/reiserfs.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(reiserfs_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh reiserfs > $@ || (rm -f $@; exit 1)


reiserfs_mod_CFLAGS = $(COMMON_CFLAGS)
reiserfs_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For cpio.mod.
cpio_mod_SOURCES = fs/cpio.c
CLEANFILES += cpio.mod mod-cpio.o mod-cpio.c pre-cpio.o cpio_mod-fs_cpio.o und-cpio.lst
ifneq ($(cpio_mod_EXPORTS),no)
CLEANFILES += def-cpio.lst
DEFSYMFILES += def-cpio.lst
endif
MOSTLYCLEANFILES += cpio_mod-fs_cpio.d
UNDSYMFILES += und-cpio.lst

cpio.mod: pre-cpio.o mod-cpio.o
	-rm -f $@
	$(TARGET_CC) $(cpio_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-cpio.o: $(cpio_mod_DEPENDENCIES) cpio_mod-fs_cpio.o
	-rm -f $@
	$(TARGET_CC) $(cpio_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ cpio_mod-fs_cpio.o

mod-cpio.o: mod-cpio.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cpio_mod_CFLAGS) -c -o $@ $<

mod-cpio.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'cpio' $< > $@ || (rm -f $@; exit 1)

ifneq ($(cpio_mod_EXPORTS),no)
def-cpio.lst: pre-cpio.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 cpio/' > $@
endif

und-cpio.lst: pre-cpio.o
	echo 'cpio' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

cpio_mod-fs_cpio.o: fs/cpio.c
	$(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(cpio_mod_CFLAGS) -MD -c -o $@ $<
-include cpio_mod-fs_cpio.d

CLEANFILES += cmd-cpio_mod-fs_cpio.lst fs-cpio_mod-fs_cpio.lst
COMMANDFILES += cmd-cpio_mod-fs_cpio.lst
FSFILES += fs-cpio_mod-fs_cpio.lst

cmd-cpio_mod-fs_cpio.lst: fs/cpio.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cpio_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh cpio > $@ || (rm -f $@; exit 1)

fs-cpio_mod-fs_cpio.lst: fs/cpio.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifs -I$(srcdir)/fs $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cpio_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh cpio > $@ || (rm -f $@; exit 1)


cpio_mod_CFLAGS = $(COMMON_CFLAGS)
cpio_mod_LDFLAGS = $(COMMON_LDFLAGS)

# Partition maps.
pkglib_MODULES += amiga.mod apple.mod pc.mod sun.mod acorn.mod gpt.mod

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

amiga_mod-partmap_amiga.o: partmap/amiga.c
	$(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(amiga_mod_CFLAGS) -MD -c -o $@ $<
-include amiga_mod-partmap_amiga.d

CLEANFILES += cmd-amiga_mod-partmap_amiga.lst fs-amiga_mod-partmap_amiga.lst
COMMANDFILES += cmd-amiga_mod-partmap_amiga.lst
FSFILES += fs-amiga_mod-partmap_amiga.lst

cmd-amiga_mod-partmap_amiga.lst: partmap/amiga.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(amiga_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh amiga > $@ || (rm -f $@; exit 1)

fs-amiga_mod-partmap_amiga.lst: partmap/amiga.c genfslist.sh
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

apple_mod-partmap_apple.o: partmap/apple.c
	$(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(apple_mod_CFLAGS) -MD -c -o $@ $<
-include apple_mod-partmap_apple.d

CLEANFILES += cmd-apple_mod-partmap_apple.lst fs-apple_mod-partmap_apple.lst
COMMANDFILES += cmd-apple_mod-partmap_apple.lst
FSFILES += fs-apple_mod-partmap_apple.lst

cmd-apple_mod-partmap_apple.lst: partmap/apple.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(apple_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh apple > $@ || (rm -f $@; exit 1)

fs-apple_mod-partmap_apple.lst: partmap/apple.c genfslist.sh
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

pc_mod-partmap_pc.o: partmap/pc.c
	$(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(pc_mod_CFLAGS) -MD -c -o $@ $<
-include pc_mod-partmap_pc.d

CLEANFILES += cmd-pc_mod-partmap_pc.lst fs-pc_mod-partmap_pc.lst
COMMANDFILES += cmd-pc_mod-partmap_pc.lst
FSFILES += fs-pc_mod-partmap_pc.lst

cmd-pc_mod-partmap_pc.lst: partmap/pc.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(pc_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh pc > $@ || (rm -f $@; exit 1)

fs-pc_mod-partmap_pc.lst: partmap/pc.c genfslist.sh
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

sun_mod-partmap_sun.o: partmap/sun.c
	$(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(sun_mod_CFLAGS) -MD -c -o $@ $<
-include sun_mod-partmap_sun.d

CLEANFILES += cmd-sun_mod-partmap_sun.lst fs-sun_mod-partmap_sun.lst
COMMANDFILES += cmd-sun_mod-partmap_sun.lst
FSFILES += fs-sun_mod-partmap_sun.lst

cmd-sun_mod-partmap_sun.lst: partmap/sun.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(sun_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh sun > $@ || (rm -f $@; exit 1)

fs-sun_mod-partmap_sun.lst: partmap/sun.c genfslist.sh
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

acorn_mod-partmap_acorn.o: partmap/acorn.c
	$(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(acorn_mod_CFLAGS) -MD -c -o $@ $<
-include acorn_mod-partmap_acorn.d

CLEANFILES += cmd-acorn_mod-partmap_acorn.lst fs-acorn_mod-partmap_acorn.lst
COMMANDFILES += cmd-acorn_mod-partmap_acorn.lst
FSFILES += fs-acorn_mod-partmap_acorn.lst

cmd-acorn_mod-partmap_acorn.lst: partmap/acorn.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(acorn_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh acorn > $@ || (rm -f $@; exit 1)

fs-acorn_mod-partmap_acorn.lst: partmap/acorn.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(acorn_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh acorn > $@ || (rm -f $@; exit 1)


acorn_mod_CFLAGS = $(COMMON_CFLAGS)
acorn_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For gpt.mod
gpt_mod_SOURCES = partmap/gpt.c
CLEANFILES += gpt.mod mod-gpt.o mod-gpt.c pre-gpt.o gpt_mod-partmap_gpt.o und-gpt.lst
ifneq ($(gpt_mod_EXPORTS),no)
CLEANFILES += def-gpt.lst
DEFSYMFILES += def-gpt.lst
endif
MOSTLYCLEANFILES += gpt_mod-partmap_gpt.d
UNDSYMFILES += und-gpt.lst

gpt.mod: pre-gpt.o mod-gpt.o
	-rm -f $@
	$(TARGET_CC) $(gpt_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-gpt.o: $(gpt_mod_DEPENDENCIES) gpt_mod-partmap_gpt.o
	-rm -f $@
	$(TARGET_CC) $(gpt_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ gpt_mod-partmap_gpt.o

mod-gpt.o: mod-gpt.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gpt_mod_CFLAGS) -c -o $@ $<

mod-gpt.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'gpt' $< > $@ || (rm -f $@; exit 1)

ifneq ($(gpt_mod_EXPORTS),no)
def-gpt.lst: pre-gpt.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 gpt/' > $@
endif

und-gpt.lst: pre-gpt.o
	echo 'gpt' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

gpt_mod-partmap_gpt.o: partmap/gpt.c
	$(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(gpt_mod_CFLAGS) -MD -c -o $@ $<
-include gpt_mod-partmap_gpt.d

CLEANFILES += cmd-gpt_mod-partmap_gpt.lst fs-gpt_mod-partmap_gpt.lst
COMMANDFILES += cmd-gpt_mod-partmap_gpt.lst
FSFILES += fs-gpt_mod-partmap_gpt.lst

cmd-gpt_mod-partmap_gpt.lst: partmap/gpt.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gpt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh gpt > $@ || (rm -f $@; exit 1)

fs-gpt_mod-partmap_gpt.lst: partmap/gpt.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ipartmap -I$(srcdir)/partmap $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gpt_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh gpt > $@ || (rm -f $@; exit 1)


gpt_mod_CFLAGS = $(COMMON_CFLAGS)
gpt_mod_LDFLAGS = $(COMMON_LDFLAGS)

# Special disk structures

pkglib_MODULES += raid.mod lvm.mod

# For raid.mod
raid_mod_SOURCES = disk/raid.c
CLEANFILES += raid.mod mod-raid.o mod-raid.c pre-raid.o raid_mod-disk_raid.o und-raid.lst
ifneq ($(raid_mod_EXPORTS),no)
CLEANFILES += def-raid.lst
DEFSYMFILES += def-raid.lst
endif
MOSTLYCLEANFILES += raid_mod-disk_raid.d
UNDSYMFILES += und-raid.lst

raid.mod: pre-raid.o mod-raid.o
	-rm -f $@
	$(TARGET_CC) $(raid_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-raid.o: $(raid_mod_DEPENDENCIES) raid_mod-disk_raid.o
	-rm -f $@
	$(TARGET_CC) $(raid_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ raid_mod-disk_raid.o

mod-raid.o: mod-raid.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(raid_mod_CFLAGS) -c -o $@ $<

mod-raid.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'raid' $< > $@ || (rm -f $@; exit 1)

ifneq ($(raid_mod_EXPORTS),no)
def-raid.lst: pre-raid.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 raid/' > $@
endif

und-raid.lst: pre-raid.o
	echo 'raid' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

raid_mod-disk_raid.o: disk/raid.c
	$(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(raid_mod_CFLAGS) -MD -c -o $@ $<
-include raid_mod-disk_raid.d

CLEANFILES += cmd-raid_mod-disk_raid.lst fs-raid_mod-disk_raid.lst
COMMANDFILES += cmd-raid_mod-disk_raid.lst
FSFILES += fs-raid_mod-disk_raid.lst

cmd-raid_mod-disk_raid.lst: disk/raid.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(raid_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh raid > $@ || (rm -f $@; exit 1)

fs-raid_mod-disk_raid.lst: disk/raid.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(raid_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh raid > $@ || (rm -f $@; exit 1)


raid_mod_CFLAGS = $(COMMON_CFLAGS)
raid_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For raid.mod
lvm_mod_SOURCES = disk/lvm.c
CLEANFILES += lvm.mod mod-lvm.o mod-lvm.c pre-lvm.o lvm_mod-disk_lvm.o und-lvm.lst
ifneq ($(lvm_mod_EXPORTS),no)
CLEANFILES += def-lvm.lst
DEFSYMFILES += def-lvm.lst
endif
MOSTLYCLEANFILES += lvm_mod-disk_lvm.d
UNDSYMFILES += und-lvm.lst

lvm.mod: pre-lvm.o mod-lvm.o
	-rm -f $@
	$(TARGET_CC) $(lvm_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-lvm.o: $(lvm_mod_DEPENDENCIES) lvm_mod-disk_lvm.o
	-rm -f $@
	$(TARGET_CC) $(lvm_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ lvm_mod-disk_lvm.o

mod-lvm.o: mod-lvm.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(lvm_mod_CFLAGS) -c -o $@ $<

mod-lvm.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'lvm' $< > $@ || (rm -f $@; exit 1)

ifneq ($(lvm_mod_EXPORTS),no)
def-lvm.lst: pre-lvm.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 lvm/' > $@
endif

und-lvm.lst: pre-lvm.o
	echo 'lvm' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

lvm_mod-disk_lvm.o: disk/lvm.c
	$(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(lvm_mod_CFLAGS) -MD -c -o $@ $<
-include lvm_mod-disk_lvm.d

CLEANFILES += cmd-lvm_mod-disk_lvm.lst fs-lvm_mod-disk_lvm.lst
COMMANDFILES += cmd-lvm_mod-disk_lvm.lst
FSFILES += fs-lvm_mod-disk_lvm.lst

cmd-lvm_mod-disk_lvm.lst: disk/lvm.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(lvm_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh lvm > $@ || (rm -f $@; exit 1)

fs-lvm_mod-disk_lvm.lst: disk/lvm.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(lvm_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh lvm > $@ || (rm -f $@; exit 1)


lvm_mod_CFLAGS = $(COMMON_CFLAGS)
lvm_mod_LDFLAGS = $(COMMON_LDFLAGS)

# Commands.
pkglib_MODULES += hello.mod boot.mod terminal.mod ls.mod	\
	cmp.mod cat.mod help.mod font.mod search.mod		\
	loopback.mod configfile.mod				\
	terminfo.mod test.mod blocklist.mod hexdump.mod

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

hello_mod-hello_hello.o: hello/hello.c
	$(TARGET_CC) -Ihello -I$(srcdir)/hello $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(hello_mod_CFLAGS) -MD -c -o $@ $<
-include hello_mod-hello_hello.d

CLEANFILES += cmd-hello_mod-hello_hello.lst fs-hello_mod-hello_hello.lst
COMMANDFILES += cmd-hello_mod-hello_hello.lst
FSFILES += fs-hello_mod-hello_hello.lst

cmd-hello_mod-hello_hello.lst: hello/hello.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ihello -I$(srcdir)/hello $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hello_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh hello > $@ || (rm -f $@; exit 1)

fs-hello_mod-hello_hello.lst: hello/hello.c genfslist.sh
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

boot_mod-commands_boot.o: commands/boot.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(boot_mod_CFLAGS) -MD -c -o $@ $<
-include boot_mod-commands_boot.d

CLEANFILES += cmd-boot_mod-commands_boot.lst fs-boot_mod-commands_boot.lst
COMMANDFILES += cmd-boot_mod-commands_boot.lst
FSFILES += fs-boot_mod-commands_boot.lst

cmd-boot_mod-commands_boot.lst: commands/boot.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(boot_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh boot > $@ || (rm -f $@; exit 1)

fs-boot_mod-commands_boot.lst: commands/boot.c genfslist.sh
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

terminal_mod-commands_terminal.o: commands/terminal.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(terminal_mod_CFLAGS) -MD -c -o $@ $<
-include terminal_mod-commands_terminal.d

CLEANFILES += cmd-terminal_mod-commands_terminal.lst fs-terminal_mod-commands_terminal.lst
COMMANDFILES += cmd-terminal_mod-commands_terminal.lst
FSFILES += fs-terminal_mod-commands_terminal.lst

cmd-terminal_mod-commands_terminal.lst: commands/terminal.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(terminal_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh terminal > $@ || (rm -f $@; exit 1)

fs-terminal_mod-commands_terminal.lst: commands/terminal.c genfslist.sh
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

ls_mod-commands_ls.o: commands/ls.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(ls_mod_CFLAGS) -MD -c -o $@ $<
-include ls_mod-commands_ls.d

CLEANFILES += cmd-ls_mod-commands_ls.lst fs-ls_mod-commands_ls.lst
COMMANDFILES += cmd-ls_mod-commands_ls.lst
FSFILES += fs-ls_mod-commands_ls.lst

cmd-ls_mod-commands_ls.lst: commands/ls.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(ls_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh ls > $@ || (rm -f $@; exit 1)

fs-ls_mod-commands_ls.lst: commands/ls.c genfslist.sh
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

cmp_mod-commands_cmp.o: commands/cmp.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(cmp_mod_CFLAGS) -MD -c -o $@ $<
-include cmp_mod-commands_cmp.d

CLEANFILES += cmd-cmp_mod-commands_cmp.lst fs-cmp_mod-commands_cmp.lst
COMMANDFILES += cmd-cmp_mod-commands_cmp.lst
FSFILES += fs-cmp_mod-commands_cmp.lst

cmd-cmp_mod-commands_cmp.lst: commands/cmp.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cmp_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh cmp > $@ || (rm -f $@; exit 1)

fs-cmp_mod-commands_cmp.lst: commands/cmp.c genfslist.sh
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

cat_mod-commands_cat.o: commands/cat.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(cat_mod_CFLAGS) -MD -c -o $@ $<
-include cat_mod-commands_cat.d

CLEANFILES += cmd-cat_mod-commands_cat.lst fs-cat_mod-commands_cat.lst
COMMANDFILES += cmd-cat_mod-commands_cat.lst
FSFILES += fs-cat_mod-commands_cat.lst

cmd-cat_mod-commands_cat.lst: commands/cat.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh cat > $@ || (rm -f $@; exit 1)

fs-cat_mod-commands_cat.lst: commands/cat.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(cat_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh cat > $@ || (rm -f $@; exit 1)


cat_mod_CFLAGS = $(COMMON_CFLAGS)
cat_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For echo.mod
echo_mod_SOURCES = commands/echo.c
echo_mod_CFLAGS = $(COMMON_CFLAGS)
echo_mod_LDFLAGS = $(COMMON_LDFLAGS)

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

help_mod-commands_help.o: commands/help.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(help_mod_CFLAGS) -MD -c -o $@ $<
-include help_mod-commands_help.d

CLEANFILES += cmd-help_mod-commands_help.lst fs-help_mod-commands_help.lst
COMMANDFILES += cmd-help_mod-commands_help.lst
FSFILES += fs-help_mod-commands_help.lst

cmd-help_mod-commands_help.lst: commands/help.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(help_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh help > $@ || (rm -f $@; exit 1)

fs-help_mod-commands_help.lst: commands/help.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(help_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh help > $@ || (rm -f $@; exit 1)


help_mod_CFLAGS = $(COMMON_CFLAGS)
help_mod_LDFLAGS = $(COMMON_LDFLAGS)

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

font_mod-font_manager.o: font/manager.c
	$(TARGET_CC) -Ifont -I$(srcdir)/font $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(font_mod_CFLAGS) -MD -c -o $@ $<
-include font_mod-font_manager.d

CLEANFILES += cmd-font_mod-font_manager.lst fs-font_mod-font_manager.lst
COMMANDFILES += cmd-font_mod-font_manager.lst
FSFILES += fs-font_mod-font_manager.lst

cmd-font_mod-font_manager.lst: font/manager.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ifont -I$(srcdir)/font $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(font_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh font > $@ || (rm -f $@; exit 1)

fs-font_mod-font_manager.lst: font/manager.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ifont -I$(srcdir)/font $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(font_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh font > $@ || (rm -f $@; exit 1)


font_mod_CFLAGS = $(COMMON_CFLAGS)
font_mod_LDFLAGS = $(COMMON_LDFLAGS)

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

search_mod-commands_search.o: commands/search.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(search_mod_CFLAGS) -MD -c -o $@ $<
-include search_mod-commands_search.d

CLEANFILES += cmd-search_mod-commands_search.lst fs-search_mod-commands_search.lst
COMMANDFILES += cmd-search_mod-commands_search.lst
FSFILES += fs-search_mod-commands_search.lst

cmd-search_mod-commands_search.lst: commands/search.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(search_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh search > $@ || (rm -f $@; exit 1)

fs-search_mod-commands_search.lst: commands/search.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(search_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh search > $@ || (rm -f $@; exit 1)


search_mod_CFLAGS = $(COMMON_CFLAGS)
search_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For test.mod.
test_mod_SOURCES = commands/test.c
CLEANFILES += test.mod mod-test.o mod-test.c pre-test.o test_mod-commands_test.o und-test.lst
ifneq ($(test_mod_EXPORTS),no)
CLEANFILES += def-test.lst
DEFSYMFILES += def-test.lst
endif
MOSTLYCLEANFILES += test_mod-commands_test.d
UNDSYMFILES += und-test.lst

test.mod: pre-test.o mod-test.o
	-rm -f $@
	$(TARGET_CC) $(test_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-test.o: $(test_mod_DEPENDENCIES) test_mod-commands_test.o
	-rm -f $@
	$(TARGET_CC) $(test_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ test_mod-commands_test.o

mod-test.o: mod-test.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(test_mod_CFLAGS) -c -o $@ $<

mod-test.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'test' $< > $@ || (rm -f $@; exit 1)

ifneq ($(test_mod_EXPORTS),no)
def-test.lst: pre-test.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 test/' > $@
endif

und-test.lst: pre-test.o
	echo 'test' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

test_mod-commands_test.o: commands/test.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(test_mod_CFLAGS) -MD -c -o $@ $<
-include test_mod-commands_test.d

CLEANFILES += cmd-test_mod-commands_test.lst fs-test_mod-commands_test.lst
COMMANDFILES += cmd-test_mod-commands_test.lst
FSFILES += fs-test_mod-commands_test.lst

cmd-test_mod-commands_test.lst: commands/test.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(test_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh test > $@ || (rm -f $@; exit 1)

fs-test_mod-commands_test.lst: commands/test.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(test_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh test > $@ || (rm -f $@; exit 1)


test_mod_CFLAGS = $(COMMON_CFLAGS)
test_mod_LDFLAGS = $(COMMON_LDFLAGS)

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

loopback_mod-disk_loopback.o: disk/loopback.c
	$(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(loopback_mod_CFLAGS) -MD -c -o $@ $<
-include loopback_mod-disk_loopback.d

CLEANFILES += cmd-loopback_mod-disk_loopback.lst fs-loopback_mod-disk_loopback.lst
COMMANDFILES += cmd-loopback_mod-disk_loopback.lst
FSFILES += fs-loopback_mod-disk_loopback.lst

cmd-loopback_mod-disk_loopback.lst: disk/loopback.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(loopback_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh loopback > $@ || (rm -f $@; exit 1)

fs-loopback_mod-disk_loopback.lst: disk/loopback.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Idisk -I$(srcdir)/disk $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(loopback_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh loopback > $@ || (rm -f $@; exit 1)


loopback_mod_CFLAGS = $(COMMON_CFLAGS)
loopback_mod_LDFLAGS = $(COMMON_LDFLAGS)

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

configfile_mod-commands_configfile.o: commands/configfile.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(configfile_mod_CFLAGS) -MD -c -o $@ $<
-include configfile_mod-commands_configfile.d

CLEANFILES += cmd-configfile_mod-commands_configfile.lst fs-configfile_mod-commands_configfile.lst
COMMANDFILES += cmd-configfile_mod-commands_configfile.lst
FSFILES += fs-configfile_mod-commands_configfile.lst

cmd-configfile_mod-commands_configfile.lst: commands/configfile.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(configfile_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh configfile > $@ || (rm -f $@; exit 1)

fs-configfile_mod-commands_configfile.lst: commands/configfile.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(configfile_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh configfile > $@ || (rm -f $@; exit 1)


configfile_mod_CFLAGS = $(COMMON_CFLAGS)
configfile_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For terminfo.mod.
terminfo_mod_SOURCES = term/terminfo.c term/tparm.c
CLEANFILES += terminfo.mod mod-terminfo.o mod-terminfo.c pre-terminfo.o terminfo_mod-term_terminfo.o terminfo_mod-term_tparm.o und-terminfo.lst
ifneq ($(terminfo_mod_EXPORTS),no)
CLEANFILES += def-terminfo.lst
DEFSYMFILES += def-terminfo.lst
endif
MOSTLYCLEANFILES += terminfo_mod-term_terminfo.d terminfo_mod-term_tparm.d
UNDSYMFILES += und-terminfo.lst

terminfo.mod: pre-terminfo.o mod-terminfo.o
	-rm -f $@
	$(TARGET_CC) $(terminfo_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-terminfo.o: $(terminfo_mod_DEPENDENCIES) terminfo_mod-term_terminfo.o terminfo_mod-term_tparm.o
	-rm -f $@
	$(TARGET_CC) $(terminfo_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ terminfo_mod-term_terminfo.o terminfo_mod-term_tparm.o

mod-terminfo.o: mod-terminfo.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(terminfo_mod_CFLAGS) -c -o $@ $<

mod-terminfo.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'terminfo' $< > $@ || (rm -f $@; exit 1)

ifneq ($(terminfo_mod_EXPORTS),no)
def-terminfo.lst: pre-terminfo.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 terminfo/' > $@
endif

und-terminfo.lst: pre-terminfo.o
	echo 'terminfo' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

terminfo_mod-term_terminfo.o: term/terminfo.c
	$(TARGET_CC) -Iterm -I$(srcdir)/term $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(terminfo_mod_CFLAGS) -MD -c -o $@ $<
-include terminfo_mod-term_terminfo.d

CLEANFILES += cmd-terminfo_mod-term_terminfo.lst fs-terminfo_mod-term_terminfo.lst
COMMANDFILES += cmd-terminfo_mod-term_terminfo.lst
FSFILES += fs-terminfo_mod-term_terminfo.lst

cmd-terminfo_mod-term_terminfo.lst: term/terminfo.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iterm -I$(srcdir)/term $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(terminfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh terminfo > $@ || (rm -f $@; exit 1)

fs-terminfo_mod-term_terminfo.lst: term/terminfo.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iterm -I$(srcdir)/term $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(terminfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh terminfo > $@ || (rm -f $@; exit 1)


terminfo_mod-term_tparm.o: term/tparm.c
	$(TARGET_CC) -Iterm -I$(srcdir)/term $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(terminfo_mod_CFLAGS) -MD -c -o $@ $<
-include terminfo_mod-term_tparm.d

CLEANFILES += cmd-terminfo_mod-term_tparm.lst fs-terminfo_mod-term_tparm.lst
COMMANDFILES += cmd-terminfo_mod-term_tparm.lst
FSFILES += fs-terminfo_mod-term_tparm.lst

cmd-terminfo_mod-term_tparm.lst: term/tparm.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iterm -I$(srcdir)/term $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(terminfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh terminfo > $@ || (rm -f $@; exit 1)

fs-terminfo_mod-term_tparm.lst: term/tparm.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iterm -I$(srcdir)/term $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(terminfo_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh terminfo > $@ || (rm -f $@; exit 1)


terminfo_mod_CFLAGS = $(COMMON_CFLAGS)
terminfo_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For blocklist.mod.
blocklist_mod_SOURCES = commands/blocklist.c
CLEANFILES += blocklist.mod mod-blocklist.o mod-blocklist.c pre-blocklist.o blocklist_mod-commands_blocklist.o und-blocklist.lst
ifneq ($(blocklist_mod_EXPORTS),no)
CLEANFILES += def-blocklist.lst
DEFSYMFILES += def-blocklist.lst
endif
MOSTLYCLEANFILES += blocklist_mod-commands_blocklist.d
UNDSYMFILES += und-blocklist.lst

blocklist.mod: pre-blocklist.o mod-blocklist.o
	-rm -f $@
	$(TARGET_CC) $(blocklist_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-blocklist.o: $(blocklist_mod_DEPENDENCIES) blocklist_mod-commands_blocklist.o
	-rm -f $@
	$(TARGET_CC) $(blocklist_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ blocklist_mod-commands_blocklist.o

mod-blocklist.o: mod-blocklist.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(blocklist_mod_CFLAGS) -c -o $@ $<

mod-blocklist.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'blocklist' $< > $@ || (rm -f $@; exit 1)

ifneq ($(blocklist_mod_EXPORTS),no)
def-blocklist.lst: pre-blocklist.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 blocklist/' > $@
endif

und-blocklist.lst: pre-blocklist.o
	echo 'blocklist' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

blocklist_mod-commands_blocklist.o: commands/blocklist.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(blocklist_mod_CFLAGS) -MD -c -o $@ $<
-include blocklist_mod-commands_blocklist.d

CLEANFILES += cmd-blocklist_mod-commands_blocklist.lst fs-blocklist_mod-commands_blocklist.lst
COMMANDFILES += cmd-blocklist_mod-commands_blocklist.lst
FSFILES += fs-blocklist_mod-commands_blocklist.lst

cmd-blocklist_mod-commands_blocklist.lst: commands/blocklist.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(blocklist_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh blocklist > $@ || (rm -f $@; exit 1)

fs-blocklist_mod-commands_blocklist.lst: commands/blocklist.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(blocklist_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh blocklist > $@ || (rm -f $@; exit 1)


blocklist_mod_CFLAGS = $(COMMON_CFLAGS)
blocklist_mod_LDFLAGS = $(COMMON_LDFLAGS)

# For hexdump.mod.
hexdump_mod_SOURCES = commands/hexdump.c
CLEANFILES += hexdump.mod mod-hexdump.o mod-hexdump.c pre-hexdump.o hexdump_mod-commands_hexdump.o und-hexdump.lst
ifneq ($(hexdump_mod_EXPORTS),no)
CLEANFILES += def-hexdump.lst
DEFSYMFILES += def-hexdump.lst
endif
MOSTLYCLEANFILES += hexdump_mod-commands_hexdump.d
UNDSYMFILES += und-hexdump.lst

hexdump.mod: pre-hexdump.o mod-hexdump.o
	-rm -f $@
	$(TARGET_CC) $(hexdump_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-hexdump.o: $(hexdump_mod_DEPENDENCIES) hexdump_mod-commands_hexdump.o
	-rm -f $@
	$(TARGET_CC) $(hexdump_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ hexdump_mod-commands_hexdump.o

mod-hexdump.o: mod-hexdump.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hexdump_mod_CFLAGS) -c -o $@ $<

mod-hexdump.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'hexdump' $< > $@ || (rm -f $@; exit 1)

ifneq ($(hexdump_mod_EXPORTS),no)
def-hexdump.lst: pre-hexdump.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 hexdump/' > $@
endif

und-hexdump.lst: pre-hexdump.o
	echo 'hexdump' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

hexdump_mod-commands_hexdump.o: commands/hexdump.c
	$(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(hexdump_mod_CFLAGS) -MD -c -o $@ $<
-include hexdump_mod-commands_hexdump.d

CLEANFILES += cmd-hexdump_mod-commands_hexdump.lst fs-hexdump_mod-commands_hexdump.lst
COMMANDFILES += cmd-hexdump_mod-commands_hexdump.lst
FSFILES += fs-hexdump_mod-commands_hexdump.lst

cmd-hexdump_mod-commands_hexdump.lst: commands/hexdump.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hexdump_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh hexdump > $@ || (rm -f $@; exit 1)

fs-hexdump_mod-commands_hexdump.lst: commands/hexdump.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Icommands -I$(srcdir)/commands $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(hexdump_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh hexdump > $@ || (rm -f $@; exit 1)


hexdump_mod_CFLAGS = $(COMMON_CFLAGS)
hexdump_mod_LDFLAGS = $(COMMON_LDFLAGS)

# Misc.
pkglib_MODULES += gzio.mod elf.mod

# For elf.mod.
elf_mod_SOURCES = kern/elf.c
CLEANFILES += elf.mod mod-elf.o mod-elf.c pre-elf.o elf_mod-kern_elf.o und-elf.lst
ifneq ($(elf_mod_EXPORTS),no)
CLEANFILES += def-elf.lst
DEFSYMFILES += def-elf.lst
endif
MOSTLYCLEANFILES += elf_mod-kern_elf.d
UNDSYMFILES += und-elf.lst

elf.mod: pre-elf.o mod-elf.o
	-rm -f $@
	$(TARGET_CC) $(elf_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

pre-elf.o: $(elf_mod_DEPENDENCIES) elf_mod-kern_elf.o
	-rm -f $@
	$(TARGET_CC) $(elf_mod_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ elf_mod-kern_elf.o

mod-elf.o: mod-elf.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(elf_mod_CFLAGS) -c -o $@ $<

mod-elf.c: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh 'elf' $< > $@ || (rm -f $@; exit 1)

ifneq ($(elf_mod_EXPORTS),no)
def-elf.lst: pre-elf.o
	$(NM) -g --defined-only -P -p $< | sed 's/^\([^ ]*\).*/\1 elf/' > $@
endif

und-elf.lst: pre-elf.o
	echo 'elf' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

elf_mod-kern_elf.o: kern/elf.c
	$(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(elf_mod_CFLAGS) -MD -c -o $@ $<
-include elf_mod-kern_elf.d

CLEANFILES += cmd-elf_mod-kern_elf.lst fs-elf_mod-kern_elf.lst
COMMANDFILES += cmd-elf_mod-kern_elf.lst
FSFILES += fs-elf_mod-kern_elf.lst

cmd-elf_mod-kern_elf.lst: kern/elf.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(elf_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh elf > $@ || (rm -f $@; exit 1)

fs-elf_mod-kern_elf.lst: kern/elf.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Ikern -I$(srcdir)/kern $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(elf_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh elf > $@ || (rm -f $@; exit 1)


elf_mod_CFLAGS = $(COMMON_CFLAGS)
elf_mod_LDFLAGS = $(COMMON_LDFLAGS)

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

gzio_mod-io_gzio.o: io/gzio.c
	$(TARGET_CC) -Iio -I$(srcdir)/io $(TARGET_CPPFLAGS)  $(TARGET_CFLAGS) $(gzio_mod_CFLAGS) -MD -c -o $@ $<
-include gzio_mod-io_gzio.d

CLEANFILES += cmd-gzio_mod-io_gzio.lst fs-gzio_mod-io_gzio.lst
COMMANDFILES += cmd-gzio_mod-io_gzio.lst
FSFILES += fs-gzio_mod-io_gzio.lst

cmd-gzio_mod-io_gzio.lst: io/gzio.c gencmdlist.sh
	set -e; 	  $(TARGET_CC) -Iio -I$(srcdir)/io $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gzio_mod_CFLAGS) -E $< 	  | sh $(srcdir)/gencmdlist.sh gzio > $@ || (rm -f $@; exit 1)

fs-gzio_mod-io_gzio.lst: io/gzio.c genfslist.sh
	set -e; 	  $(TARGET_CC) -Iio -I$(srcdir)/io $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(gzio_mod_CFLAGS) -E $< 	  | sh $(srcdir)/genfslist.sh gzio > $@ || (rm -f $@; exit 1)


gzio_mod_CFLAGS = $(COMMON_CFLAGS)
gzio_mod_LDFLAGS = $(COMMON_LDFLAGS)



