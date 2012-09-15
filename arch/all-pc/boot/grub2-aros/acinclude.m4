dnl Redefine AC_LANG_PROGRAM with a "-Wstrict-prototypes -Werror"-friendly
dnl version.  Patch submitted to bug-autoconf in 2009-09-16.
m4_define([AC_LANG_PROGRAM(C)],
[$1
int
main (void)
{
dnl Do *not* indent the following line: there may be CPP directives.
dnl Don't move the `;' right after for the same reason.
$2
  ;
  return 0;
}])


dnl Check whether target compiler is working
AC_DEFUN([grub_PROG_TARGET_CC],
[AC_MSG_CHECKING([whether target compiler is working])
AC_CACHE_VAL(grub_cv_prog_target_cc,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[
asm (".globl start; start:");
int main (void);
]], [[]])],
  		[grub_cv_prog_target_cc=yes],
		[grub_cv_prog_target_cc=no])
])
AC_MSG_RESULT([$grub_cv_prog_target_cc])

if test "x$grub_cv_prog_target_cc" = xno; then
  AC_MSG_ERROR([cannot compile for the target])
fi
])


dnl grub_ASM_USCORE checks if C symbols get an underscore after
dnl compiling to assembler.
dnl Written by Pavel Roskin. Based on grub_ASM_EXT_C written by
dnl Erich Boleyn and modified by Yoshinori K. Okuji.
AC_DEFUN([grub_ASM_USCORE],
[AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PROG_EGREP])
AC_MSG_CHECKING([if C symbols get an underscore after compilation])
AC_CACHE_VAL(grub_cv_asm_uscore,
[cat > conftest.c <<\EOF
int func (int *);
int
func (int *list)
{
  *list = 0;
  return *list;
}
EOF

if AC_TRY_COMMAND([${CC-cc} ${CFLAGS} -S conftest.c]) && test -s conftest.s; then
  true
else
  AC_MSG_ERROR([${CC-cc} failed to produce assembly code])
fi

if $EGREP '(^|[^_[:alnum]])_func' conftest.s >/dev/null 2>&1; then
  HAVE_ASM_USCORE=1
  grub_cv_asm_uscore=yes
else
  HAVE_ASM_USCORE=0
  grub_cv_asm_uscore=no
fi

rm -f conftest*])

AC_MSG_RESULT([$grub_cv_asm_uscore])
])


dnl Some versions of `objcopy -O binary' vary their output depending
dnl on the link address.
AC_DEFUN([grub_PROG_OBJCOPY_ABSOLUTE],
[AC_MSG_CHECKING([whether ${OBJCOPY} works for absolute addresses])
AC_CACHE_VAL(grub_cv_prog_objcopy_absolute,
[cat > conftest.c <<\EOF
void cmain (void);
void
cmain (void)
{
   *((int *) 0x1000) = 2;
}
EOF

if AC_TRY_EVAL(ac_compile) && test -s conftest.o; then :
else
  AC_MSG_ERROR([${CC-cc} cannot compile C source code])
fi
grub_cv_prog_objcopy_absolute=yes
for link_addr in 0x2000 0x8000 0x7C00; do
  if AC_TRY_COMMAND([${CC-cc} ${CFLAGS} -nostdlib ${TARGET_IMG_LDFLAGS_AC} ${TARGET_IMG_BASE_LDOPT},$link_addr conftest.o -o conftest.exec]); then :
  else
    AC_MSG_ERROR([${CC-cc} cannot link at address $link_addr])
  fi
  if AC_TRY_COMMAND([${OBJCOPY-objcopy} --only-section=.text -O binary conftest.exec conftest]); then :
  else
    AC_MSG_ERROR([${OBJCOPY-objcopy} cannot create binary files])
  fi
  if test ! -f conftest.old || AC_TRY_COMMAND([cmp -s conftest.old conftest]); then
    mv -f conftest conftest.old
  else
    grub_cv_prog_objcopy_absolute=no
    break
  fi
done
rm -f conftest*])
AC_MSG_RESULT([$grub_cv_prog_objcopy_absolute])

if test "x$grub_cv_prog_objcopy_absolute" = xno; then
  AC_MSG_ERROR([GRUB requires a working absolute objcopy; upgrade your binutils])
fi
])


dnl Supply --build-id=none to ld if building modules.
dnl This suppresses warnings from ld on some systems
AC_DEFUN([grub_PROG_LD_BUILD_ID_NONE],
[AC_MSG_CHECKING([whether linker accepts --build-id=none])
AC_CACHE_VAL(grub_cv_prog_ld_build_id_none,
[save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -Wl,--build-id=none"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
	       [grub_cv_prog_ld_build_id_none=yes],
	       [grub_cv_prog_ld_build_id_none=no])
LDFLAGS="$save_LDFLAGS"
])
AC_MSG_RESULT([$grub_cv_prog_ld_build_id_none])

if test "x$grub_cv_prog_ld_build_id_none" = xyes; then
  TARGET_LDFLAGS="$TARGET_LDFLAGS -Wl,--build-id=none"
fi
])


dnl Mass confusion!
dnl Older versions of GAS interpret `.code16' to mean ``generate 32-bit
dnl instructions, but implicitly insert addr32 and data32 bytes so
dnl that the code works in real mode''.
dnl
dnl Newer versions of GAS interpret `.code16' to mean ``generate 16-bit
dnl instructions,'' which seems right.  This requires the programmer
dnl to explicitly insert addr32 and data32 instructions when they want
dnl them.
dnl
dnl We only support the newer versions, because the old versions cause
dnl major pain, by requiring manual assembly to get 16-bit instructions into
dnl asm files.
AC_DEFUN([grub_I386_ASM_ADDR32],
[AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([grub_I386_ASM_PREFIX_REQUIREMENT])
AC_MSG_CHECKING([for .code16 addr32 assembler support])
AC_CACHE_VAL(grub_cv_i386_asm_addr32,
[cat > conftest.s.in <<\EOF
	.code16
l1:	@ADDR32@	movb	%al, l1
EOF

if test "x$grub_cv_i386_asm_prefix_requirement" = xyes; then
  sed -e s/@ADDR32@/addr32/ < conftest.s.in > conftest.s
else
  sed -e s/@ADDR32@/addr32\;/ < conftest.s.in > conftest.s
fi

if AC_TRY_COMMAND([${CC-cc} ${CFLAGS} -c conftest.s]) && test -s conftest.o; then
  grub_cv_i386_asm_addr32=yes
else
  grub_cv_i386_asm_addr32=no
fi

rm -f conftest*])

AC_MSG_RESULT([$grub_cv_i386_asm_addr32])])

dnl check if our compiler is apple cc
dnl because it requires numerous workarounds
AC_DEFUN([grub_apple_cc],
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([whether our compiler is apple cc])
AC_CACHE_VAL(grub_cv_apple_cc,
[if $CC -v 2>&1 | grep "Apple Inc." > /dev/null; then
  grub_cv_apple_cc=yes
else
  grub_cv_apple_cc=no
fi
])

AC_MSG_RESULT([$grub_cv_apple_cc])])

dnl check if our target compiler is apple cc
dnl because it requires numerous workarounds
AC_DEFUN([grub_apple_target_cc],
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([whether our target compiler is apple cc])
AC_CACHE_VAL(grub_cv_apple_target_cc,
[if $CC -v 2>&1 | grep "Apple Inc." > /dev/null; then
  grub_cv_apple_target_cc=yes
else
  grub_cv_apple_target_cc=no
fi
])

AC_MSG_RESULT([$grub_cv_apple_target_cc])])


dnl Later versions of GAS requires that addr32 and data32 prefixes
dnl appear in the same lines as the instructions they modify, while
dnl earlier versions requires that they appear in separate lines.
AC_DEFUN([grub_I386_ASM_PREFIX_REQUIREMENT],
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING(dnl
[whether addr32 must be in the same line as the instruction])
AC_CACHE_VAL(grub_cv_i386_asm_prefix_requirement,
[cat > conftest.s <<\EOF
	.code16
l1:	addr32	movb	%al, l1
EOF

if AC_TRY_COMMAND([${CC-cc} ${CFLAGS} -c conftest.s]) && test -s conftest.o; then
  grub_cv_i386_asm_prefix_requirement=yes
else
  grub_cv_i386_asm_prefix_requirement=no
fi

rm -f conftest*])

if test "x$grub_cv_i386_asm_prefix_requirement" = xyes; then
  grub_tmp_addr32="addr32"
  grub_tmp_data32="data32"
else
  grub_tmp_addr32="addr32;"
  grub_tmp_data32="data32;"
fi

ADDR32=$grub_tmp_addr32
DATA32=$grub_tmp_data32

AC_MSG_RESULT([$grub_cv_i386_asm_prefix_requirement])])


dnl Check what symbol is defined as a bss start symbol.
dnl Written by Michael Hohmoth and Yoshinori K. Okuji.
AC_DEFUN([grub_CHECK_BSS_START_SYMBOL],
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([if __bss_start is defined by the compiler])
AC_CACHE_VAL(grub_cv_check_uscore_uscore_bss_start_symbol,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
		[[asm ("incl __bss_start")]])],
		[grub_cv_check_uscore_uscore_bss_start_symbol=yes],
		[grub_cv_check_uscore_uscore_bss_start_symbol=no])])

AC_MSG_RESULT([$grub_cv_check_uscore_uscore_bss_start_symbol])

AC_MSG_CHECKING([if edata is defined by the compiler])
AC_CACHE_VAL(grub_cv_check_edata_symbol,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
		[[asm ("incl edata")]])],
		[grub_cv_check_edata_symbol=yes],
		[grub_cv_check_edata_symbol=no])])

AC_MSG_RESULT([$grub_cv_check_edata_symbol])

AC_MSG_CHECKING([if _edata is defined by the compiler])
AC_CACHE_VAL(grub_cv_check_uscore_edata_symbol,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
		[[asm ("incl _edata")]])],
		[grub_cv_check_uscore_edata_symbol=yes],
		[grub_cv_check_uscore_edata_symbol=no])])

AC_MSG_RESULT([$grub_cv_check_uscore_edata_symbol])

if test "x$grub_cv_check_uscore_uscore_bss_start_symbol" = xyes; then
  BSS_START_SYMBOL=__bss_start
elif test "x$grub_cv_check_edata_symbol" = xyes; then
  BSS_START_SYMBOL=edata
elif test "x$grub_cv_check_uscore_edata_symbol" = xyes; then
  BSS_START_SYMBOL=_edata
else
  AC_MSG_ERROR([none of __bss_start, edata or _edata is defined])
fi
])

dnl Check what symbol is defined as an end symbol.
dnl Written by Yoshinori K. Okuji.
AC_DEFUN([grub_CHECK_END_SYMBOL],
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([if end is defined by the compiler])
AC_CACHE_VAL(grub_cv_check_end_symbol,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
		[[asm ("incl end")]])],
		[grub_cv_check_end_symbol=yes],
		[grub_cv_check_end_symbol=no])])

AC_MSG_RESULT([$grub_cv_check_end_symbol])

AC_MSG_CHECKING([if _end is defined by the compiler])
AC_CACHE_VAL(grub_cv_check_uscore_end_symbol,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
		[[asm ("incl _end")]])],
		[grub_cv_check_uscore_end_symbol=yes],
		[grub_cv_check_uscore_end_symbol=no])])

AC_MSG_RESULT([$grub_cv_check_uscore_end_symbol])

if test "x$grub_cv_check_end_symbol" = xyes; then
  END_SYMBOL=end
elif test "x$grub_cv_check_uscore_end_symbol" = xyes; then
  END_SYMBOL=_end
else
  AC_MSG_ERROR([neither end nor _end is defined])
fi
])

dnl Check if the C compiler generates calls to `__enable_execute_stack()'.
AC_DEFUN([grub_CHECK_ENABLE_EXECUTE_STACK],[
AC_MSG_CHECKING([whether `$CC' generates calls to `__enable_execute_stack()'])
AC_LANG_CONFTEST([AC_LANG_SOURCE([[
void f (int (*p) (void));
void g (int i)
{
  int nestedfunc (void) { return i; }
  f (nestedfunc);
}
]])])
if AC_TRY_COMMAND([${CC-cc} ${CFLAGS} -S conftest.c]) && test -s conftest.s; then
  true
else
  AC_MSG_ERROR([${CC-cc} failed to produce assembly code])
fi
if grep __enable_execute_stack conftest.s >/dev/null 2>&1; then
  NEED_ENABLE_EXECUTE_STACK=1
  AC_MSG_RESULT([yes])
else
  NEED_ENABLE_EXECUTE_STACK=0
  AC_MSG_RESULT([no])
fi
rm -f conftest*
])


dnl Check if the C compiler supports `-fstack-protector'.
AC_DEFUN([grub_CHECK_STACK_PROTECTOR],[
[# Smashing stack protector.
ssp_possible=yes]
AC_MSG_CHECKING([whether `$CC' accepts `-fstack-protector'])
# Is this a reliable test case?
AC_LANG_CONFTEST([AC_LANG_SOURCE([[
void foo (void) { volatile char a[8]; a[3]; }
]])])
[# `$CC -c -o ...' might not be portable.  But, oh, well...  Is calling
# `ac_compile' like this correct, after all?
if eval "$ac_compile -S -fstack-protector -o conftest.s" 2> /dev/null; then]
  AC_MSG_RESULT([yes])
  [# Should we clear up other files as well, having called `AC_LANG_CONFTEST'?
  rm -f conftest.s
else
  ssp_possible=no]
  AC_MSG_RESULT([no])
[fi]
])

dnl Check if the C compiler supports `-mstack-arg-probe' (Cygwin).
AC_DEFUN([grub_CHECK_STACK_ARG_PROBE],[
[# Smashing stack arg probe.
sap_possible=yes]
AC_MSG_CHECKING([whether `$CC' accepts `-mstack-arg-probe'])
AC_LANG_CONFTEST([AC_LANG_SOURCE([[
void foo (void) { volatile char a[8]; a[3]; }
]])])
[if eval "$ac_compile -S -mstack-arg-probe -o conftest.s" 2> /dev/null; then]
  AC_MSG_RESULT([yes])
  [# Should we clear up other files as well, having called `AC_LANG_CONFTEST'?
  rm -f conftest.s
else
  sap_possible=no]
  AC_MSG_RESULT([no])
[fi]
])

dnl Check if ln can handle directories properly (mingw).
AC_DEFUN([grub_CHECK_LINK_DIR],[
AC_MSG_CHECKING([whether ln can handle directories properly])
[mkdir testdir 2>/dev/null
case $srcdir in
[\\/$]* | ?:[\\/]* ) reldir=$srcdir/include/grub/util ;;
    *) reldir=../$srcdir/include/grub/util ;;
esac
if ln -s $reldir testdir/util 2>/dev/null ; then]
  AC_MSG_RESULT([yes])
  [link_dir=yes
else
  link_dir=no]
  AC_MSG_RESULT([no])
[fi
rm -rf testdir]
])

dnl Check if the C compiler supports `-fPIE'.
AC_DEFUN([grub_CHECK_PIE],[
[# Position independent executable.
pie_possible=yes]
AC_MSG_CHECKING([whether `$CC' has `-fPIE' as default])
# Is this a reliable test case?
AC_LANG_CONFTEST([AC_LANG_SOURCE([[
#ifdef __PIE__
int main() {
	return 0;
}
#else
#error NO __PIE__ DEFINED
#endif
]])])

[# `$CC -c -o ...' might not be portable.  But, oh, well...  Is calling
# `ac_compile' like this correct, after all?
if eval "$ac_compile -S -o conftest.s" 2> /dev/null; then]
  AC_MSG_RESULT([yes])
  [# Should we clear up other files as well, having called `AC_LANG_CONFTEST'?
  rm -f conftest.s
else
  pie_possible=no]
  AC_MSG_RESULT([no])
[fi]
])

dnl Check if the C compiler supports `-fPIC'.
AC_DEFUN([grub_CHECK_PIC],[
[# Position independent executable.
pic_possible=yes]
AC_MSG_CHECKING([whether `$CC' has `-fPIC' as default])
# Is this a reliable test case?
AC_LANG_CONFTEST([AC_LANG_SOURCE([[
#ifdef __PIC__
int main() {
	return 0;
}
#else
#error NO __PIC__ DEFINED
#endif
]])])

[# `$CC -c -o ...' might not be portable.  But, oh, well...  Is calling
# `ac_compile' like this correct, after all?
if eval "$ac_compile -S -o conftest.s" 2> /dev/null; then]
  AC_MSG_RESULT([yes])
  [# Should we clear up other files as well, having called `AC_LANG_CONFTEST'?
  rm -f conftest.s
else
  pic_possible=no]
  AC_MSG_RESULT([no])
[fi]
])
