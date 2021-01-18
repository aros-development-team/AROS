
LIBDIR=../Lib
LIBNAME=libadf.a

DEPEND=makedepend

CFLAGS=-I$(LIBDIR) -O2 -Wall
LDFLAGS=-L$(LIBDIR) -ladf

EXES= fl_test fl_test2 dir_test dir_test2 hd_test hd_test2 hd_test3 \
	file_test file_test2 file_test3 del_test bootdisk \
	rename hardfile rename2 hardfile2 access comment undel readonly \
    undel2 dispsect progbar undel3

CC=gcc

all: $(EXES)

lib:
	cd $(LIBDIR) && $(MAKE)

access: lib access.o
	$(CC) $(CFLAGS) -o $@ access.o $(LDFLAGS)

dispsect: lib dispsect.o
	$(CC) $(CFLAGS) -o $@ dispsect.o $(LDFLAGS)

progbar: lib progbar.o
	$(CC) $(CFLAGS) -o $@ progbar.o $(LDFLAGS)

readonly: lib readonly.o
	$(CC) $(CFLAGS) -o $@ readonly.o $(LDFLAGS)

undel: lib undel.o
	$(CC) $(CFLAGS) -o $@ undel.o $(LDFLAGS)

undel2: lib undel2.o
	$(CC) $(CFLAGS) -o $@ undel2.o $(LDFLAGS)

undel3: lib undel3.o
	$(CC) $(CFLAGS) -o $@ undel3.o $(LDFLAGS)

comment: lib comment.o
	$(CC) $(CFLAGS) -o $@ comment.o $(LDFLAGS)

bootdisk2: lib bootdisk2.o
	$(CC) $(CFLAGS) -o $@ bootdisk2.o $(LDFLAGS)

hardfile: lib hardfile.o
	$(CC) $(CFLAGS) -o $@ hardfile.o $(LDFLAGS)

hardfile2: lib hardfile2.o
	$(CC) $(CFLAGS) -o $@ hardfile2.o $(LDFLAGS)

rename: lib rename.o
	$(CC) $(CFLAGS) -o $@ rename.o $(LDFLAGS)

rename2: lib rename2.o
	$(CC) $(CFLAGS) -o $@ rename2.o $(LDFLAGS)

bootdisk: lib bootdisk.o
	$(CC) $(CFLAGS) -o $@ bootdisk.o $(LDFLAGS)

fl_test: lib fl_test.o
	$(CC) $(CFLAGS) -o $@ fl_test.o $(LDFLAGS)

fl_test2: lib fl_test2.o
	$(CC) $(CFLAGS) -o $@ fl_test2.o $(LDFLAGS)

dir_test: lib dir_test.o
	$(CC) $(CFLAGS) -o $@ dir_test.o $(LDFLAGS)

dir_test2: lib dir_test2.o
	$(CC) $(CFLAGS) -o $@ dir_test2.o $(LDFLAGS)

hd_test: lib hd_test.o
	$(CC) $(CFLAGS) -o $@ hd_test.o $(LDFLAGS)

hd_test2: lib hd_test2.o
	$(CC) $(CFLAGS) -o $@ hd_test2.o $(LDFLAGS)

hd_test3: lib hd_test3.o
	$(CC) $(CFLAGS) -o $@ hd_test3.o $(LDFLAGS)

file_test: lib file_test.o
	$(CC) $(CFLAGS) -o $@ file_test.o $(LDFLAGS)

file_test2: lib file_test2.o
	$(CC) $(CFLAGS) -o $@ file_test2.o $(LDFLAGS)

file_test3: lib file_test3.o
	$(CC) $(CFLAGS) -o $@ file_test3.o $(LDFLAGS)

del_test: lib del_test.o
	$(CC) $(CFLAGS) -o $@ del_test.o $(LDFLAGS)

dirc: lib dirc.o
	$(CC) $(CFLAGS) -o $@ dirc.o $(LDFLAGS)

clean:
	rm *.o $(EXES) core newdev

depend:
	$(DEPEND) -v -- $(CFLAGS) --  *.[ch]


# DO NOT DELETE THIS LINE -- make depend depends on it.

access.o: /usr/include/stdio.h
# /usr/include/stdio.h includes:
#	features.h
#	stddef.h
#	stdarg.h
#	bits/types.h
#	libio.h
#	bits/stdio_lim.h
access.o: /usr/include/features.h
# /usr/include/features.h includes:
#	sys/cdefs.h
#	gnu/stubs.h
access.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
access.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
access.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
access.o: /usr/include/bits/types.h
# /usr/include/bits/types.h includes:
#	features.h
#	stddef.h
access.o: /usr/include/libio.h
# /usr/include/libio.h includes:
#	_G_config.h
#	stdarg.h
access.o: /usr/include/_G_config.h
# /usr/include/_G_config.h includes:
#	bits/types.h
#	stddef.h
access.o: /usr/include/bits/stdio_lim.h /usr/include/stdlib.h
# /usr/include/stdlib.h includes:
#	features.h
#	stddef.h
#	sys/types.h
#	alloca.h
access.o: /usr/include/sys/types.h
# /usr/include/sys/types.h includes:
#	features.h
#	bits/types.h
#	time.h
#	stddef.h
#	endian.h
#	sys/select.h
#	sys/sysmacros.h
access.o: /usr/include/time.h
# /usr/include/time.h includes:
#	bits/types.h
access.o: /usr/include/endian.h
# /usr/include/endian.h includes:
#	features.h
#	bits/endian.h
access.o: /usr/include/bits/endian.h /usr/include/sys/select.h
# /usr/include/sys/select.h includes:
#	features.h
#	bits/types.h
#	bits/select.h
#	bits/sigset.h
#	time.h
access.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
access.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
# /usr/include/alloca.h includes:
#	features.h
#	stddef.h
access.o: /usr/include/string.h
# /usr/include/string.h includes:
#	features.h
#	stddef.h
access.o: ../Lib/adflib.h
# ../Lib/adflib.h includes:
#	adf_defs.h
#	adf_str.h
access.o: ../Lib/adf_defs.h ../Lib/adf_str.h
# ../Lib/adf_str.h includes:
#	stdio.h
#	adf_defs.h
#	adf_blk.h
#	adf_err.h
access.o: ../Lib/adf_blk.h ../Lib/adf_err.h
bootdisk.o: /usr/include/stdio.h /usr/include/features.h
bootdisk.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
bootdisk.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
bootdisk.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
bootdisk.o: /usr/include/bits/types.h /usr/include/libio.h
bootdisk.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
bootdisk.o: /usr/include/stdlib.h /usr/include/sys/types.h
bootdisk.o: /usr/include/time.h /usr/include/endian.h
bootdisk.o: /usr/include/bits/endian.h /usr/include/sys/select.h
bootdisk.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
bootdisk.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
bootdisk.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
bootdisk.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
bootdisk2.o: /usr/include/stdio.h /usr/include/features.h
bootdisk2.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
bootdisk2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
bootdisk2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
bootdisk2.o: /usr/include/bits/types.h /usr/include/libio.h
bootdisk2.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
bootdisk2.o: /usr/include/stdlib.h /usr/include/sys/types.h
bootdisk2.o: /usr/include/time.h /usr/include/endian.h
bootdisk2.o: /usr/include/bits/endian.h /usr/include/sys/select.h
bootdisk2.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
bootdisk2.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
bootdisk2.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
bootdisk2.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
cache_test.o: /usr/include/stdio.h /usr/include/features.h
cache_test.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
cache_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
cache_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
cache_test.o: /usr/include/bits/types.h /usr/include/libio.h
cache_test.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
cache_test.o: /usr/include/stdlib.h /usr/include/sys/types.h
cache_test.o: /usr/include/time.h /usr/include/endian.h
cache_test.o: /usr/include/bits/endian.h /usr/include/sys/select.h
cache_test.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
cache_test.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
cache_test.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
cache_test.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
comment.o: /usr/include/stdio.h /usr/include/features.h
comment.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
comment.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
comment.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
comment.o: /usr/include/bits/types.h /usr/include/libio.h
comment.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
comment.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
comment.o: /usr/include/endian.h /usr/include/bits/endian.h
comment.o: /usr/include/sys/select.h /usr/include/bits/select.h
comment.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
comment.o: /usr/include/alloca.h /usr/include/string.h ../Lib/adflib.h
comment.o: ../Lib/adf_defs.h ../Lib/adf_str.h ../Lib/adf_blk.h
comment.o: ../Lib/adf_err.h
del_test.o: /usr/include/stdio.h /usr/include/features.h
del_test.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
del_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
del_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
del_test.o: /usr/include/bits/types.h /usr/include/libio.h
del_test.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
del_test.o: /usr/include/stdlib.h /usr/include/sys/types.h
del_test.o: /usr/include/time.h /usr/include/endian.h
del_test.o: /usr/include/bits/endian.h /usr/include/sys/select.h
del_test.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
del_test.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
del_test.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
del_test.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
dir_test.o: /usr/include/stdio.h /usr/include/features.h
dir_test.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
dir_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
dir_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
dir_test.o: /usr/include/bits/types.h /usr/include/libio.h
dir_test.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
dir_test.o: /usr/include/stdlib.h /usr/include/sys/types.h
dir_test.o: /usr/include/time.h /usr/include/endian.h
dir_test.o: /usr/include/bits/endian.h /usr/include/sys/select.h
dir_test.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
dir_test.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
dir_test.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
dir_test.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
dir_test2.o: /usr/include/stdio.h /usr/include/features.h
dir_test2.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
dir_test2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
dir_test2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
dir_test2.o: /usr/include/bits/types.h /usr/include/libio.h
dir_test2.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
dir_test2.o: /usr/include/stdlib.h /usr/include/sys/types.h
dir_test2.o: /usr/include/time.h /usr/include/endian.h
dir_test2.o: /usr/include/bits/endian.h /usr/include/sys/select.h
dir_test2.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
dir_test2.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
dir_test2.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
dir_test2.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
dirc.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
dirc.o: /usr/include/gnu/stubs.h
dirc.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
dirc.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
dirc.o: /usr/include/bits/types.h /usr/include/libio.h
dirc.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
dirc.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
dirc.o: /usr/include/endian.h /usr/include/bits/endian.h
dirc.o: /usr/include/sys/select.h /usr/include/bits/select.h
dirc.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
dirc.o: /usr/include/alloca.h /usr/include/string.h ../Lib/adflib.h
dirc.o: ../Lib/adf_defs.h ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
dirc_test.o: /usr/include/stdio.h /usr/include/features.h
dirc_test.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
dirc_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
dirc_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
dirc_test.o: /usr/include/bits/types.h /usr/include/libio.h
dirc_test.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
dirc_test.o: /usr/include/stdlib.h /usr/include/sys/types.h
dirc_test.o: /usr/include/time.h /usr/include/endian.h
dirc_test.o: /usr/include/bits/endian.h /usr/include/sys/select.h
dirc_test.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
dirc_test.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
dirc_test.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
dirc_test.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
file_test.o: /usr/include/stdio.h /usr/include/features.h
file_test.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
file_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
file_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
file_test.o: /usr/include/bits/types.h /usr/include/libio.h
file_test.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
file_test.o: /usr/include/stdlib.h /usr/include/sys/types.h
file_test.o: /usr/include/time.h /usr/include/endian.h
file_test.o: /usr/include/bits/endian.h /usr/include/sys/select.h
file_test.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
file_test.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
file_test.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
file_test.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
file_test2.o: /usr/include/stdio.h /usr/include/features.h
file_test2.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
file_test2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
file_test2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
file_test2.o: /usr/include/bits/types.h /usr/include/libio.h
file_test2.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
file_test2.o: /usr/include/stdlib.h /usr/include/sys/types.h
file_test2.o: /usr/include/time.h /usr/include/endian.h
file_test2.o: /usr/include/bits/endian.h /usr/include/sys/select.h
file_test2.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
file_test2.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
file_test2.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
file_test2.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
file_test2a.o: /usr/include/stdio.h /usr/include/features.h
file_test2a.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
file_test2a.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
file_test2a.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
file_test2a.o: /usr/include/bits/types.h /usr/include/libio.h
file_test2a.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
file_test2a.o: /usr/include/stdlib.h /usr/include/sys/types.h
file_test2a.o: /usr/include/time.h /usr/include/endian.h
file_test2a.o: /usr/include/bits/endian.h /usr/include/sys/select.h
file_test2a.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
file_test2a.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
file_test2a.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
file_test2a.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
file_test3.o: /usr/include/stdio.h /usr/include/features.h
file_test3.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
file_test3.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
file_test3.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
file_test3.o: /usr/include/bits/types.h /usr/include/libio.h
file_test3.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
file_test3.o: /usr/include/stdlib.h /usr/include/sys/types.h
file_test3.o: /usr/include/time.h /usr/include/endian.h
file_test3.o: /usr/include/bits/endian.h /usr/include/sys/select.h
file_test3.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
file_test3.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
file_test3.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
file_test3.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
fl_test.o: /usr/include/stdio.h /usr/include/features.h
fl_test.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
fl_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
fl_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
fl_test.o: /usr/include/bits/types.h /usr/include/libio.h
fl_test.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
fl_test.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
fl_test.o: /usr/include/endian.h /usr/include/bits/endian.h
fl_test.o: /usr/include/sys/select.h /usr/include/bits/select.h
fl_test.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
fl_test.o: /usr/include/alloca.h /usr/include/string.h ../Lib/adflib.h
fl_test.o: ../Lib/adf_defs.h ../Lib/adf_str.h ../Lib/adf_blk.h
fl_test.o: ../Lib/adf_err.h
fl_test2.o: /usr/include/stdio.h /usr/include/features.h
fl_test2.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
fl_test2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
fl_test2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
fl_test2.o: /usr/include/bits/types.h /usr/include/libio.h
fl_test2.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
fl_test2.o: /usr/include/stdlib.h /usr/include/sys/types.h
fl_test2.o: /usr/include/time.h /usr/include/endian.h
fl_test2.o: /usr/include/bits/endian.h /usr/include/sys/select.h
fl_test2.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
fl_test2.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
fl_test2.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
fl_test2.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
flfile_test.o: /usr/include/stdio.h /usr/include/features.h
flfile_test.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
flfile_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
flfile_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
flfile_test.o: /usr/include/bits/types.h /usr/include/libio.h
flfile_test.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
flfile_test.o: /usr/include/stdlib.h /usr/include/sys/types.h
flfile_test.o: /usr/include/time.h /usr/include/endian.h
flfile_test.o: /usr/include/bits/endian.h /usr/include/sys/select.h
flfile_test.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
flfile_test.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
flfile_test.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
flfile_test.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
hardfile.o: /usr/include/stdio.h /usr/include/features.h
hardfile.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
hardfile.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
hardfile.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
hardfile.o: /usr/include/bits/types.h /usr/include/libio.h
hardfile.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
hardfile.o: /usr/include/stdlib.h /usr/include/sys/types.h
hardfile.o: /usr/include/time.h /usr/include/endian.h
hardfile.o: /usr/include/bits/endian.h /usr/include/sys/select.h
hardfile.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
hardfile.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
hardfile.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
hardfile.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
hardfile2.o: /usr/include/stdio.h /usr/include/features.h
hardfile2.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
hardfile2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
hardfile2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
hardfile2.o: /usr/include/bits/types.h /usr/include/libio.h
hardfile2.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
hardfile2.o: /usr/include/stdlib.h /usr/include/sys/types.h
hardfile2.o: /usr/include/time.h /usr/include/endian.h
hardfile2.o: /usr/include/bits/endian.h /usr/include/sys/select.h
hardfile2.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
hardfile2.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
hardfile2.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
hardfile2.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
hd_test.o: /usr/include/stdio.h /usr/include/features.h
hd_test.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
hd_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
hd_test.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
hd_test.o: /usr/include/bits/types.h /usr/include/libio.h
hd_test.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
hd_test.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
hd_test.o: /usr/include/endian.h /usr/include/bits/endian.h
hd_test.o: /usr/include/sys/select.h /usr/include/bits/select.h
hd_test.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
hd_test.o: /usr/include/alloca.h /usr/include/string.h ../Lib/adflib.h
hd_test.o: ../Lib/adf_defs.h ../Lib/adf_str.h ../Lib/adf_blk.h
hd_test.o: ../Lib/adf_err.h
hd_test2.o: /usr/include/stdio.h /usr/include/features.h
hd_test2.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
hd_test2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
hd_test2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
hd_test2.o: /usr/include/bits/types.h /usr/include/libio.h
hd_test2.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
hd_test2.o: /usr/include/stdlib.h /usr/include/sys/types.h
hd_test2.o: /usr/include/time.h /usr/include/endian.h
hd_test2.o: /usr/include/bits/endian.h /usr/include/sys/select.h
hd_test2.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
hd_test2.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
hd_test2.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
hd_test2.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
hd_test3.o: /usr/include/stdio.h /usr/include/features.h
hd_test3.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
hd_test3.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
hd_test3.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
hd_test3.o: /usr/include/bits/types.h /usr/include/libio.h
hd_test3.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
hd_test3.o: /usr/include/stdlib.h /usr/include/sys/types.h
hd_test3.o: /usr/include/time.h /usr/include/endian.h
hd_test3.o: /usr/include/bits/endian.h /usr/include/sys/select.h
hd_test3.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
hd_test3.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
hd_test3.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
hd_test3.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
readonly.o: /usr/include/stdio.h /usr/include/features.h
readonly.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
readonly.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
readonly.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
readonly.o: /usr/include/bits/types.h /usr/include/libio.h
readonly.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
readonly.o: /usr/include/stdlib.h /usr/include/sys/types.h
readonly.o: /usr/include/time.h /usr/include/endian.h
readonly.o: /usr/include/bits/endian.h /usr/include/sys/select.h
readonly.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
readonly.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
readonly.o: /usr/include/string.h ../Lib/adflib.h ../Lib/adf_defs.h
readonly.o: ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
rename.o: /usr/include/stdio.h /usr/include/features.h
rename.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
rename.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
rename.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
rename.o: /usr/include/bits/types.h /usr/include/libio.h
rename.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
rename.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
rename.o: /usr/include/endian.h /usr/include/bits/endian.h
rename.o: /usr/include/sys/select.h /usr/include/bits/select.h
rename.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
rename.o: /usr/include/alloca.h /usr/include/string.h ../Lib/adflib.h
rename.o: ../Lib/adf_defs.h ../Lib/adf_str.h ../Lib/adf_blk.h
rename.o: ../Lib/adf_err.h
rename2.o: /usr/include/stdio.h /usr/include/features.h
rename2.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
rename2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
rename2.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
rename2.o: /usr/include/bits/types.h /usr/include/libio.h
rename2.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
rename2.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
rename2.o: /usr/include/endian.h /usr/include/bits/endian.h
rename2.o: /usr/include/sys/select.h /usr/include/bits/select.h
rename2.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
rename2.o: /usr/include/alloca.h /usr/include/string.h ../Lib/adflib.h
rename2.o: ../Lib/adf_defs.h ../Lib/adf_str.h ../Lib/adf_blk.h
rename2.o: ../Lib/adf_err.h
undel.o: /usr/include/stdio.h /usr/include/features.h
undel.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
undel.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
undel.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
undel.o: /usr/include/bits/types.h /usr/include/libio.h
undel.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
undel.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
undel.o: /usr/include/endian.h /usr/include/bits/endian.h
undel.o: /usr/include/sys/select.h /usr/include/bits/select.h
undel.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
undel.o: /usr/include/alloca.h /usr/include/string.h ../Lib/adflib.h
undel.o: ../Lib/adf_defs.h ../Lib/adf_str.h ../Lib/adf_blk.h ../Lib/adf_err.h
