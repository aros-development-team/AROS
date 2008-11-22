# Process this file with autoconf to produce a configure script.

# Copyright (C) 2002,2003,2004,2005,2006,2007,2008  Free Software Foundation, Inc.
#
# This configure.ac is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

dnl This configure script is complicated, because GRUB needs to deal
dnl with three potentially different types:
dnl
dnl   build  -- the environment for building GRUB
dnl   host   -- the environment for running utilities
dnl   target -- the environment for running GRUB
dnl
dnl In addition, GRUB needs to deal with a platform specification
dnl which specifies the system running GRUB, such as firmware.
dnl This is necessary because the target type in autoconf does not
dnl describe such a system very well.
dnl
dnl The current strategy is to use variables with no prefix (such as
dnl CC, CFLAGS, etc.) for the host type as well as the build type,
dnl because GRUB does not need to use those variables for the build
dnl type, so there is no conflict. Variables with the prefix "TARGET_"
dnl (such as TARGET_CC, TARGET_CFLAGS, etc.) are used for the target
dnl type.


AC_INIT([GRUB],[1.96],[bug-grub@gnu.org])
AC_PREREQ(2.59)
AC_CONFIG_SRCDIR([include/grub/dl.h])
AC_CONFIG_HEADER([config.h])

# Checks for host and target systems.
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

# Program name transformations
AC_ARG_PROGRAM

case "$host_cpu" in
  powerpc64) host_m32=1 ;;
esac

case "$target_cpu" in
  i[[3456]]86) target_cpu=i386 ;;
esac

# Specify the platform (such as firmware).
AC_ARG_WITH([platform],
            AS_HELP_STRING([--with-platform=PLATFORM],
                           [select the host platform [[guessed]]]))

# Guess the platform if not specified.
if test "x$with_platform" = x; then
  case "$target_cpu"-"$target_vendor" in
    i386-apple) platform=efi ;;
    i386-*) platform=pc ;;
    x86_64-apple) platform=efi ;;
    x86_64-*) platform=pc ;;
    powerpc-*) platform=ieee1275 ;;
    powerpc64-*) platform=ieee1275 ;;
    sparc64-*) platform=ieee1275 ;;
    *) AC_MSG_ERROR([unsupported CPU: "$target_cpu"]) ;;
  esac
else
  platform="$with_platform"
fi

# Adjust CPU unless target was explicitly specified.
if test -z "$target_alias"; then
  case "$target_cpu"-"$platform" in
    x86_64-efi) ;;
    x86_64-*) target_cpu=i386 ;;
    powerpc64-ieee1275) target_cpu=powerpc ;;
  esac
fi

# Check if the platform is supported, make final adjustments.
case "$target_cpu"-"$platform" in
  i386-efi) ;;
  x86_64-efi) ;;
  i386-pc) ;;
  i386-coreboot) ;;
  i386-linuxbios) platform=coreboot ;;
  i386-ieee1275) ;;
  powerpc-ieee1275) ;;
  sparc64-ieee1275) ;;
  *) AC_MSG_ERROR([platform "$platform" is not supported for target CPU "$target_cpu"]) ;;
esac

case "$target_cpu" in
  i386 | powerpc) target_m32=1 ;;
  x86_64 | sparc64) target_m64=1 ;;
esac

case "$host_os" in
  mingw32) host_os=cygwin ;;
esac

case "$target_os" in
  mingw32) target_os=cygwin ;;
esac

AC_SUBST(target_cpu)
AC_SUBST(target_os)
AC_SUBST(platform)

#
# Checks for build programs.
#

# Although cmp is listed in the GNU Coding Standards as a command which
# can used directly, OpenBSD lacks cmp in the default installation.
AC_CHECK_PROGS([CMP], [cmp])
if test "x$CMP" = x; then
  AC_MSG_ERROR([cmp is not found])
fi

AC_CHECK_PROGS([YACC], [bison])
if test "x$YACC" = x; then
  AC_MSG_ERROR([bison is not found])
fi

for file in /usr/share/unifont/unifont.hex ; do
  if test -e $file ; then
    AC_SUBST([UNIFONT_HEX], [$file])
    break
  fi
done

AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_MAKE_SET

# These are not a "must".
AC_PATH_PROG(RUBY, ruby)
AC_PATH_PROG(HELP2MAN, help2man)

#
# Checks for host programs.
#

AC_PROG_CC
# Must be GCC.
test "x$GCC" = xyes || AC_MSG_ERROR([GCC is required])

AC_GNU_SOURCE
AC_SYS_LARGEFILE

# Identify characteristics of the host architecture.
AC_C_BIGENDIAN
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(long)

if test "x$host_m32" = x1; then
  # Force 32-bit mode.
  CFLAGS="$CFLAGS -m32"
  LDFLAGS="$LDFLAGS -m32"
fi

# Check LZO when compiling for the i386-pc.
if test "$target_cpu"-"$platform" = i386-pc; then
  AC_ARG_ENABLE([lzo],
	      [AS_HELP_STRING([--enable-lzo],
                             [use lzo to compress kernel (default is lzma)])])
  [if [ x"$enable_lzo" = xyes ]; then
    # There are three possibilities. LZO version 2 installed with the name
    # liblzo2, with the name liblzo, and LZO version 1.]
    AC_DEFINE([ENABLE_LZO], [1], [Use lzo compression])
    AC_CHECK_LIB([lzo2], [__lzo_init_v2], [LIBLZO="-llzo2"],
      [AC_CHECK_LIB([lzo], [__lzo_init_v2], [LIBLZO="-llzo"],
        [AC_CHECK_LIB([lzo], [__lzo_init2], [LIBLZO="-llzo"],
	       [AC_MSG_ERROR([LZO library version 1.02 or later is required])])])])
    AC_SUBST([LIBLZO])
    [LIBS="$LIBS $LIBLZO"]
    AC_CHECK_FUNC([lzo1x_999_compress], ,
	        [AC_MSG_ERROR([LZO1X-999 must be enabled])])

    [# LZO version 2 uses lzo/lzo1x.h, while LZO version 1 uses lzo1x.h.]
    AC_CHECK_HEADERS([lzo/lzo1x.h lzo1x.h])
  [else]
    AC_DEFINE([ENABLE_LZMA], [1], [Use lzma compression])
  [fi]
  AC_SUBST([enable_lzo])
fi

# Check for functions.
AC_CHECK_FUNCS(posix_memalign memalign asprintf)

#
# Check for target programs.
#


# Use linker script if present, otherwise use builtin -N script.
AC_MSG_CHECKING([for option to link raw image])
if test -f "${srcdir}/conf/${target_cpu}-${platform}-${target_os}-img-ld.sc"; then
  TARGET_IMG_LDSCRIPT='$(top_srcdir)'"/conf/${target_cpu}-${platform}-${target_os}-img-ld.sc"
  TARGET_IMG_LDFLAGS="-Wl,-T${TARGET_IMG_LDSCRIPT}"
  TARGET_IMG_LDFLAGS_AC="-Wl,-T${srcdir}/conf/${target_cpu}-${platform}-${target_os}-img-ld.sc"
else
  TARGET_IMG_LDSCRIPT=
  TARGET_IMG_LDFLAGS='-Wl,-N'
  TARGET_IMG_LDFLAGS_AC='-Wl,-N'
fi
AC_SUBST(TARGET_IMG_LDSCRIPT)
AC_SUBST(TARGET_IMG_LDFLAGS)
AC_MSG_RESULT([$TARGET_IMG_LDFLAGS_AC])

# For platforms where ELF is not the default link format.
AC_MSG_CHECKING([for command to convert module to ELF format])
case "${host_os}:${target_os}" in
  cygwin:cygwin) TARGET_OBJ2ELF='grub-pe2elf' ;;
  *) ;;
esac
AC_SUBST(TARGET_OBJ2ELF)
AC_MSG_RESULT([$TARGET_OBJ2ELF])

# For cross-compiling.
if test "x$target" != "x$host"; then
  # XXX this depends on the implementation of autoconf!
  tmp_ac_tool_prefix="$ac_tool_prefix"
  ac_tool_prefix=$target_alias-

  AC_CHECK_TOOLS(TARGET_CC, [gcc egcs cc],
                 [AC_MSG_ERROR([none of gcc, egcs and cc is found. set TARGET_CC manually.])])
  AC_CHECK_TOOL(OBJCOPY, objcopy)
  AC_CHECK_TOOL(STRIP, strip)
  AC_CHECK_TOOL(NM, nm)

  ac_tool_prefix="$tmp_ac_tool_prefix"
else
  if test "x$TARGET_CC" = x; then
    TARGET_CC=$CC
  fi
  AC_CHECK_TOOL(OBJCOPY, objcopy)
  AC_CHECK_TOOL(STRIP, strip)
  AC_CHECK_TOOL(NM, nm)
fi
AC_SUBST(TARGET_CC)


# Test the C compiler for the target environment.
tmp_CC="$CC"
tmp_CFLAGS="$CFLAGS"
tmp_LDFLAGS="$LDFLAGS"
tmp_CPPFLAGS="$CPPFLAGS"
tmp_LIBS="$LIBS"
CC="$TARGET_CC"
CFLAGS="$TARGET_CFLAGS"
CPPFLAGS="$TARGET_CPPFLAGS"
LDFLAGS="$TARGET_LDFLAGS"
LIBS=""

if test "x$TARGET_CFLAGS" = x; then
  # debug flags.
  TARGET_CFLAGS="-Wall -W -Wshadow -Wpointer-arith -Wmissing-prototypes \
                 -Wundef -Wstrict-prototypes -g"

  # optimization flags.
  AC_CACHE_CHECK([whether optimization for size works], grub_cv_cc_Os, [
    CFLAGS=-Os
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
		      [grub_cv_cc_Os=yes],
		      [grub_cv_cc_Os=no])
  ])
  if test "x$grub_cv_cc_Os" = xyes; then
    TARGET_CFLAGS="$TARGET_CFLAGS -Os"
  else
    TARGET_CFLAGS="$TARGET_CFLAGS -O2 -fno-strength-reduce -fno-unroll-loops"
  fi

  # Force no alignment to save space on i386.
  if test "x$target_cpu" = xi386; then
    AC_CACHE_CHECK([whether -falign-loops works], [grub_cv_cc_falign_loop], [
      CFLAGS="-falign-loops=1"
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
		        [grub_cv_cc_falign_loop=yes],
			[grub_cv_cc_falign_loop=no])
    ])

    if test "x$grub_cv_cc_falign_loop" = xyes; then
      TARGET_CFLAGS="$TARGET_CFLAGS -falign-jumps=1 -falign-loops=1 -falign-functions=1"
    else
      TARGET_CFLAGS="$TARGET_CFLAGS -malign-jumps=1 -malign-loops=1 -malign-functions=1"
    fi
  fi
fi

if test "x$target_m32" = x1; then
  # Force 32-bit mode.
  TARGET_CFLAGS="$TARGET_CFLAGS -m32"
  TARGET_LDFLAGS="$TARGET_LDFLAGS -m32"
fi

if test "x$target_m64" = x1; then
  # Force 64-bit mode.
  TARGET_CFLAGS="$TARGET_CFLAGS -m64"
  TARGET_LDFLAGS="$TARGET_LDFLAGS -m64"
fi

#
# Compiler features.
#

# Need __enable_execute_stack() for nested function trampolines?
grub_CHECK_ENABLE_EXECUTE_STACK

# Smashing stack protector.
grub_CHECK_STACK_PROTECTOR
# Need that, because some distributions ship compilers that include
# `-fstack-protector' in the default specs.
if test "x$ssp_possible" = xyes; then
  TARGET_CFLAGS="$TARGET_CFLAGS -fno-stack-protector"
fi
grub_CHECK_STACK_ARG_PROBE
# Cygwin's GCC uses alloca() to probe the stackframe on static
# stack allocations above some threshold.
if test x"$sap_possible" = xyes; then
  TARGET_CFLAGS="$TARGET_CFLAGS -mno-stack-arg-probe"
fi

AC_SUBST(TARGET_CFLAGS)
AC_SUBST(TARGET_CPPFLAGS)
AC_SUBST(TARGET_LDFLAGS)
AC_SUBST(MODULE_LDFLAGS)

# Set them to their new values for the tests below.
CC="$TARGET_CC"
CFLAGS="$TARGET_CFLAGS"
CPPFLAGS="$TARGET_CPPFLAGS"
LDFLAGS="$TARGET_LDFLAGS"

# Defined in aclocal.m4.
grub_PROG_TARGET_CC
grub_PROG_OBJCOPY_ABSOLUTE
grub_PROG_LD_BUILD_ID_NONE
grub_ASM_USCORE
if test "x$target_cpu" = xi386; then
  if test ! -z "$TARGET_IMG_LDSCRIPT"; then
    # Check symbols provided by linker script.
    CFLAGS="$TARGET_CFLAGS -nostdlib $TARGET_IMG_LDFLAGS_AC -Wl,-Ttext,8000,--defsym,___main=0x8100"
  fi
  if test "x$platform" = xpc; then
    grub_CHECK_START_SYMBOL
    grub_CHECK_BSS_START_SYMBOL
    grub_CHECK_END_SYMBOL
  fi
  CFLAGS="$TARGET_CFLAGS"
  grub_I386_ASM_PREFIX_REQUIREMENT
  grub_I386_ASM_ADDR32
  grub_I386_ASM_ABSOLUTE_WITHOUT_ASTERISK
  grub_I386_CHECK_REGPARM_BUG
else
  AC_DEFINE([NESTED_FUNC_ATTR], [], [Catch gcc bug])
fi

# Restore the flags.
CC="$tmp_CC"
CFLAGS="$tmp_CFLAGS"
CPPFLAGS="$tmp_CPPFLAGS"
LDFLAGS="$tmp_LDFLAGS"
LIBS="$tmp_LIBS"

#
# Check for options.
#

# Memory manager debugging.
AC_ARG_ENABLE([mm-debug], 
	      AS_HELP_STRING([--enable-mm-debug],
                             [include memory manager debugging]),
              [AC_DEFINE([MM_DEBUG], [1],
                         [Define to 1 if you enable memory manager debugging.])])

AC_ARG_ENABLE([grub-emu],
	      [AS_HELP_STRING([--enable-grub-emu],
                             [build and install the `grub-emu' debugging utility])])
[if [ x"$enable_grub_emu" = xyes ]; then
  # Check for curses libraries.]
  AC_CHECK_LIB([ncurses], [wgetch], [LIBCURSES="-lncurses"],
    [AC_CHECK_LIB([curses], [wgetch], [LIBCURSES="-lcurses"],
      [AC_MSG_ERROR([(n)curses libraries are required to build `grub-emu'])])])
  AC_SUBST([LIBCURSES])

  [# Check for headers.]
  AC_CHECK_HEADERS([ncurses/curses.h], [],
    [AC_CHECK_HEADERS([ncurses.h], [],
      [AC_CHECK_HEADERS([curses.h], [],
	[AC_MSG_ERROR([(n)curses header files are required to build `grub-emu'])])])])
[fi]
AC_SUBST([enable_grub_emu])

AC_ARG_ENABLE([grub-fstest],
	      [AS_HELP_STRING([--enable-grub-fstest],
                             [build and install the `grub-fstest' debugging utility])])
AC_SUBST([enable_grub_fstest])

AC_ARG_ENABLE([grub-pe2elf],
	      [AS_HELP_STRING([--enable-grub-pe2elf],
                             [build and install the `grub-pe2elf' conversion utility])])
AC_SUBST([enable_grub_pe2elf])

# Output files.
AC_CONFIG_LINKS([include/grub/cpu:include/grub/$target_cpu
	include/grub/machine:include/grub/$target_cpu/$platform])
AC_CONFIG_FILES([Makefile gensymlist.sh genkernsyms.sh])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_OUTPUT
