# realloc.m4 serial 11
dnl Copyright (C) 2007, 2009, 2010 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# gl_FUNC_REALLOC_GNU
# -------------------
# Test whether 'realloc (0, 0)' is handled like in GNU libc, and replace
# realloc if it is not.
AC_DEFUN([gl_FUNC_REALLOC_GNU],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  dnl _AC_FUNC_REALLOC_IF is defined in Autoconf.
  _AC_FUNC_REALLOC_IF(
    [AC_DEFINE([HAVE_REALLOC_GNU], [1],
               [Define to 1 if your system has a GNU libc compatible 'realloc'
                function, and to 0 otherwise.])],
    [AC_DEFINE([HAVE_REALLOC_GNU], [0])
     gl_REPLACE_REALLOC
    ])
])# gl_FUNC_REALLOC_GNU

# gl_FUNC_REALLOC_POSIX
# ---------------------
# Test whether 'realloc' is POSIX compliant (sets errno to ENOMEM when it
# fails), and replace realloc if it is not.
AC_DEFUN([gl_FUNC_REALLOC_POSIX],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_REQUIRE([gl_CHECK_MALLOC_POSIX])
  if test $gl_cv_func_malloc_posix = yes; then
    AC_DEFINE([HAVE_REALLOC_POSIX], [1],
      [Define if the 'realloc' function is POSIX compliant.])
  else
    gl_REPLACE_REALLOC
  fi
])

AC_DEFUN([gl_REPLACE_REALLOC],
[
  AC_LIBOBJ([realloc])
  REPLACE_REALLOC=1
])
