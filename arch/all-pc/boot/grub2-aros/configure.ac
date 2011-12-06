# -*- autoconf -*-

# Process this file with autoconf to produce a configure script.

# Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

AC_INIT([GRUB],[1.99~beta0],[bug-grub@gnu.org])

AC_CONFIG_AUX_DIR([build-aux])

# We don't want -g -O2 by default in CFLAGS
: ${CFLAGS=""}

# Checks for host and target systems.
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE()
AC_PREREQ(2.60)
AC_CONFIG_SRCDIR([include/grub/dl.h])
AC_CONFIG_HEADER([config-util.h])

# Program name transformations
AC_ARG_PROGRAM

# Optimization flag.  Allow user to override.
if test "x$TARGET_CFLAGS" = x; then
  TARGET_CFLAGS="$TARGET_CFLAGS -Os"
fi

# Default HOST_CPPFLAGS
HOST_CPPFLAGS="$HOST_CPPFLAGS -Wall -W"
HOST_CPPFLAGS="$HOST_CPPFLAGS -I\$(top_builddir)/include"
HOST_CPPFLAGS="$HOST_CPPFLAGS -DGRUB_UTIL=1"
HOST_CPPFLAGS="$HOST_CPPFLAGS -DGRUB_LIBDIR=\\\"\$(pkglibdir)\\\""
HOST_CPPFLAGS="$HOST_CPPFLAGS -DLOCALEDIR=\\\"\$(localedir)\\\""

TARGET_CPPFLAGS="$TARGET_CPPFLAGS -Wall -W"
TARGET_CPPFLAGS="$TARGET_CPPFLAGS -I\$(top_srcdir)/include"
TARGET_CPPFLAGS="$TARGET_CPPFLAGS -I\$(top_builddir)/include"

case "$target_cpu" in
  i[[3456]]86)	target_cpu=i386 ;;
  amd64)	target_cpu=x86_64 ;;
  sparc)	target_cpu=sparc64 ;;
  mipsel|mips64el)
                target_cpu=mips;
		machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_CPU_MIPSEL=1";
		;;
  mips|mips64)
                target_cpu=mips;
		machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_CPU_MIPS=1";
		;;
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
    mips-*) platform=yeeloong ;;
    *) AC_MSG_ERROR([unsupported CPU: "$target_cpu"]) ;;
  esac
else
  platform="$with_platform"
fi

case "$target_cpu"-"$platform" in
  x86_64-efi) ;;
  x86_64-emu) ;;
  x86_64-*) target_cpu=i386 ;;
  powerpc64-ieee1275) target_cpu=powerpc ;;
esac

# Check if the platform is supported, make final adjustments.
case "$target_cpu"-"$platform" in
  i386-efi) ;;
  x86_64-efi) ;;
  i386-pc) ;;
  i386-multiboot) ;;
  i386-coreboot) ;;
  i386-linuxbios) platform=coreboot ;;
  i386-ieee1275) ;;
  i386-qemu) ;;
  powerpc-ieee1275) ;;
  sparc64-ieee1275) ;;
  mips-qemu-mips) ;;
  mips-yeeloong) ;;
  *-emu) ;;
  *) AC_MSG_ERROR([platform "$platform" is not supported for target CPU "$target_cpu"]) ;;
esac

case "$target_cpu" in
  i386 | powerpc) target_m32=1 ;;
  x86_64 | sparc64) target_m64=1 ;;
esac

dnl AROS FIX: Originally $host_os is used here, but this causes problems with AROS build system
dnl because it specifies --target, causing a ELF cross-compiler to be used for building standalone
dnl grub-core. Using $host_os here caused attempt to use pe2elf on ELF objects.
dnl The same affects one more 'cygwin' appearance below (line 462).
case "$target_os" in
  mingw32*) target_os=cygwin ;;
esac

# This normalizes the names, and creates a new variable ("host_kernel")
# while at it, since the mapping is not always 1:1 (e.g. different OSes
# using the same kernel type).
case "$host_os" in
  gnu*)				host_kernel=hurd ;;
  linux*)			host_kernel=linux ;;
  freebsd* | kfreebsd*-gnu)	host_kernel=kfreebsd ;;
  netbsd*)			host_kernel=netbsd ;;
  cygwin)			host_kernel=windows ;;
esac

case "$platform" in
  coreboot)	machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE_COREBOOT=1" ;;
  multiboot)	machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE_MULTIBOOT=1" ;;
  efi)		machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE_EFI=1" ;;
  ieee1275)	machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE_IEEE1275=1" ;;
  qemu)		machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE_QEMU=1" ;;
  pc)		machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE_PCBIOS=1" ;;
  emu)		machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE_EMU=1" ;;
  yeeloong)	machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE_MIPS_YEELOONG=1 -DGRUB_MACHINE_MIPS_BONITO=1" ;;
  qemu-mips)	machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE_MIPS_QEMU_MIPS=1 -DGRUB_MACHINE_MIPS_BONITO=1" ;;
esac
case "$target_cpu" in
  mips)      	machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE_MIPS=1" ;;
  sparc64)      machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE_SPARC64=1" ;;
esac
machine_CPPFLAGS="$machine_CPPFLAGS -DGRUB_MACHINE=`echo ${target_cpu}_$platform | sed y,abcdefghijklmnopqrstuvwxyz,ABCDEFGHIJKLMNOPQRSTUVWXYZ,`"

HOST_CPPFLAGS="$HOST_CPPFLAGS $machine_CPPFLAGS"
TARGET_CPPFLAGS="$TARGET_CPPFLAGS $machine_CPPFLAGS"

AC_SUBST(host_cpu)
AC_SUBST(host_os)
AC_SUBST(host_kernel)

AC_SUBST(target_cpu)
AC_SUBST(platform)

# Define default variables
case "$host_os" in
  netbsd* | openbsd*)
        # Because /boot is used for the boot block in NetBSD and OpenBSD,
        bootdirname=''      ;;
  *)    bootdirname='boot'  ;;
esac
bootdirname=`echo "$bootdirname" | sed "$program_transform_name"`
AC_SUBST(bootdirname)
AC_DEFINE_UNQUOTED(GRUB_BOOT_DIR_NAME, "$bootdirname",
    [Default boot directory name]")

grubdirname=`echo "$PACKAGE" | sed "$program_transform_name"`
AC_SUBST(grubdirname)
AC_DEFINE_UNQUOTED(GRUB_DIR_NAME, "$grubdirname",
    [Default grub directory name])

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

FONT_SOURCE=

for ext in pcf pcf.gz bdf bdf.gz ttf ttf.gz; do
  for dir in . /usr/src /usr/share/fonts/X11/misc /usr/share/fonts/unifont; do
    if test -f "$dir/unifont.$ext"; then
      FONT_SOURCE="$dir/unifont.$ext"
      break 2
    fi
  done
done

if test "x$FONT_SOURCE" = x && ( test "x$platform" = xqemu || test "x$platform" = xyeeloong ); then
   AC_MSG_ERROR([qemu and yeeloong ports need unifont])
fi

AC_SUBST([FONT_SOURCE])

AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_LN_S

if test "x$LEX" = "x:"; then
  AC_MSG_ERROR([flex is not found])
else
  version=`$LEX --version | $AWK '{ split($NF,x,"."); print x[[1]]*10000+x[[2]]*100+x[[3]]; }'`
  if test -n "$version" -a "$version" -ge 20535; then
    :
  else
    AC_MSG_ERROR([flex is too old. GRUB requires 2.5.35 or above])
  fi
fi

# These are not a "must".
AC_PATH_PROG(MAKEINFO, makeinfo)

#
# Checks for host programs.
#

AC_PROG_CC
gl_EARLY
AM_PROG_CC_C_O
AM_PROG_AS

# Must be GCC.
test "x$GCC" = xyes || AC_MSG_ERROR([GCC is required])

AC_GNU_SOURCE
AM_GNU_GETTEXT([external])
AC_SYS_LARGEFILE

# Identify characteristics of the host architecture.
AC_C_BIGENDIAN
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(long)

grub_apple_cc
if test x$grub_cv_apple_cc = xyes ; then
  HOST_CPPFLAGS="$HOST_CPPFLAGS -DAPPLE_CC=1"
  HOST_CFLAGS="$HOST_CFLAGS -fnested-functions"
fi

if test "x$cross_compiling" = xyes; then
  AC_MSG_WARN([cannot generate manual pages while cross compiling])
else
  AC_PATH_PROG(HELP2MAN, help2man)
fi

# Check for functions and headers.
AC_CHECK_FUNCS(posix_memalign memalign asprintf vasprintf getextmntent)
AC_CHECK_HEADERS(libzfs.h libnvpair.h sys/param.h sys/mount.h sys/mnttab.h sys/mkdev.h)

AC_CHECK_MEMBERS([struct statfs.f_fstypename],,,[$ac_includes_default
#include <sys/param.h>
#include <sys/mount.h>])

AC_CHECK_MEMBERS([struct statfs.f_mntfromname],,,[$ac_includes_default
#include <sys/param.h>
#include <sys/mount.h>])

# For opendisk() and getrawpartition() on NetBSD.
# Used in util/deviceiter.c and in util/hostdisk.c.
AC_CHECK_HEADER([util.h], [
  AC_CHECK_LIB([util], [opendisk], [
    LIBUTIL="-lutil"
    AC_DEFINE(HAVE_OPENDISK, 1, [Define if opendisk() in -lutil can be used])
  ])
  AC_CHECK_LIB([util], [getrawpartition], [
    LIBUTIL="-lutil"
    AC_DEFINE(HAVE_GETRAWPARTITION, 1, [Define if getrawpartition() in -lutil can be used])
  ])
])
AC_SUBST([LIBUTIL])

#
# Check for host and build compilers.
#
HOST_CC=$CC
AC_CHECK_PROGS(BUILD_CC, [gcc egcs cc],
                 [AC_MSG_ERROR([none of gcc, egcs and cc is found. set BUILD_CC manually.])])

# For gnulib.
gl_INIT

#
# Check for target programs.
#

# Find tools for the target.
if test "x$target_alias" != x && test "x$host_alias" != "x$target_alias"; then
  tmp_ac_tool_prefix="$ac_tool_prefix"
  ac_tool_prefix=$target_alias-

  AC_CHECK_TOOLS(TARGET_CC, [gcc egcs cc],
                 [AC_MSG_ERROR([none of gcc, egcs and cc is found. set TARGET_CC manually.])])
  AC_CHECK_TOOL(OBJCOPY, objcopy)
  AC_CHECK_TOOL(STRIP, strip)
  AC_CHECK_TOOL(NM, nm)
dnl AROS FIX: added TARGET_RANLIB and TARGET_STRIP, used by grub-core/Makefile.am
  AC_CHECK_TOOL(TARGET_RANLIB, ranlib)
  AC_CHECK_TOOL(TARGET_STRIP, strip)

  ac_tool_prefix="$tmp_ac_tool_prefix"
else
  if test "x$TARGET_CC" = x; then
    TARGET_CC=$CC
  fi
  if test "x$TARGET_RANLIB" = x; then
    TARGET_RANLIB=$RANLIB
  fi
  AC_CHECK_TOOL(OBJCOPY, objcopy)
  AC_CHECK_TOOL(STRIP, strip)
  AC_CHECK_TOOL(NM, nm)
  if test "x$TARGET_STRIP" = x; then
    TARGET_STRIP=$STRIP
  fi
fi
AC_SUBST(HOST_CC)
AC_SUBST(BUILD_CC)
AC_SUBST(TARGET_CC)
AC_SUBST(TARGET_RANLIB)
AC_SUBST(TARGET_STRIP)

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

# debug flags.
TARGET_CFLAGS="$TARGET_CFLAGS -Wall -W -Wshadow -Wpointer-arith -Wmissing-prototypes -Wundef -Wstrict-prototypes -g"
TARGET_CCASFLAGS="$TARGET_CCASFLAGS -g"

# Force no alignment to save space on i386.
if test "x$target_cpu" = xi386; then
  AC_CACHE_CHECK([whether -falign-loops works], [grub_cv_cc_falign_loop], [
    CFLAGS="$CFLAGS -falign-loops=1"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
        [grub_cv_cc_falign_loop=yes],
	[grub_cv_cc_falign_loop=no])
  ])

  if test "x$grub_cv_cc_falign_loop" = xyes; then
    TARGET_CFLAGS="$TARGET_CFLAGS -falign-jumps=1 -falign-loops=1 -falign-functions=1"
  else
    TARGET_CFLAGS="$TARGET_CFLAGS -malign-jumps=1 -malign-loops=1 -malign-functions=1"
  fi

  # Some toolchains enable these features by default, but they need
  # registers that aren't set up properly in GRUB.
  TARGET_CFLAGS="$TARGET_CFLAGS -mno-mmx -mno-sse -mno-sse2 -mno-3dnow"
fi

# By default, GCC 4.4 generates .eh_frame sections containing unwind
# information in some cases where it previously did not. GRUB doesn't need
# these and they just use up vital space. Restore the old compiler
# behaviour.
AC_CACHE_CHECK([whether -fno-dwarf2-cfi-asm works], [grub_cv_cc_fno_dwarf2_cfi_asm], [
  SAVE_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -fno-dwarf2-cfi-asm"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
      [grub_cv_cc_fno_dwarf2_cfi_asm=yes],
      [grub_cv_cc_fno_dwarf2_cfi_asm=no])
  CFLAGS="$SAVE_CFLAGS"
])

if test "x$grub_cv_cc_fno_dwarf2_cfi_asm" = xyes; then
  TARGET_CFLAGS="$TARGET_CFLAGS -fno-dwarf2-cfi-asm"
fi

if test "${target_cpu}-${platform}" = mips-yeeloong; then
  AC_CACHE_CHECK([whether -march=loongson2f works], [grub_cv_cc_march_loongson2f], [
    SAVE_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -march=loongson2f"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
        [grub_cv_cc_march_loongson2f=yes],
        [grub_cv_cc_march_loongson2f=no])
    CFLAGS="$SAVE_CFLAGS"
  ])

  if test "x$grub_cv_cc_march_loongson2f" = xyes; then
    TARGET_CFLAGS="$TARGET_CFLAGS -march=loongson2f"
  else
    TARGET_CFLAGS="$TARGET_CFLAGS -march=mips3"
  fi
fi

grub_apple_target_cc
if test x$grub_cv_apple_target_cc = xyes ; then
  TARGET_CPPFLAGS="$TARGET_CPPFLAGS -DAPPLE_CC=1"
  TARGET_CFLAGS="$TARGET_CFLAGS -fnested-functions"

  CFLAGS="$CFLAGS -DAPPLE_CC=1 -fnested-functions"
  TARGET_APPLE_CC=1
  AC_CHECK_PROG([OBJCONV], [objconv], [objconv], [])
  if test "x$OBJCONV" = x ; then
     AC_CHECK_PROG([OBJCONV], [objconv], [./objconv], [], [.])
  fi
  if test "x$OBJCONV" = x ; then
    AC_MSG_ERROR([objconv not found which is required when building with apple compiler])
  fi
  TARGET_IMG_LDSCRIPT=
  TARGET_IMG_CFLAGS="-static"
  TARGET_IMG_LDFLAGS='-nostdlib -static -Wl,-preload -Wl,-segalign,20'
  TARGET_IMG_LDFLAGS_AC='-nostdlib -static -Wl,-preload -Wl,-segalign,20'
  TARGET_IMG_BASE_LDOPT="-Wl,-image_base"
else
  TARGET_APPLE_CC=0
# Use linker script if present, otherwise use builtin -N script.
if test -f "${srcdir}/${grub_coredir}/conf/${target_cpu}-${platform}-${host_os}-img-ld.sc"; then
  TARGET_IMG_LDSCRIPT='$(top_srcdir)'"/${grub_coredir}/conf/${target_cpu}-${platform}-${host_os}-img-ld.sc"
  TARGET_IMG_LDFLAGS="-Wl,-T${TARGET_IMG_LDSCRIPT}"
  TARGET_IMG_LDFLAGS_AC="-Wl,-T${srcdir}/${grub_coredir}/conf/${target_cpu}-${platform}-${host_os}-img-ld.sc"
  TARGET_IMG_BASE_LDOPT="-Wl,-Ttext"
else
  TARGET_IMG_LDSCRIPT=
  TARGET_IMG_LDFLAGS='-Wl,-N'
  TARGET_IMG_LDFLAGS_AC='-Wl,-N'
  TARGET_IMG_BASE_LDOPT="-Wl,-Ttext"
fi
TARGET_IMG_CFLAGS=
fi

# For platforms where ELF is not the default link format.
AC_MSG_CHECKING([for command to convert module to ELF format])
case "${target_os}" in
  cygwin) TARGET_OBJ2ELF='$(grub_utildir)/grub-pe2elf';
# FIXME: put proper test here
  NEED_REGISTER_FRAME_INFO=1
  ;;
  *) NEED_REGISTER_FRAME_INFO=0 ;;
esac
AC_MSG_RESULT([$TARGET_OBJ2ELF])

if test "x$target_m32" = x1; then
  # Force 32-bit mode.
  TARGET_CFLAGS="$TARGET_CFLAGS -m32"
  TARGET_CCASFLAGS="$TARGET_CCASFLAGS -m32"
  TARGET_LDFLAGS="$TARGET_LDFLAGS -m32"
  TARGET_MODULE_FORMAT="elf32"
fi

if test "x$target_m64" = x1; then
  # Force 64-bit mode.
  TARGET_CFLAGS="$TARGET_CFLAGS -m64"
  TARGET_CCASFLAGS="$TARGET_CCASFLAGS -m64"
  TARGET_LDFLAGS="$TARGET_LDFLAGS -m64"
  TARGET_MODULE_FORMAT="elf64"
fi

if test "$target_cpu"-"$platform" = x86_64-efi; then
  # Use large model to support 4G memory
  AC_CACHE_CHECK([whether option -mcmodel=large works], grub_cv_cc_mcmodel, [
    SAVED_CFLAGS=$CFLAGS
    CFLAGS="$CFLAGS -m64 -mcmodel=large"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
		      [grub_cv_cc_mcmodel=yes],
		      [grub_cv_cc_mcmodel=no])
  ])
  if test "x$grub_cv_cc_mcmodel" = xno; then
    AC_MSG_ERROR([-mcmodel=large not supported. Upgrade your gcc.])
  else
    TARGET_CFLAGS="$TARGET_CFLAGS -mcmodel=large"
  fi

  # EFI writes to stack below %rsp, we must not use the red zone
  AC_CACHE_CHECK([whether option -mno-red-zone works], grub_cv_cc_no_red_zone, [
    CFLAGS="$CFLAGS -m64 -mno-red-zone"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
		      [grub_cv_cc_no_red_zone=yes],
		      [grub_cv_cc_no_red_zone=no])
  ])
  if test "x$grub_cv_cc_no_red_zone" = xno; then
    AC_MSG_ERROR([-mno-red-zone not supported, upgrade your gcc])
  fi

  TARGET_CFLAGS="$TARGET_CFLAGS -mno-red-zone"
fi

#
# Compiler features.
#

# Need __enable_execute_stack() for nested function trampolines?
grub_CHECK_ENABLE_EXECUTE_STACK

# Position independent executable.
grub_CHECK_PIE
[# Need that, because some distributions ship compilers that include
# `-fPIE' in the default specs.
if [ x"$pie_possible" = xyes ]; then
  TARGET_CFLAGS="$TARGET_CFLAGS -fno-PIE"
fi]

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

AC_ARG_ENABLE([werror],
	      [AS_HELP_STRING([--disable-werror],
                             [do not use -Werror when building GRUB])])
if test x"$enable_werror" != xno ; then
  TARGET_CFLAGS="$TARGET_CFLAGS -Werror"
fi

TARGET_CPP="$TARGET_CC -E"
TARGET_CCAS=$TARGET_CC

AC_SUBST(OBJCONV)
AC_SUBST(TARGET_CPP)
AC_SUBST(TARGET_CCAS)
AC_SUBST(TARGET_OBJ2ELF)
AC_SUBST(TARGET_APPLE_CC)
AC_SUBST(TARGET_MODULE_FORMAT)

AC_SUBST(TARGET_CFLAGS)
AC_SUBST(TARGET_LDFLAGS)
AC_SUBST(TARGET_CPPFLAGS)
AC_SUBST(TARGET_CCASFLAGS)

AC_SUBST(TARGET_IMG_LDSCRIPT)
AC_SUBST(TARGET_IMG_LDFLAGS)
AC_SUBST(TARGET_IMG_CFLAGS)
AC_SUBST(TARGET_IMG_BASE_LDOPT)

AC_SUBST(HOST_CFLAGS)
AC_SUBST(HOST_LDFLAGS)
AC_SUBST(HOST_CPPFLAGS)
AC_SUBST(HOST_CCASFLAGS)

# Set them to their new values for the tests below.
CC="$TARGET_CC"
if test "x$TARGET_APPLE_CC" = x1 ; then
CFLAGS="$TARGET_CFLAGS -nostdlib -Wno-error"
else
CFLAGS="$TARGET_CFLAGS -nostdlib -Wl,--defsym,___main=0x8100 -Wno-error"
fi
CPPFLAGS="$TARGET_CPPFLAGS"
if test x$target_cpu = xi386 || test x$target_cpu = xx86_64 ; then
LIBS=
else
LIBS=-lgcc
fi

grub_ASM_USCORE
if test x$grub_cv_asm_uscore = xyes; then
CFLAGS="$CFLAGS -Wl,--defsym,_abort=_main"
else
CFLAGS="$CFLAGS -Wl,--defsym,abort=main"
fi

# Check for libgcc symbols
AC_CHECK_FUNCS(__bswapsi2 __bswapdi2 __ashldi3 __ashrdi3 __lshrdi3 __trampoline_setup __ucmpdi2 _restgpr_14_x)

if test "x$TARGET_APPLE_CC" = x1 ; then
CFLAGS="$TARGET_CFLAGS -nostdlib"
else
CFLAGS="$TARGET_CFLAGS -nostdlib -Wl,--defsym,___main=0x8100"
fi
LIBS=""

# Defined in aclocal.m4.
grub_PROG_TARGET_CC
if test "x$TARGET_APPLE_CC" != x1 ; then
grub_PROG_OBJCOPY_ABSOLUTE
fi
grub_PROG_LD_BUILD_ID_NONE
if test "x$target_cpu" = xi386; then
  if test "$platform" != emu && test "x$TARGET_APPLE_CC" != x1 ; then
    if test ! -z "$TARGET_IMG_LDSCRIPT"; then
      # Check symbols provided by linker script.
      CFLAGS="$TARGET_CFLAGS -nostdlib ${TARGET_IMG_LDFLAGS_AC} ${TARGET_IMG_BASE_LDOPT},8000 -Wl,--defsym,___main=0x8100"
    fi
    grub_CHECK_BSS_START_SYMBOL
    grub_CHECK_END_SYMBOL
  fi
  CFLAGS="$TARGET_CFLAGS"
  grub_I386_ASM_PREFIX_REQUIREMENT
  grub_I386_ASM_ADDR32
fi

AC_ARG_ENABLE([efiemu],
	      [AS_HELP_STRING([--enable-efiemu],
                             [build and install the efiemu runtimes (default=guessed)])])
if test x"$enable_efiemu" = xno ; then
  efiemu_excuse="explicitly disabled"
fi
if test x"$target_cpu" != xi386 ; then
  efiemu_excuse="only available on i386"
fi
if test x"$platform" = xefi ; then
  efiemu_excuse="not available on efi"
fi
if test x"$efiemu_excuse" = x ; then
  AC_CACHE_CHECK([whether options required for efiemu work], grub_cv_cc_efiemu, [
    CFLAGS="$CFLAGS -m64 -mcmodel=large -mno-red-zone -nostdlib"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
		      [grub_cv_cc_efiemu=yes],
		      [grub_cv_cc_efiemu=no])
  ])
  if test x$grub_cv_cc_efiemu = xno; then
     efiemu_excuse="cannot compile with -m64 -mcmodel=large -mno-red-zone -nostdlib"
  fi
fi
if test x"$enable_efiemu" = xyes && test x"$efiemu_excuse" != x ; then
  AC_MSG_ERROR([efiemu runtime was explicitly requested but can't be compiled])
fi
if test x"$efiemu_excuse" = x ; then
enable_efiemu=yes
else
enable_efiemu=no
fi
AC_SUBST([enable_efiemu])

if test "$platform" != emu; then
AC_CACHE_CHECK([whether -nostdinc -isystem works], [grub_cv_cc_isystem], [
  SAVED_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$TARGET_CPPFLAGS -nostdinc -isystem `$TARGET_CC -print-file-name=include`"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdarg.h>
int va_arg_func (int fixed, va_list args);]], [[]])],
      [grub_cv_cc_isystem=yes],
      [grub_cv_cc_isystem=no])
  CPPFLAGS="$SAVED_CPPFLAGS"
])

if test x"$grub_cv_cc_isystem" = xyes ; then
  TARGET_CPPFLAGS="$TARGET_CPPFLAGS -nostdinc -isystem `$TARGET_CC -print-file-name=include`"
fi
fi

AC_CACHE_CHECK([whether -Wno-trampolines work], [grub_cv_cc_wnotrampolines], [
  SAVED_CFLAGS="$CFLAGS"
  # Test for -Wtrampolines rather than -Wno-trampolines to reduce confusion
  # in the event of later failures (since -Wno-* is always accepted, but
  # produces a diagnostic if something else is wrong).
  CFLAGS="$TARGET_CFLAGS -Wtrampolines"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdarg.h>
int va_arg_func (int fixed, va_list args);]], [[]])],
      [grub_cv_cc_wnotrampolines=yes],
      [grub_cv_cc_wnotrampolines=no])
  CFLAGS="$SAVED_CFLAGS"
])

if test x"$grub_cv_cc_wnotrampolines" = xyes ; then
  TARGET_CFLAGS="$TARGET_CFLAGS -Wno-trampolines"
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

AC_ARG_ENABLE([grub-emu-usb],
	      [AS_HELP_STRING([--enable-grub-emu-usb],
                             [build and install the `grub-emu' debugging utility with USB support (default=guessed)])])

AC_ARG_ENABLE([grub-emu-sdl],
	      [AS_HELP_STRING([--enable-grub-emu-sdl],
                             [build and install the `grub-emu' debugging utility with SDL support (default=guessed)])])

AC_ARG_ENABLE([grub-emu-pci],
	      [AS_HELP_STRING([--enable-grub-emu-pci],
                             [build and install the `grub-emu' debugging utility with PCI support (potentially dangerous) (default=no)])])

if test "$platform" = emu; then
  missing_ncurses=
[# Check for curses libraries.]
  AC_CHECK_LIB([ncurses], [wgetch], [LIBCURSES="-lncurses"],
    [AC_CHECK_LIB([curses], [wgetch], [LIBCURSES="-lcurses"],
      [missing_ncurses=[true]])])
  AC_SUBST([LIBCURSES])
[if [ x"$missing_ncurses" = x ]; then ]
  [# Check for headers.]
  AC_CHECK_HEADERS([ncurses/curses.h], [],
    [AC_CHECK_HEADERS([ncurses.h], [],
      [AC_CHECK_HEADERS([curses.h], [],
	[missing_ncurses=[true]])])])
[fi]
if test x"$missing_ncurses" = xtrue ; then
  AC_MSG_ERROR([grub-emu can't be compiled without ncurses])
fi

if test x"$enable_grub_emu_usb" != xyes ; then
   grub_emu_usb_excuse="not enabled"
fi

if test x"$enable_grub_emu_pci" = xyes ; then
   grub_emu_usb_excuse="conflicts with PCI support"
fi

[if [ x"$grub_emu_usb_excuse" = x ]; then
    # Check for libusb libraries.]
AC_CHECK_LIB([usb], [usb_claim_interface], [LIBUSB="-lusb"],
    [grub_emu_usb_excuse=["need libusb library"]])
    AC_SUBST([LIBUSB])
[fi]
[if [ x"$grub_emu_usb_excuse" = x ]; then
    # Check for headers.]
    AC_CHECK_HEADERS([usb.h], [],
      [grub_emu_usb_excuse=["need libusb headers"]])
[fi]
if test x"$enable_grub_emu_usb" = xyes && test x"$grub_emu_usb_excuse" != x ; then
  AC_MSG_ERROR([USB support for grub-emu was explicitly requested but can't be compiled])
fi
if test x"$grub_emu_usb_excuse" = x ; then
enable_grub_emu_usb=yes
else
enable_grub_emu_usb=no
fi

if test x"$enable_grub_emu_sdl" = xno ; then
  grub_emu_sdl_excuse="explicitely disabled"
fi
[if [ x"$grub_emu_sdl_excuse" = x ]; then
    # Check for libSDL libraries.]
AC_CHECK_LIB([SDL], [SDL_Init], [LIBSDL="-lSDL"],
    [grub_emu_sdl_excuse=["libSDL libraries are required to build \`grub-emu' with SDL support"]])
    AC_SUBST([LIBSDL])
[fi]

[if [ x"$grub_emu_sdl_excuse" = x ]; then
    # Check for headers.]
    AC_CHECK_HEADERS([SDL/SDL.h], [],
      [grub_emu_sdl_excuse=["libSDL header file is required to build \`grub-emu' with SDL support"]])
[fi]

if test x"enable_grub_emu_sdl" = xyes && test x"$grub_emu_sdl_excuse" != x ; then
  AC_MSG_ERROR([SDL support for grub-emu was explicitely requested but can't be compiled])
fi
if test x"$grub_emu_sdl_excuse" = x ; then
enable_grub_emu_sdl=yes
else
enable_grub_emu_sdl=no
fi

if test x"$enable_grub_emu_pci" != xyes ; then
   grub_emu_pci_excuse="not enabled"
fi

if test x"$enable_grub_emu_usb" = xyes ; then
   grub_emu_pci_excuse="conflicts with USB support"
fi

[if [ x"$grub_emu_pci_excuse" = x ]; then
      # Check for libpci libraries.]
   AC_CHECK_LIB([pciaccess], [pci_system_init], [LIBPCIACCESS="-lpciaccess"],
      [grub_emu_pci_excuse=["need libpciaccess library"]])
    AC_SUBST([LIBPCIACCESS])
[fi]
[if [ x"$grub_emu_pci_excuse" = x ]; then
    # Check for headers.]
    AC_CHECK_HEADERS([pci/pci.h], [],
      [grub_emu_pci_excuse=["need libpciaccess headers"]])
[fi]

if test x"$grub_emu_pci_excuse" = x ; then
enable_grub_emu_pci=yes
else

enable_grub_emu_pci=no
fi

AC_SUBST([enable_grub_emu_sdl])
AC_SUBST([enable_grub_emu_usb])
AC_SUBST([enable_grub_emu_pci])
fi

AC_ARG_ENABLE([grub-mkfont],
	      [AS_HELP_STRING([--enable-grub-mkfont],
                             [build and install the `grub-mkfont' utility (default=guessed)])])
if test x"$enable_grub_mkfont" = xno ; then
  grub_mkfont_excuse="explicitly disabled"
fi

if test x"$grub_mkfont_excuse" = x ; then
  # Check for freetype libraries.
  AC_CHECK_PROGS([FREETYPE], [freetype-config])
  if test "x$FREETYPE" = x ; then
    grub_mkfont_excuse=["need freetype2 library"]
  fi
fi

if test x"$grub_mkfont_excuse" = x ; then
  # Check for freetype libraries.
  freetype_cflags=`freetype-config --cflags`
  freetype_libs=`freetype-config --libs`
  SAVED_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $freetype_cflags"
  AC_CHECK_HEADERS([ft2build.h], [],
  	[grub_mkfont_excuse=["need freetype2 headers"]])
  CPPFLAGS="$SAVED_CPPFLAGS"
fi

if test x"$enable_grub_mkfont" = xyes && test x"$grub_mkfont_excuse" != x ; then
  AC_MSG_ERROR([grub-mkfont was explicitly requested but can't be compiled])
fi
if test x"$grub_mkfont_excuse" = x ; then
enable_grub_mkfont=yes
else
enable_grub_mkfont=no
fi
AC_SUBST([enable_grub_mkfont])
AC_SUBST([freetype_cflags])
AC_SUBST([freetype_libs])

AC_ARG_ENABLE([device-mapper],
              [AS_HELP_STRING([--enable-device-mapper],
                              [enable Linux device-mapper support (default=guessed)])])
if test x"$enable_device_mapper" = xno ; then
  device_mapper_excuse="explicitly disabled"
fi

if test x"$device_mapper_excuse" = x ; then
  # Check for device-mapper library.
  AC_CHECK_LIB([devmapper], [dm_task_create], [],
               [device_mapper_excuse="need devmapper library"])
fi

if test x"$device_mapper_excuse" = x ; then
  # Check for device-mapper library.
  AC_CHECK_LIB([devmapper], [dm_log_with_errno_init],
               [],
               [device_mapper_excuse="need devmapper library"])
fi

if test x"$device_mapper_excuse" = x ; then
   LIBDEVMAPPER="-ldevmapper";
   AC_DEFINE([HAVE_DEVICE_MAPPER], [1],
             [Define to 1 if you have the devmapper library.])
fi

AC_SUBST([LIBDEVMAPPER])

AC_CHECK_LIB([lzma], [lzma_code],
             [LIBLZMA="-llzma"
              AC_DEFINE([HAVE_LIBLZMA], [1],
                        [Define to 1 if you have the LZMA library.])],)
AC_SUBST([LIBLZMA])

AC_CHECK_LIB([zfs], [libzfs_init],
             [LIBZFS="-lzfs"
              AC_DEFINE([HAVE_LIBZFS], [1],
                        [Define to 1 if you have the ZFS library.])],)
AC_SUBST([LIBZFS])

AC_CHECK_LIB([nvpair], [nvlist_print],
             [LIBNVPAIR="-lnvpair"
              AC_DEFINE([HAVE_LIBNVPAIR], [1],
                        [Define to 1 if you have the NVPAIR library.])],)
AC_SUBST([LIBNVPAIR])

LIBS=""

pkglibrootdir='$(libdir)'/`echo $PACKAGE | sed "$program_transform_name"`
AC_SUBST(pkglibrootdir)

AC_SUBST([FONT_SOURCE])
AS_IF([test x$target_cpu = xi386 -a x$platform = xqemu],
	    [AC_SUBST([GRUB_BOOT_MACHINE_LINK_ADDR], 0xffe00)])
AS_IF([test x$TARGET_APPLE_CC = x1],
	    [AC_SUBST([USE_APPLE_CC_FIXES], yes)])

AC_SUBST(HAVE_ASM_USCORE)
AC_SUBST(ADDR32)
AC_SUBST(DATA32)
AC_SUBST(BSS_START_SYMBOL)
AC_SUBST(END_SYMBOL)
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(NEED_ENABLE_EXECUTE_STACK)
AC_SUBST(NEED_REGISTER_FRAME_INFO)

#
# Automake conditionals
#

AM_CONDITIONAL([COND_emu], [test x$platform = xemu])
AM_CONDITIONAL([COND_i386_pc], [test x$target_cpu = xi386 -a x$platform = xpc])
AM_CONDITIONAL([COND_i386_efi], [test x$target_cpu = xi386 -a x$platform = xefi])
AM_CONDITIONAL([COND_i386_qemu], [test x$target_cpu = xi386 -a x$platform = xqemu])
AM_CONDITIONAL([COND_i386_ieee1275], [test x$target_cpu = xi386 -a x$platform = xieee1275])
AM_CONDITIONAL([COND_i386_coreboot], [test x$target_cpu = xi386 -a x$platform = xcoreboot])
AM_CONDITIONAL([COND_i386_multiboot], [test x$target_cpu = xi386 -a x$platform = xmultiboot])
AM_CONDITIONAL([COND_x86_64_efi], [test x$target_cpu = xx86_64 -a x$platform = xefi])
AM_CONDITIONAL([COND_mips_yeeloong], [test x$target_cpu = xmips -a x$platform = xyeeloong])
AM_CONDITIONAL([COND_mips_qemu_mips], [test x$target_cpu = xmips -a x$platform = xqemu_mips])
AM_CONDITIONAL([COND_sparc64_ieee1275], [test x$target_cpu = xsparc64 -a x$platform = xieee1275])
AM_CONDITIONAL([COND_powerpc_ieee1275], [test x$target_cpu = xpowerpc -a x$platform = xieee1275])
AM_CONDITIONAL([COND_mips], [test x$target_cpu = xmips])

AM_CONDITIONAL([COND_HOST_HURD], [test x$host_kernel = xhurd])
AM_CONDITIONAL([COND_HOST_LINUX], [test x$host_kernel = xlinux])
AM_CONDITIONAL([COND_HOST_NETBSD], [test x$host_kernel = xnetbsd])
AM_CONDITIONAL([COND_HOST_WINDOWS], [test x$host_kernel = xwindows])
AM_CONDITIONAL([COND_HOST_KFREEBSD], [test x$host_kernel = xkfreebsd])

AM_CONDITIONAL([COND_MAN_PAGES], [test x$cross_compiling = xno -a x$HELP2MAN != x])
AM_CONDITIONAL([COND_GRUB_EMU_USB], [test x$enable_grub_emu_usb = xyes])
AM_CONDITIONAL([COND_GRUB_EMU_SDL], [test x$enable_grub_emu_sdl = xyes])
AM_CONDITIONAL([COND_GRUB_EMU_PCI], [test x$enable_grub_emu_pci = xyes])
AM_CONDITIONAL([COND_GRUB_MKFONT], [test x$enable_grub_mkfont = xyes])
AM_CONDITIONAL([COND_HAVE_FONT_SOURCE], [test x$FONT_SOURCE != x])
AM_CONDITIONAL([COND_GRUB_PE2ELF], [test x$TARGET_OBJ2ELF != x])
AM_CONDITIONAL([COND_APPLE_CC], [test x$TARGET_APPLE_CC = x1])
AM_CONDITIONAL([COND_ENABLE_EFIEMU], [test x$enable_efiemu = xyes])

AM_CONDITIONAL([COND_HAVE_ASM_USCORE], [test x$HAVE_ASM_USCORE = x1])

# Output files.
grub_CHECK_LINK_DIR
if test x"$link_dir" = xyes ; then
  AC_CONFIG_LINKS([include/grub/cpu:include/grub/$target_cpu])
  if test "$platform" != emu ; then
    AC_CONFIG_LINKS([include/grub/machine:include/grub/$target_cpu/$platform])
  fi
else
  mkdir -p include/grub 2>/dev/null
  rm -rf include/grub/cpu
  cp -rp $srcdir/include/grub/$target_cpu include/grub/cpu 2>/dev/null
  if test "$platform" != emu ; then
    rm -rf include/grub/machine
    cp -rp $srcdir/include/grub/$target_cpu/$platform include/grub/machine 2>/dev/null
  fi
fi

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([grub-core/Makefile])
AC_CONFIG_FILES([grub-core/gensyminfo.sh])
AC_CONFIG_FILES([grub-core/gnulib/Makefile])
AC_CONFIG_FILES([po/Makefile.in])
AC_CONFIG_FILES([docs/Makefile])
AC_CONFIG_FILES([util/bash-completion.d/Makefile])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_CONFIG_FILES([config.h])

AC_OUTPUT
[
echo "*******************************************************"
echo GRUB2 will be compiled with following components:
echo Platform: "$target_cpu"-"$platform"
if [ x"$platform" = xemu ]; then
if [ x"$grub_emu_usb_excuse" = x ]; then
echo USB support for grub-emu: Yes
else
echo USB support for grub-emu: No "($grub_emu_usb_excuse)"
fi
if [ x"$grub_emu_sdl_excuse" = x ]; then
echo SDL support for grub-emu: Yes
else
echo SDL support for grub-emu: No "($grub_emu_sdl_excuse)"
fi
if [ x"$grub_emu_pci_excuse" = x ]; then
echo PCI support for grub-emu: Yes
else
echo PCI support for grub-emu: No "($grub_emu_pci_excuse)"
fi
fi
if test x"$device_mapper_excuse" = x ; then
echo With devmapper support: Yes
else
echo With devmapper support: No "($device_mapper_excuse)"
fi
if [ x"$enable_mm_debug" = xyes ]; then
echo With memory debugging: Yes
else
echo With memory debugging: No
fi
if [ x"$efiemu_excuse" = x ]; then
echo efiemu runtime: Yes
else
echo efiemu runtime: No "($efiemu_excuse)"
fi
if [ x"$grub_mkfont_excuse" = x ]; then
echo grub-mkfont: Yes
else
echo grub-mkfont: No "($grub_mkfont_excuse)"
fi
echo "*******************************************************"
]
