bin_PROGRAMS = mbchk
sbin_SCRIPTS = grub-install
noinst_SCRIPTS = grub-image

# XXX: Need to search for a header file in docs, because of multiboot.h.
AM_CFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir)/docs

mbchk_SOURCES = mbchk.c
mbchk_LDADD = ../lib/libcommon.a
