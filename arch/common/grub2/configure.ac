# Process this file with autoconf to produce a configure script.

# Copyright (C) 2002,2003,2004,2005  Free Software Foundation, Inc.
#
# This configure.ac is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

AC_INIT(GRUB, 1.91, [bug-grub@gnu.org])
AC_PREREQ(2.53)
AC_CONFIG_SRCDIR([include/grub/dl.h])
AC_CONFIG_HEADER([config.h])

# Checks for build and host systems.
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

case "$host_cpu" in
  i[[3456]]86) host_cpu=i386 ;;
  x86_64) host_cpu=i386 amd64=1 ;;
  powerpc) ;;
  sparc64) ;;
  *) AC_MSG_ERROR([unsupported CPU type]) ;;
esac

case "$host_cpu"-"$host_vendor" in
  i386-*) host_vendor=pc ;;
  powerpc-*) host_vendor=ieee1275 ;;
  sparc64-*) host_vendor=ieee1275 ;;
  *) AC_MSG_ERROR([unsupported machine type]) ;;
esac

AC_SUBST(host_cpu)
AC_SUBST(host_vendor)

# Checks for programs.
if test "x$CFLAGS" = x; then
  default_CFLAGS=yes
fi

AC_PROG_CC
AC_SYS_LARGEFILE

# Must be GCC.
test "x$GCC" = xyes || AC_MSG_ERROR([GCC is required])

if test "x$default_CFLAGS" = xyes; then
  # debug flags.
  tmp_CFLAGS="-Wall -W -Wshadow -Wpointer-arith -Wmissing-prototypes \
		-Wundef -Wstrict-prototypes -g"

  # optimization flags.
  AC_CACHE_CHECK([whether optimization for size works], size_flag, [
    CFLAGS=-Os
    AC_TRY_COMPILE(, , size_flag=yes, size_flag=no)
  ])
  if test "x$size_flag" = xyes; then
    tmp_CFLAGS="$tmp_CFLAGS -Os"
  else
    tmp_CFLAGS="$tmp_CFLAGS -O2 -fno-strength-reduce -fno-unroll-loops"
  fi

  # Force no alignment to save space on i386.
  if test "x$host_cpu" = xi386; then
    AC_CACHE_CHECK([whether -falign-loops works], [falign_loop_flag], [
      CFLAGS="-falign-loops=1"
      AC_TRY_COMPILE(, , [falign_loop_flag=yes], [falign_loop_flag=no])
    ])

    if test "x$falign_loop_flag" = xyes; then
      tmp_CFLAGS="$tmp_CFLAGS -falign-jumps=1 -falign-loops=1 -falign-functions=1"
    else
      tmp_CFLAGS="$tmp_CFLAGS -malign-jumps=1 -malign-loops=1 -malign-functions=1"
    fi
  fi

  if test "x$amd64" = x1; then
    tmp_CFLAGS="$tmp_CFLAGS -m32"
  fi

  CFLAGS="$tmp_CFLAGS"
fi
AC_SUBST(CFLAGS)

# Defined in aclocal.m4.
grub_ASM_USCORE
if test "x$host_cpu" = xi386; then
  grub_CHECK_START_SYMBOL
  grub_CHECK_BSS_START_SYMBOL
  grub_CHECK_END_SYMBOL
fi

if test "x$host_cpu" = xi386; then
  grub_I386_ASM_PREFIX_REQUIREMENT
  grub_I386_ASM_ADDR32
  grub_I386_ASM_ABSOLUTE_WITHOUT_ASTERISK
  grub_I386_CHECK_REGPARM_BUG
else
  AC_DEFINE([NESTED_FUNC_ATTR], [], [Catch gcc bug])
fi

AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_CHECK_TOOL(OBJCOPY, objcopy)
grub_PROG_OBJCOPY_ABSOLUTE
AC_CHECK_TOOL(STRIP, strip)
AC_CHECK_TOOL(NM, nm)
AC_CHECK_TOOL(LD, ld)

# This is not a "must".
AC_PATH_PROG(RUBY, ruby)

# For cross-compiling.
if test "x$build" != "x$host"; then
  AC_CHECK_PROGS(BUILD_CC, [gcc egcs cc],
		 [AC_MSG_ERROR([none of gcc, egcs and cc is found. set BUILD_CC manually.])])
else
  BUILD_CC="$CC"
  AC_SUBST(BUILD_CC)
fi

# Test the C compiler for the build environment.
tmp_CC="$CC"
tmp_CFLAGS="$CFLAGS"
tmp_LDFLAGS="$LDFLAGS"
tmp_CPPFLAGS="$CPPFLAGS"
CC="$BUILD_CC"
CFLAGS="$BUILD_CFLAGS"
CPPFLAGS="$BUILD_CPPFLAGS"
LDFLAGS="$BUILD_LDDFLAGS"

# Identify characteristics of the build architecture.
AC_C_BIGENDIAN
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(long)

# Check LZO when compiling for the i386.
if test "x$host_cpu" = xi386; then
  # There are three possibilities. LZO version 2 installed with the name
  # liblzo2, with the name liblzo, and LZO version 1.
  AC_CHECK_LIB(lzo2, __lzo_init_v2, [LIBLZO="-llzo2"],
    AC_CHECK_LIB(lzo, __lzo_init_v2, [LIBLZO="-llzo"],
      AC_CHECK_LIB(lzo, __lzo_init2, [LIBLZO="-llzo"],
	       AC_MSG_ERROR([LZO library version 1.02 or later is required]))))
  AC_SUBST(LIBLZO)
  LIBS="$LIBS $LIBLZO"
  AC_CHECK_FUNC(lzo1x_999_compress, , 
	        [AC_MSG_ERROR([LZO1X-999 must be enabled])])

  # LZO version 2 uses lzo/lzo1x.h, while LZO version 1 uses lzo1x.h.
  AC_CHECK_HEADERS(lzo/lzo1x.h lzo1x.h)
fi

# Check for curses.
AC_CHECK_LIB(ncurses, wgetch, [LIBCURSES="-lncurses"],
  [AC_CHECK_LIB(curses, wgetch, [LIBCURSES="-lcurses"])])
AC_SUBST(LIBCURSES)

# Check for headers.
AC_CHECK_HEADERS(ncurses/curses.h ncurses.h curses.h)

CC="$tmp_CC"
CFLAGS="$tmp_CFLAGS"
CPPFLAGS="$tmp_CPPFLAGS"

# Output files.
AC_CONFIG_LINKS([include/grub/cpu:include/grub/$host_cpu
	include/grub/machine:include/grub/$host_cpu/$host_vendor])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_OUTPUT
