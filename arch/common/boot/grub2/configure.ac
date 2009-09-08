# Process this file with autoconf to produce a configure script.

# Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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


AC_INIT([GRUB],[1.97~beta2],[bug-grub@gnu.org])
AC_PREREQ(2.59)
AC_CONFIG_SRCDIR([include/grub/dl.h])
AC_CONFIG_HEADER([config.h])

# Checks for host and target systems.
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

# Program name transformations
AC_ARG_PROGRAM

case "$target_cpu" in
  i[[3456]]86)	target_cpu=i386 ;;
  sparc)	target_cpu=sparc64 ;;
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

# Adjust CPU.
case "$target_cpu"-"$platform" in
  x86_64-efi) ;;
  x86_64-*) target_cpu=i386 ;;
  powerpc64-ieee1275) target_cpu=powerpc ;;
esac

# Check if the platform is supported, make final adjustments.
case "$target_cpu"-"$platform" in
  i386-efi) ;;
  x86_64-efi) ;;
  i386-pc) ;;
  i386-coreboot) ;;
  i386-linuxbios) platform=coreboot ;;
  i386-ieee1275) ;;
  i386-qemu) ;;
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

# This normalizes the names, and creates a new variable ("host_kernel")
# while at it, since the mapping is not always 1:1 (e.g. different OSes
# using the same kernel type).
case "$host_os" in
  gnu*)				host_kernel=hurd ;;
  linux*)			host_kernel=linux ;;
  freebsd* | kfreebsd*-gnu)	host_kernel=freebsd ;;
  cygwin)			host_kernel=windows ;;
esac

AC_SUBST(host_cpu)
AC_SUBST(host_os)
AC_SUBST(host_kernel)

AC_SUBST(target_cpu)
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

for file in /usr/src/unifont.bdf ; do
  if test -e $file ; then
    AC_SUBST([UNIFONT_BDF], [$file])
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

grub_apple_cc
if test x$grub_cv_apple_cc = xyes ; then
  CFLAGS="$CFLAGS -DAPPLE_CC=1 -fnested-functions"
  ASFLAGS="$ASFLAGS -DAPPLE_CC=1"
fi

# Check for functions.
AC_CHECK_FUNCS(posix_memalign memalign asprintf)

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
fi

grub_apple_target_cc
if test x$grub_cv_apple_target_cc = xyes ; then
  TARGET_CFLAGS="$TARGET_CFLAGS -DAPPLE_CC=1 -fnested-functions"
  CFLAGS="$CFLAGS -DAPPLE_CC=1 -fnested-functions"
  TARGET_ASFLAGS="$TARGET_ASFLAGS -DAPPLE_CC=1"
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
  TARGET_IMG_LDFLAGS='-nostdlib -static -Wl,-preload -Wl,-segalign,20 -Wl,-image_base,'
  TARGET_IMG_LDFLAGS_AC='-nostdlib -static -Wl,-preload -Wl,-segalign,20 -Wl,-image_base,'
else
  TARGET_APPLE_CC=0
# Use linker script if present, otherwise use builtin -N script.
if test -f "${srcdir}/conf/${target_cpu}-${platform}-${host_os}-img-ld.sc"; then
  TARGET_IMG_LDSCRIPT='$(top_srcdir)'"/conf/${target_cpu}-${platform}-${host_os}-img-ld.sc"
  TARGET_IMG_LDFLAGS="-Wl,-T${TARGET_IMG_LDSCRIPT}  -Wl,-Ttext,"
  TARGET_IMG_LDFLAGS_AC="-Wl,-T${srcdir}/conf/${target_cpu}-${platform}-${host_os}-img-ld.sc"
else
  TARGET_IMG_LDSCRIPT=
  TARGET_IMG_LDFLAGS='-Wl,-N  -Wl,-Ttext,'
  TARGET_IMG_LDFLAGS_AC='-Wl,-N  -Wl,-Ttext,'
fi
TARGET_IMG_CFLAGS=
fi

AC_SUBST(TARGET_IMG_LDSCRIPT)
AC_SUBST(TARGET_IMG_LDFLAGS)
AC_SUBST(TARGET_IMG_CFLAGS)

# For platforms where ELF is not the default link format.
AC_MSG_CHECKING([for command to convert module to ELF format])
case "${host_os}" in
  cygwin) TARGET_OBJ2ELF='grub-pe2elf' ;;
  *) ;;
esac
AC_SUBST(TARGET_OBJ2ELF)
AC_MSG_RESULT([$TARGET_OBJ2ELF])


if test "x$target_m32" = x1; then
  # Force 32-bit mode.
  TARGET_CFLAGS="$TARGET_CFLAGS -m32"
  TARGET_LDFLAGS="$TARGET_LDFLAGS -m32"
  TARGET_MODULE_FORMAT="elf32"
fi

if test "x$target_m64" = x1; then
  # Force 64-bit mode.
  TARGET_CFLAGS="$TARGET_CFLAGS -m64"
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
    CFLAGS="$SAVED_CFLAGS -m64 -DMCMODEL_SMALL=1"
    TARGET_CFLAGS="$TARGET_CFLAGS -DMCMODEL_SMALL=1"
    AC_MSG_WARN([-mcmodel=large not supported. You won't be able to use the memory over 4GiB. Upgrade your gcc])
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

AC_SUBST(TARGET_CFLAGS)
AC_SUBST(TARGET_MODULE_FORMAT)
AC_SUBST(OBJCONV)
AC_SUBST(TARGET_APPLE_CC)
AC_SUBST(TARGET_ASFLAGS)
AC_SUBST(TARGET_CPPFLAGS)
AC_SUBST(TARGET_LDFLAGS)

# Set them to their new values for the tests below.
CC="$TARGET_CC"
if test "x$TARGET_APPLE_CC" = x1 ; then
CFLAGS="$TARGET_CFLAGS -nostdlib"
else
CFLAGS="$TARGET_CFLAGS -nostdlib -Wl,--defsym,___main=0x8100"
fi
CPPFLAGS="$TARGET_CPPFLAGS"
LDFLAGS="$TARGET_LDFLAGS"

# Defined in aclocal.m4.
grub_PROG_TARGET_CC
if test "x$TARGET_APPLE_CC" != x1 ; then
grub_PROG_OBJCOPY_ABSOLUTE
fi
grub_PROG_LD_BUILD_ID_NONE
grub_ASM_USCORE
if test "x$target_cpu" = xi386; then
  if test ! -z "$TARGET_IMG_LDSCRIPT"; then
    # Check symbols provided by linker script.
    CFLAGS="$TARGET_CFLAGS -nostdlib $TARGET_IMG_LDFLAGS_AC -Wl,-Ttext,8000,--defsym,___main=0x8100"
  fi
  if test "x$TARGET_APPLE_CC" != x1 ; then
    grub_CHECK_BSS_START_SYMBOL
    grub_CHECK_END_SYMBOL
  fi
  CFLAGS="$TARGET_CFLAGS"
  grub_I386_ASM_PREFIX_REQUIREMENT
  grub_I386_ASM_ADDR32
  grub_I386_ASM_ABSOLUTE_WITHOUT_ASTERISK
else
  AC_DEFINE([NESTED_FUNC_ATTR], [], [Catch gcc bug])
fi

AH_BOTTOM([#if defined(__i386__) && !defined(GRUB_UTIL)
#define NESTED_FUNC_ATTR __attribute__ ((__regparm__ (1)))
#else
#define NESTED_FUNC_ATTR
#endif])

AC_ARG_ENABLE([efiemu],
	      [AS_HELP_STRING([--enable-efiemu],
                             [build and install the efiemu runtimes (default=guessed)])])
if test x"$enable_efiemu" = xno ; then
  efiemu_excuse="explicitly disabled"
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
                             [build and install the `grub-emu' debugging utility (default=guessed)])])
AC_ARG_ENABLE([grub-emu-usb],
	      [AS_HELP_STRING([--enable-grub-emu-usb],
                             [build and install the `grub-emu' debugging utility with USB support (default=guessed)])])
if test x"$enable_grub_emu" = xno ; then
  grub_emu_excuse="explicitly disabled"
fi

  [# Check for curses libraries.]
[if [ x"$grub_emu_excuse" = x ]; then ]
  AC_CHECK_LIB([ncurses], [wgetch], [LIBCURSES="-lncurses"],
    [AC_CHECK_LIB([curses], [wgetch], [LIBCURSES="-lcurses"],
      [grub_emu_excuse=["need (n)curses libraries"]])])
  AC_SUBST([LIBCURSES])
[fi]
[if [ x"$grub_emu_excuse" = x ]; then ]
  [# Check for headers.]
  AC_CHECK_HEADERS([ncurses/curses.h], [],
    [AC_CHECK_HEADERS([ncurses.h], [],
      [AC_CHECK_HEADERS([curses.h], [],
	[grub_emu_excuse=["need (n)curses headers"]])])])
[fi]

if test x"$enable_grub_emu" = xyes && test x"$grub_emu_excuse" != x ; then
  AC_MSG_ERROR([grub-emu was explicitly requested but can't be compiled])
fi
if test x"$grub_emu_excuse" = x ; then
enable_grub_emu=yes
else
enable_grub_emu=no
grub_emu_usb_excuse="grub-emu isn't built"
fi
if test x"$enable_grub_emu_usb" = xno ; then
  grub_emu_usb_excuse="explicitly disabled"
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
if test x"enable_grub_emu_usb" = xyes && test x"$grub_emu_usb_excuse" != x ; then
  AC_MSG_ERROR([USB support for grub-emu was explicitly requested but can't be compiled])
fi
if test x"$grub_emu_usb_excuse" = x ; then
enable_grub_emu_usb=yes
else
enable_grub_emu_usb=no
fi

AC_SUBST([enable_grub_emu])
AC_SUBST([enable_grub_emu_usb])

AC_ARG_ENABLE([grub-fstest],
	      [AS_HELP_STRING([--enable-grub-fstest],
                             [build and install the `grub-fstest' debugging utility (default=guessed)])])
if test x"$enable_grub_fstest" = xno ; then
  grub_fstest_excuse="explicitly disabled"
fi
if test x"$grub_fstest_excuse" = x ; then
enable_grub_fstest=yes
else
enable_grub_fstest=no
fi
AC_SUBST([enable_grub_fstest])

AC_ARG_ENABLE([grub-pe2elf],
	      [AS_HELP_STRING([--enable-grub-pe2elf],
                             [build and install the `grub-pe2elf' conversion utility])])
AC_SUBST([enable_grub_pe2elf])

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
  freetype_cflags=`freetype-config --cflags`
  freetype_libs=`freetype-config --libs`
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

AC_SUBST(ASFLAGS)

# Output files.
grub_CHECK_LINK_DIR
if test x"$link_dir" = xyes ; then
  AC_CONFIG_LINKS([include/grub/cpu:include/grub/$target_cpu
	include/grub/machine:include/grub/$target_cpu/$platform])
else
  mkdir -p include/grub 2>/dev/null
  rm -rf include/grub/cpu
  cp -rp $srcdir/include/grub/$target_cpu include/grub/cpu 2>/dev/null
  rm -rf include/grub/machine
  cp -rp $srcdir/include/grub/$target_cpu/$platform include/grub/machine 2>/dev/null
fi
AC_CONFIG_FILES([Makefile gensymlist.sh genkernsyms.sh])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])
AC_OUTPUT
[
echo "*******************************************************"
echo GRUB2 will be compiled with following components:
echo Platform: "$target_cpu"-"$platform"
if [ x"$grub_emu_excuse" = x ]; then
echo grub-emu: Yes
else
echo grub-emu: No "($grub_emu_excuse)"
fi
if [ x"$grub_emu_usb_excuse" = x ]; then
echo USB support for grub-emu: Yes
else
echo USB support for grub-emu: No "($grub_emu_usb_excuse)"
fi
if [ x"$enable_mm_debug" = xyes ]; then
echo With memory debugging: Yes
else
echo With memory debugging: No
fi
if [ x"$enable_grub_pe2elf" = xyes ]; then
echo grub-pe2elf will be built and installed
elif [ x$TARGET_OBJ2ELF = xgrub-pe2elf ]; then
echo grub-pe2elf will be built but not installed
else
echo grub-pe2elf will not be built
fi
if [ x"$efiemu_excuse" = x ]; then
echo efiemu runtime: Yes
else
echo efiemu runtime: No "($efiemu_excuse)"
fi
if [ x"$grub_fstest_excuse" = x ]; then
echo grub-fstest: Yes
else
echo grub-fstest: No "($grub_fstest_excuse)"
fi
if [ x"$grub_mkfont_excuse" = x ]; then
echo grub-mkfont: Yes
else
echo grub-mkfont: No "($grub_mkfont_excuse)"
fi
echo "*******************************************************"
]
