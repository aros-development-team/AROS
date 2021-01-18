#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(adflib, 0.7.12, lclevy_AT_club-internet.fr)
AC_CONFIG_SRCDIR([src/adf_env.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AC_ARG_ENABLE([examples],
              [  --enable-examples    Build exmples],
              [case "${enableval}" in
                yes) examples=true ;;
                no)  examples=false ;;
                *) AC_MSG_ERROR([bad value ${enableval} for --enable-examples]) ;;
               esac],
              [examples=true])

AM_CONDITIONAL([EXAMPLES], [test x$examples = xtrue])

AC_MSG_CHECKING([where to install examples])
AC_ARG_WITH(example-DIR,AC_HELP_STRING([--with-example-DIR=PATH],[Install the examples to the directory specified by PATH (defaults to $(DESTDIR)$(bindir))]),top_examplebindir="$withval",top_examplebindir="")
if test "$top_examplebindir" != "" ; then
    msg_result=$top_examplebindir
else
    top_examplebindir="$""(DESTDIR)""$""(bindir)"
    msg_result="default"
fi
AC_MSG_RESULT($msg_result)

AC_ARG_ENABLE([regtests],
              [  --enable-regtests    Build regression tests],
              [case "${enableval}" in
                yes) regtests=true ;;
                no)  regtests=false ;;
                *) AC_MSG_ERROR([bad value ${enableval} for --enable-regtests]) ;;
               esac],
              [regtests=true])

AM_CONDITIONAL([REGTESTS], [test x$regtests = xtrue])

AC_MSG_CHECKING([where to install tests])
AC_ARG_WITH(test-DIR,AC_HELP_STRING([--with-test-DIR=PATH],[Install the tests to the directory specified by PATH (defaults to $(DESTDIR)$(bindir))]),top_testbindir="$withval",top_testbindir="")
if test "$top_testbindir" != "" ; then
    msg_result=$top_testbindir
else
    top_testbindir="$""(DESTDIR)""$""(bindir)"
    msg_result="default"
fi
AC_MSG_RESULT($msg_result)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_HEADER_STDBOOL
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_CHECK_FUNCS([memset strchr strdup strerror strrchr])

AC_SUBST(top_examplebindir)
AC_SUBST(top_testbindir)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 doc/Makefile
                 examples/Makefile
                 regtests/Test/Makefile
                 adflib.pc])
AC_OUTPUT
