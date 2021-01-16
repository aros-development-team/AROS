# generated automatically by aclocal 1.15.1 -*- Autoconf -*-

# Copyright (C) 1996-2017 Free Software Foundation, Inc.

# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

m4_ifndef([AC_CONFIG_MACRO_DIRS], [m4_defun([_AM_CONFIG_MACRO_DIRS], [])m4_defun([AC_CONFIG_MACRO_DIRS], [_AM_CONFIG_MACRO_DIRS($@)])])
# Configure paths for SDL
# Sam Lantinga 9/21/99
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

# serial 1

dnl AM_PATH_SDL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for SDL, and define SDL_CFLAGS and SDL_LIBS
dnl
AC_DEFUN([AM_PATH_SDL],
[dnl 
dnl Get the cflags and libraries from the sdl-config script
dnl
AC_ARG_WITH(sdl-prefix,[  --with-sdl-prefix=PFX   Prefix where SDL is installed (optional)],
            sdl_prefix="$withval", sdl_prefix="")
AC_ARG_WITH(sdl-exec-prefix,[  --with-sdl-exec-prefix=PFX Exec prefix where SDL is installed (optional)],
            sdl_exec_prefix="$withval", sdl_exec_prefix="")
AC_ARG_ENABLE(sdltest, [  --disable-sdltest       Do not try to compile and run a test SDL program],
		    , enable_sdltest=yes)

  if test x$sdl_exec_prefix != x ; then
    sdl_config_args="$sdl_config_args --exec-prefix=$sdl_exec_prefix"
    if test x${SDL_CONFIG+set} != xset ; then
      SDL_CONFIG=$sdl_exec_prefix/bin/sdl-config
    fi
  fi
  if test x$sdl_prefix != x ; then
    sdl_config_args="$sdl_config_args --prefix=$sdl_prefix"
    if test x${SDL_CONFIG+set} != xset ; then
      SDL_CONFIG=$sdl_prefix/bin/sdl-config
    fi
  fi

  as_save_PATH="$PATH"
  if test "x$prefix" != xNONE; then
    PATH="$prefix/bin:$prefix/usr/bin:$PATH"
  fi
  AC_PATH_PROG(SDL_CONFIG, sdl-config, no, [$PATH])
  PATH="$as_save_PATH"
  min_sdl_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for SDL - version >= $min_sdl_version)
  no_sdl=""
  if test "$SDL_CONFIG" = "no" ; then
    no_sdl=yes
  else
    SDL_CFLAGS=`$SDL_CONFIG $sdl_config_args --cflags`
    SDL_LIBS=`$SDL_CONFIG $sdl_config_args --libs`

    sdl_major_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    sdl_minor_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    sdl_micro_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_sdltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_CXXFLAGS="$CXXFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SDL_CFLAGS"
      CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SDL_LIBS"
dnl
dnl Now check if the installed SDL is sufficiently new. (Also sanity
dnl checks the results of sdl-config to some extent
dnl
      rm -f conf.sdltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.sdltest");
  */
  { FILE *fp = fopen("conf.sdltest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_sdl_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_sdl_version");
     exit(1);
   }

   if (($sdl_major_version > major) ||
      (($sdl_major_version == major) && ($sdl_minor_version > minor)) ||
      (($sdl_major_version == major) && ($sdl_minor_version == minor) && ($sdl_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'sdl-config --version' returned %d.%d.%d, but the minimum version\n", $sdl_major_version, $sdl_minor_version, $sdl_micro_version);
      printf("*** of SDL required is %d.%d.%d. If sdl-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If sdl-config was wrong, set the environment variable SDL_CONFIG\n");
      printf("*** to point to the correct copy of sdl-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_sdl=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       CXXFLAGS="$ac_save_CXXFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_sdl" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$SDL_CONFIG" = "no" ; then
       echo "*** The sdl-config script installed by SDL could not be found"
       echo "*** If SDL was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SDL_CONFIG environment variable to the"
       echo "*** full path to sdl-config."
     else
       if test -f conf.sdltest ; then
        :
       else
          echo "*** Could not run SDL test program, checking why..."
          CFLAGS="$CFLAGS $SDL_CFLAGS"
          CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
          LIBS="$LIBS $SDL_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "SDL.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SDL or finding the wrong"
          echo "*** version of SDL. If it is not finding SDL, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means SDL was incorrectly installed"
          echo "*** or that you have moved SDL since it was installed. In the latter case, you"
          echo "*** may want to edit the sdl-config script: $SDL_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          CXXFLAGS="$ac_save_CXXFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SDL_CFLAGS=""
     SDL_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SDL_CFLAGS)
  AC_SUBST(SDL_LIBS)
  rm -f conf.sdltest
])

# Copyright 2012, 2013, 2014 Brandon Invergo <brandon@invergo.net>
#
# This file is part of pyconfigure.  This program is free
# software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Under Section 7 of GPL version 3, you are granted additional
# permissions described in the Autoconf Configure Script Exception,
# version 3.0, as published by the Free Software Foundation.
#
# You should have received a copy of the GNU General Public License
# and a copy of the Autoconf Configure Script Exception along with
# this program; see the files COPYINGv3 and COPYING.EXCEPTION
# respectively.  If not, see <http://www.gnu.org/licenses/>.


# Many of these macros were adapted from ones written by Andrew Dalke
# and James Henstridge and are included with the Automake utility
# under the following copyright terms:
#
# Copyright (C) 1999-2012 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Table of Contents:
#
# 1. Language selection
#    and routines to produce programs in a given language.
#
# 2. Producing programs in a given language.
#
# 3. Looking for a compiler
#    And possibly the associated preprocessor.
#
# 4. Looking for specific libs & functionality


## ----------------------- ##
## 1. Language selection.  ##
## ----------------------- ##


# AC_LANG(Python)
# ---------------
AC_LANG_DEFINE([Python], [py], [PY], [PYTHON], [],
[ac_ext=py
ac_compile='chmod +x conftest.$ac_ext >&AS_MESSAGE_LOG_FD'
ac_link='chmod +x conftest.$ac_ext && cp conftest.$ac_ext conftest >&AS_MESSAGE_LOG_FD'
])


# AC_LANG_PYTHON
# --------------
AU_DEFUN([AC_LANG_PYTHON], [AC_LANG(Python)])


## ----------------------- ##
## 2. Producing programs.  ##
## ----------------------- ##


# AC_LANG_PROGRAM(Python)([PROLOGUE], [BODY])
# -------------------------------------------
m4_define([AC_LANG_PROGRAM(Python)], [dnl
@%:@!$PYTHON
$1
m4_if([$2], [], [], [dnl
if __name__ == '__main__':
$2])])


# _AC_LANG_IO_PROGRAM(Python)
# ---------------------------
# Produce source that performs I/O.
m4_define([_AC_LANG_IO_PROGRAM(Python)],
[AC_LANG_PROGRAM([dnl
import sys
try:
    h = open('conftest.out')
except:
    sys.exit(1)
else:
    close(h)
    sys.exit(0)
], [])])


# _AC_LANG_CALL(Python)([PROLOGUE], [FUNCTION])
# ---------------------
# Produce source that calls FUNCTION
m4_define([_AC_LANG_CALL(Python)],
[AC_LANG_PROGRAM([$1], [$2])])


## -------------------------------------------- ##
## 3. Looking for Compilers and Interpreters.   ##
## -------------------------------------------- ##


AC_DEFUN([AC_LANG_COMPILER(Python)],
[AC_REQUIRE([PC_PROG_PYTHON])])


# PC_INIT([MIN-VERSION], [MAX-VERSION]) 
# -----------------------------
# Initialize pyconfigure, finding a Python interpreter with a given
# minimum and/or maximum version. 
AC_DEFUN([PC_INIT],
[PC_PROG_PYTHON([], [$1], [$2])
dnl If we found something, do a sanity check that the interpreter really
dnl has the version its name would suggest.
m4_ifval([PYTHON],
        [PC_PYTHON_VERIFY_VERSION([>=], [pc_min_ver], [],
                  [AC_MSG_FAILURE([No compatible Python interpreter found. If you're sure that you have one, try setting the PYTHON environment variable to the location of the interpreter.])])])
m4_ifval([PYTHON],
        [PC_PYTHON_VERIFY_VERSION([<=], [pc_max_ver], [],
                  [AC_MSG_FAILURE([No compatible Python interpreter found. If you're sure that you have one, try setting the PYTHON environment variable to the location of the interpreter.])])])
])# PC_INIT

# PC_PROG_PYTHON([PROG-TO-CHECK-FOR], [MIN-VERSION], [MAX-VERSION])
# ---------------------------------
# Find a Python interpreter.  Python versions prior to 2.0 are not
# supported. (2.0 was released on October 16, 2000).
AC_DEFUN_ONCE([PC_PROG_PYTHON],
[AC_ARG_VAR([PYTHON], [the Python interpreter])
dnl The default minimum version is 2.0
m4_define_default([pc_min_ver], m4_ifval([$2], [$2], [2.0]))
dnl The default maximum version is 3.3
m4_define_default([pc_max_ver], m4_ifval([$3], [$3], [4.0]))
dnl Build up a list of possible interpreter names. 
m4_define_default([_PC_PYTHON_INTERPRETER_LIST],
    [[python] \
dnl If we want some Python 3 versions (max version >= 3.0), 
dnl also search for "python3"
     m4_if(m4_version_compare(pc_max_ver, [2.9]), [1], [python3], []) \
dnl If we want some Python 2 versions (min version <= 2.7),
dnl also search for "python2".
     m4_if(m4_version_compare(pc_min_ver, [2.8]), [-1], [python2], []) \
dnl Construct a comma-separated list of interpreter names (python2.6, 
dnl python2.7, etc). We only care about the first 3 characters of the
dnl version strings (major-dot-minor; not 
dnl major-dot-minor-dot-bugfix[-dot-whatever])
     m4_foreach([pc_ver], 
                    m4_esyscmd_s(seq -s[[", "]] -f["[[%.1f]]"] m4_substr(pc_max_ver, [0], [3]) -0.1 m4_substr(pc_min_ver, [0], [3])),
dnl Remove python2.8 and python2.9 since they will never exist
                    [m4_bmatch(pc_ver, [2.[89]], [], [python]pc_ver)])])
dnl Do the actual search at last.
m4_ifval([$1],
	[AC_PATH_PROGS(PYTHON, [$1 _PC_PYTHON_INTERPRETER_LIST])],
	[AC_PATH_PROGS(PYTHON, [_PC_PYTHON_INTERPRETER_LIST])])
])# PC_PROG_PYTHON
  

# PC_PYTHON_PROG_PYTHON_CONFIG(PROG-TO-CHECK-FOR)
# ----------------------------------------------
# Find the python-config program
AC_DEFUN([PC_PYTHON_PROG_PYTHON_CONFIG],
[AC_REQUIRE([PC_PROG_PYTHON])[]dnl
AC_ARG_VAR([PYTHON_CONFIG], [the Python-config program])
dnl python-config's binary name is normally based on the Python interpreter's
dnl binary name (i.e. python2.7 -> python2.7-config)
m4_define([_PYTHON_BASENAME], [`basename $PYTHON`])
m4_ifval([$1],
	[AC_PATH_PROGS(PYTHON_CONFIG, [$1 _PYTHON_BASENAME-config])],
	[AC_PATH_PROG(PYTHON_CONFIG, _PYTHON_BASENAME-config)])
]) # PC_PYTHON_PROG_PYTHON_CONFIG


# PC_PYTHON_VERIFY_VERSION([RELATION], [VERSION], [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ---------------------------------------------------------------------------
# Run ACTION-IF-TRUE if the Python interpreter PROG has version [RELATION] VERSION.
# i.e if RELATION is "<", check if PROG has a version number less than VERSION.
# Run ACTION-IF-FALSE otherwise.
# Specify RELATION as any mathematical comparison "<", ">", "<=", ">=", "==" or "!="
# This test uses sys.hexversion instead of the string equivalent (first
# word of sys.version), in order to cope with versions such as 2.2c1.
# This supports Python 2.0 or higher. (2.0 was released on October 16, 2000).
AC_DEFUN([PC_PYTHON_VERIFY_VERSION],
[m4_define([pc_python_safe_ver], m4_bpatsubsts($2, [\.], [_]))
AC_CACHE_CHECK([if Python $1 '$2'],
    [[pc_cv_python_req_version_]pc_python_safe_ver],
    [AC_LANG_PUSH(Python)[]dnl
     AC_RUN_IFELSE(
        [AC_LANG_PROGRAM([dnl
import sys
], [dnl
    # split strings by '.' and convert to numeric.  Append some zeros
    # because we need at least 4 digits for the hex conversion.
    # map returns an iterator in Python 3.0 and a list in 2.x
    reqver = list(map(int, '$2'.split('.'))) + [[0, 0, 0]]
    reqverhex = 0
    # xrange is not present in Python 3.0 and range returns an iterator
    for i in list(range(4)):
        reqverhex = (reqverhex << 8) + reqver[[i]]
    # the final 8 bits are "0xf0" for final versions, which are all
    # we'll test against, since it's doubtful that a released software
    # will depend on an alpha- or beta-state Python.
    reqverhex += 0xf0
    if sys.hexversion $1 reqverhex:
        sys.exit()
    else:
        sys.exit(1)
])], 
         [[pc_cv_python_req_version_]pc_python_safe_ver=yes], 
         [[pc_cv_python_req_version_]pc_python_safe_ver=no])
     AC_LANG_POP(Python)[]dnl
    ])
AS_IF([test "$[pc_cv_python_req_version_]pc_python_safe_ver" = "no"], [$4], [$3])
])# PC_PYTHON_VERIFY_VERSION


# PC_PYTHON_CHECK_VERSION
# -----------------------
# Query Python for its version number.  Getting [:3] seems to be
# the best way to do this; it's what "site.py" does in the standard
# library.
AC_DEFUN([PC_PYTHON_CHECK_VERSION],
[AC_REQUIRE([PC_PROG_PYTHON])[]dnl
AC_CACHE_CHECK([for $1 version], 
    [pc_cv_python_version],
    [AC_LANG_PUSH(Python)[]dnl
     AC_LINK_IFELSE([AC_LANG_PROGRAM([dnl
import sys
], [dnl
    sys.stdout.write(sys.version[[:3]])
])],
                   [pc_cv_python_version=`./conftest`],
                   [AC_MSG_FAILURE([failed to run Python program])])
     AC_LANG_POP(Python)[]dnl
    ])
AC_SUBST([PYTHON_VERSION], [$pc_cv_python_version])
])# PC_PYTHON_CHECK_VERSION


# PC_PYTHON_CHECK_PREFIX
# ----------------------
# Use the value of $prefix for the corresponding value of
# PYTHON_PREFIX. This is made a distinct variable so it can be
# overridden if need be.  However, general consensus is that you
# shouldn't need this ability. 
AC_DEFUN([PC_PYTHON_CHECK_PREFIX],
[AC_REQUIRE([PC_PYTHON_PROG_PYTHON_CONFIG])[]dnl
dnl Try to get it with python-config otherwise do it from within Python
AC_CACHE_CHECK([for Python prefix], [pc_cv_python_prefix],
[if test -x "$PYTHON_CONFIG"; then
    pc_cv_python_prefix=`$PYTHON_CONFIG --prefix 2>&AS_MESSAGE_LOG_FD`
else
    AC_LANG_PUSH(Python)[]dnl
    AC_LINK_IFELSE([AC_LANG_PROGRAM([dnl
import sys
], [dnl
    sys.stdout.write(sys.prefix)
])], [pc_cv_python_prefix=`./conftest`;
      if test $? != 0; then
         AC_MSG_FAILURE([could not determine Python prefix])
      fi],
      [AC_MSG_FAILURE([failed to run Python program])])
    AC_LANG_POP(Python)[]dnl
fi])
AC_SUBST([PYTHON_PREFIX], [$pc_cv_python_prefix])])


# PC_PYTHON_CHECK_EXEC_PREFIX
# --------------------------
# Like above, but for $exec_prefix
AC_DEFUN([PC_PYTHON_CHECK_EXEC_PREFIX],
[AC_REQUIRE([PC_PYTHON_PROG_PYTHON_CONFIG])[]dnl
dnl Try to get it with python-config otherwise do it from within Python
AC_CACHE_CHECK([for Python exec-prefix], [pc_cv_python_exec_prefix],
[if test -x "$PYTHON_CONFIG"; then
    pc_cv_python_exec_prefix=`$PYTHON_CONFIG --exec-prefix 2>&AS_MESSAGE_LOG_FD`
else
    AC_LANG_PUSH(Python)[]dnl
    AC_LINK_IFELSE([AC_LANG_PROGRAM([dnl
import sys
], [dnl
    sys.stdout.write(sys.exec_prefix)
])],
        [pc_cv_python_exec_prefix=`./conftest`;
         if test $? != 0; then
            AC_MSG_FAILURE([could not determine Python exec_prefix])
         fi],
         [AC_MSG_FAILURE([failed to run Python program])])
    AC_LANG_POP(Python)[]dnl
fi
])
AC_SUBST([PYTHON_EXEC_PREFIX], [$pc_cv_python_exec_prefix])])


# PC_PYTHON_CHECK_INCLUDES
# ------------------------
# Find the Python header file include flags (ie
# '-I/usr/include/python')
AC_DEFUN([PC_PYTHON_CHECK_INCLUDES],
[AC_REQUIRE([PC_PYTHON_PROG_PYTHON_CONFIG])[]dnl
dnl Try to find the headers location with python-config otherwise guess
AC_CACHE_CHECK([for Python includes], [pc_cv_python_includes],
[if test -x "$PYTHON_CONFIG"; then
    pc_cv_python_includes=`$PYTHON_CONFIG --includes 2>&AS_MESSAGE_LOG_FD`
else
    pc_cv_python_includes="[-I$includedir/$_PYTHON_BASENAME]m4_ifdef(PYTHON_ABI_FLAGS,
    PYTHON_ABI_FLAGS,)"
fi
])
AC_SUBST([PYTHON_INCLUDES], [$pc_cv_python_includes])])


# PC_PYTHON_CHECK_HEADERS([ACTION-IF-PRESENT], [ACTION-IF-ABSENT])
# -----------------------
# Check for the presence and usability of Python.h
AC_DEFUN([PC_PYTHON_CHECK_HEADERS],
[AC_REQUIRE([PC_PYTHON_CHECK_INCLUDES])[]dnl
pc_cflags_store=$CPPFLAGS
CPPFLAGS="$CFLAGS $PYTHON_INCLUDES"
AC_CHECK_HEADER([Python.h], [$1], [$2])
CPPFLAGS=$pc_cflags_store
])


# PC_PYTHON_CHECK_LIBS
# --------------------
# Find the Python lib flags (ie '-lpython')
AC_DEFUN([PC_PYTHON_CHECK_LIBS],
[AC_REQUIRE([PC_PYTHON_PROG_PYTHON_CONFIG])[]dnl
dnl Try to find the lib flags with python-config otherwise guess
AC_CACHE_CHECK([for Python libs], [pc_cv_python_libs],
[if test -x "$PYTHON_CONFIG"; then
    pc_cv_python_libs=`$PYTHON_CONFIG --libs 2>&AS_MESSAGE_LOG_FD`
else
    pc_cv_python_libs="[-l$_PYTHON_BASENAME]m4_ifdef(PYTHON_ABI_FLAGS, PYTHON_ABI_FLAGS,)"
fi
])
AC_SUBST([PYTHON_LIBS], [$pc_cv_python_libs])])


# PC_PYTHON_TEST_LIBS(LIBRARY-FUNCTION, [ACTION-IF-PRESENT], [ACTION-IF-ABSENT])
# -------------------
# Verify that the Python libs can be loaded
AC_DEFUN([PC_PYTHON_TEST_LIBS],
[AC_REQUIRE([PC_PYTHON_CHECK_LIBS])[]dnl
pc_libflags_store=$LIBS
for lflag in $PYTHON_LIBS; do
    case $lflag in
    	 -lpython*@:}@
		LIBS="$LIBS $lflag"
		pc_libpython=`echo $lflag | sed -e 's/^-l//'`
		;;
         *@:}@;;
    esac
done
AC_CHECK_LIB([$pc_libpython], [$1], [$2], [$3])])


# PC_PYTHON_CHECK_CFLAGS
# ----------------------
# Find the Python CFLAGS
AC_DEFUN([PC_PYTHON_CHECK_CFLAGS],
[AC_REQUIRE([PC_PYTHON_PROG_PYTHON_CONFIG])[]dnl
dnl Try to find the CFLAGS with python-config otherwise give up
AC_CACHE_CHECK([for Python CFLAGS], [pc_cv_python_cflags],
[if test -x "$PYTHON_CONFIG"; then
    pc_cv_python_cflags=`$PYTHON_CONFIG --cflags 2>&AS_MESSAGE_LOG_FD`
else
    pc_cv_python_cflags=
fi
])
AC_SUBST([PYTHON_CFLAGS], [$pc_cv_python_cflags])])


# PC_PYTHON_CHECK_LDFLAGS
# -----------------------
# Find the Python LDFLAGS
AC_DEFUN([PC_PYTHON_CHECK_LDFLAGS],
[AC_REQUIRE([PC_PYTHON_PROG_PYTHON_CONFIG])[]dnl
dnl Try to find the LDFLAGS with python-config otherwise give up
AC_CACHE_CHECK([for Python LDFLAGS], [pc_cv_python_ldflags],
[if test -x "$PYTHON_CONFIG"; then
    pc_cv_python_ldflags=`$PYTHON_CONFIG --ldflags 2>&AS_MESSAGE_LOG_FD`
else
    pc_cv_python_ldflags=
fi
])
AC_SUBST([PYTHON_LDFLAGS], [$pc_cv_python_ldflags])])


# PC_PYTHON_CHECK_EXTENSION_SUFFIX
# --------------------------------
# Find the Python extension suffix (i.e. '.cpython-32.so')
AC_DEFUN([PC_PYTHON_CHECK_EXTENSION_SUFFIX],
[AC_REQUIRE([PC_PYTHON_PROG_PYTHON_CONFIG])[]dnl
dnl Try to find the suffix with python-config otherwise give up
AC_CACHE_CHECK([for Python extension suffix], [pc_cv_python_extension_suffix],
[if test -x "$PYTHON_CONFIG"; then
     pc_cv_python_extension_suffix=`$PYTHON_CONFIG --extension-suffix 2>&AS_MESSAGE_LOG_FD`
else
    pc_cv_python_extension_suffix=
fi
])
AC_SUBST([PYTHON_EXTENSION_SUFFIX], [$pc_cv_python_extension_suffix])])


# PC_PYTHON_CHECK_ABI_FLAGS
# -------------------------
# Find the Python ABI flags
AC_DEFUN([PC_PYTHON_CHECK_ABI_FLAGS],
[AC_REQUIRE([PC_PYTHON_PROG_PYTHON_CONFIG])[]dnl
dnl Try to find the ABI flags with python-config otherwise give up
AC_CACHE_CHECK([for Python ABI flags], [pc_cv_python_abi_flags],
[if test -x "$PYTHON_CONFIG"; then
     pc_cv_python_abi_flags=`$PYTHON_CONFIG --abiflags 2>&AS_MESSAGE_LOG_FD`
else
    pc_cv_python_abi_flags=
fi
])
AC_SUBST([PYTHON_ABI_FLAGS], [$pc_cv_python_abi_flags])])


# PC_PYTHON_CHECK_PLATFORM
# ------------------------
# At times (like when building shared libraries) you may want
# to know which OS platform Python thinks this is.
AC_DEFUN([PC_PYTHON_CHECK_PLATFORM],
[AC_REQUIRE([PC_PROG_PYTHON])[]dnl
dnl Get the platform from within Python (sys.platform)
AC_CACHE_CHECK([for Python platform], [pc_cv_python_platform],
    [AC_LANG_PUSH(Python)[]dnl
     AC_LINK_IFELSE([AC_LANG_PROGRAM([dnl
import sys
], [dnl
    sys.stdout.write(sys.platform)
])], [pc_cv_python_platform=`./conftest`;
     if test $? != 0; then
        AC_MSG_FAILURE([could not determine Python platform])
     fi],
     [AC_MSG_FAILURE([failed to run Python program])])
    AC_LANG_POP(Python)[]dnl
   ])
AC_SUBST([PYTHON_PLATFORM], [$pc_cv_python_platform])
])


# PC_PYTHON_CHECK_SITE_DIR
# ---------------------
# The directory to which new libraries are installed (i.e. the
# "site-packages" directory.
AC_DEFUN([PC_PYTHON_CHECK_SITE_DIR],
[AC_REQUIRE([PC_PROG_PYTHON])AC_REQUIRE([PC_PYTHON_CHECK_PREFIX])[]dnl
AC_CACHE_CHECK([for Python site-packages directory],
    [pc_cv_python_site_dir],
    [AC_LANG_PUSH(Python)[]dnl
    if test "x$prefix" = xNONE
     then
       pc_py_prefix=$ac_default_prefix
     else
       pc_py_prefix=$prefix
     fi
     AC_LINK_IFELSE([AC_LANG_PROGRAM([dnl
import sys
from platform import python_implementation
# sysconfig in CPython 2.7 doesn't work in virtualenv
# <https://github.com/pypa/virtualenv/issues/118>
try:
    import sysconfig
except:
    can_use_sysconfig = False
else:
    can_use_sysconfig = True
if can_use_sysconfig:
    if python_implementation() == "CPython" and sys.version[[:3]] == '2.7':
        can_use_sysconfig = False
if not can_use_sysconfig:        
    from distutils import sysconfig
    sitedir = sysconfig.get_python_lib(False, False, prefix='$pc_py_prefix')
else:
    sitedir = sysconfig.get_path('purelib', vars={'base':'$pc_py_prefix'})
], [dnl
    sys.stdout.write(sitedir)
])], [pc_cv_python_site_dir=`./conftest`],
     [AC_MSG_FAILURE([failed to run Python program])])
     AC_LANG_POP(Python)[]dnl
     case $pc_cv_python_site_dir in
     $pc_py_prefix*)
       pc__strip_prefix=`echo "$pc_py_prefix" | sed 's|.|.|g'`
       pc_cv_python_site_dir=`echo "$pc_cv_python_site_dir" | sed "s,^$pc__strip_prefix/,,"`
       ;;
     *)
       case $pc_py_prefix in
         /usr|/System*) ;;
         *)
	  pc_cv_python_site_dir=lib/python$PYTHON_VERSION/site-packages
	  ;;
       esac
       ;;
     esac
     ])
AC_SUBST([pythondir], [\${prefix}/$pc_cv_python_site_dir])])# PC_PYTHON_CHECK_SITE_DIR

# PC_PYTHON_SITE_PACKAGE_DIR
# --------------------------
# $PACKAGE directory under PYTHON_SITE_DIR
AC_DEFUN([PC_PYTHON_SITE_PACKAGE_DIR],
[AC_REQUIRE([PC_PYTHON_CHECK_SITE_DIR])[]dnl
AC_SUBST([pkgpythondir], [\${pythondir}/$PACKAGE_NAME])])


# PC_PYTHON_CHECK_EXEC_DIR
# ------------------------
# directory for installing python extension modules (shared libraries)
AC_DEFUN([PC_PYTHON_CHECK_EXEC_DIR],
[AC_REQUIRE([PC_PROG_PYTHON])AC_REQUIRE([PC_PYTHON_CHECK_EXEC_PREFIX])[]dnl
  AC_CACHE_CHECK([for Python extension module directory],
    [pc_cv_python_exec_dir],
    [AC_LANG_PUSH(Python)[]dnl
    if test "x$pc_cv_python_exec_prefix" = xNONE
     then
       pc_py_exec_prefix=$pc_cv_python_prefix
     else
       pc_py_exec_prefix=$pc_cv_python_exec_prefix
     fi
     AC_LINK_IFELSE([AC_LANG_PROGRAM([dnl
import sys
from platform import python_implementation
# sysconfig in CPython 2.7 doesn't work in virtualenv
# <https://github.com/pypa/virtualenv/issues/118>
try:
    import sysconfig
except:
    can_use_sysconfig = False
else:
    can_use_sysconfig = True
if can_use_sysconfig:
    if python_implementation() == "CPython" and sys.version[[:3]] == '2.7':
        can_use_sysconfig = False
if not can_use_sysconfig:        
    from distutils import sysconfig
    sitedir = sysconfig.get_python_lib(False, False, prefix='$pc_py__exec_prefix')
else:
    sitedir = sysconfig.get_path('purelib', vars={'platbase':'$pc_py_exec_prefix'})
], [dnl
    sys.stdout.write(sitedir)
])], [pc_cv_python_exec_dir=`./conftest`],
     [AC_MSG_FAILURE([failed to run Python program])])
     AC_LANG_POP(Python)[]dnl
     case $pc_cv_python_exec_dir in
     $pc_py_exec_prefix*)
       pc__strip_prefix=`echo "$pc_py_exec_prefix" | sed 's|.|.|g'`
       pc_cv_python_exec_dir=`echo "$pc_cv_python_exec_dir" | sed "s,^$pc__strip_prefix/,,"`
       ;;
     *)
       case $pc_py_exec_prefix in
         /usr|/System*) ;;
         *)
	   pc_cv_python_exec_dir=lib/python$PYTHON_VERSION/site-packages
	   ;;
       esac
       ;;
     esac
    ])
AC_SUBST([pyexecdir], [\${exec_prefix}/$pc_cv_python_pyexecdir])]) #PY_PYTHON_CHECK_EXEC_LIB_DIR


# PC_PYTHON_EXEC_PACKAGE_DIR
# --------------------------
# $PACKAGE directory under PYTHON_SITE_DIR
AC_DEFUN([PC_PYTHON_EXEC_PACKAGE_DIR],
[AC_REQUIRE([PC_PYTHON_CHECK_EXEC_DIR])[]dnl
AC_SUBST([pkgpyexecdir], [\${pyexecdir}/$PACKAGE_NAME])])


## -------------------------------------------- ##
## 4. Looking for specific libs & functionality ##
## -------------------------------------------- ##


# PC_PYTHON_CHECK_MODULE(LIBRARY, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ----------------------------------------------------------------------
# Macro for checking if a Python library is installed
AC_DEFUN([PC_PYTHON_CHECK_MODULE],
[AC_REQUIRE([PC_PROG_PYTHON])[]dnl
m4_define([pc_python_safe_mod], m4_bpatsubsts($1, [\.], [_]))
AC_CACHE_CHECK([for Python '$1' library],
    [[pc_cv_python_module_]pc_python_safe_mod],
    [AC_LANG_PUSH(Python)[]dnl
     AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([dnl
import sys
try:
    import $1
except:
    sys.exit(1)
else:
    sys.exit(0)
], [])],
	[[pc_cv_python_module_]pc_python_safe_mod="yes"],
	[[pc_cv_python_module_]pc_python_safe_mod="no"])
     AC_LANG_POP(Python)[]dnl
    ])
AS_IF([test "$[pc_cv_python_module_]pc_python_safe_mod" = "no"], [$3], [$2])
])# PC_PYTHON_CHECK_MODULE


# PC_PYTHON_CHECK_FUNC([LIBRARY], FUNCTION, ARGS, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ---------------------------------------------------------------------------------------
# Check to see if a given function call, optionally from a module, can
# be successfully called
AC_DEFUN([PC_PYTHON_CHECK_FUNC],
[AC_REQUIRE([PC_PROG_PYTHON])[]dnl
m4_define([pc_python_safe_mod], m4_bpatsubsts($1, [\.], [_]))
AC_CACHE_CHECK([for Python m4_ifnblank($1, '$1.$2()', '$2()') function],
    [[pc_cv_python_func_]pc_python_safe_mod[_$2]],
    [AC_LANG_PUSH(Python)[]dnl
     AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([dnl
import sys
m4_ifnblank([$1], [dnl
try:
    import $1
except:
    sys.exit(1)
], [])], 
[
m4_ifnblank([$1], [
    try:
        $1.$2($3)], [
    try:
        $2($3)])
    except:
        sys.exit(1)
    else:
        sys.exit(0)
])],
	[[pc_cv_python_func_]pc_python_safe_mod[_$2]="yes"],
	[[pc_cv_python_func_]pc_python_safe_mod[_$2]="no"])
     AC_LANG_POP(Python)[]dnl
    ])
AS_IF([test "$[pc_cv_python_func_]pc_python_safe_mod[_$2]" = "no"], [$5], [$4])
])# PC_PYTHON_CHECK_FUNC

# Copyright (C) 1999-2017 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.


# AM_PATH_PYTHON([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ---------------------------------------------------------------------------
# Adds support for distributing Python modules and packages.  To
# install modules, copy them to $(pythondir), using the python_PYTHON
# automake variable.  To install a package with the same name as the
# automake package, install to $(pkgpythondir), or use the
# pkgpython_PYTHON automake variable.
#
# The variables $(pyexecdir) and $(pkgpyexecdir) are provided as
# locations to install python extension modules (shared libraries).
# Another macro is required to find the appropriate flags to compile
# extension modules.
#
# If your package is configured with a different prefix to python,
# users will have to add the install directory to the PYTHONPATH
# environment variable, or create a .pth file (see the python
# documentation for details).
#
# If the MINIMUM-VERSION argument is passed, AM_PATH_PYTHON will
# cause an error if the version of python installed on the system
# doesn't meet the requirement.  MINIMUM-VERSION should consist of
# numbers and dots only.
AC_DEFUN([AM_PATH_PYTHON],
 [
  dnl Find a Python interpreter.  Python versions prior to 2.0 are not
  dnl supported. (2.0 was released on October 16, 2000).
  dnl FIXME: Remove the need to hard-code Python versions here.
  m4_define_default([_AM_PYTHON_INTERPRETER_LIST],
[python python2 python3 python3.8 python3.7 python3.6 python3.5 python3.4 python3.3 python3.2 python3.1 python3.0 python2.7 dnl
 python2.6 python2.5 python2.4 python2.3 python2.2 python2.1 python2.0])

  AC_ARG_VAR([PYTHON], [the Python interpreter])

  m4_if([$1],[],[
    dnl No version check is needed.
    # Find any Python interpreter.
    if test -z "$PYTHON"; then
      AC_PATH_PROGS([PYTHON], _AM_PYTHON_INTERPRETER_LIST, :)
    fi
    am_display_PYTHON=python
  ], [
    dnl A version check is needed.
    if test -n "$PYTHON"; then
      # If the user set $PYTHON, use it and don't search something else.
      AC_MSG_CHECKING([whether $PYTHON version is >= $1])
      AM_PYTHON_CHECK_VERSION([$PYTHON], [$1],
			      [AC_MSG_RESULT([yes])],
			      [AC_MSG_RESULT([no])
			       AC_MSG_ERROR([Python interpreter is too old])])
      am_display_PYTHON=$PYTHON
    else
      # Otherwise, try each interpreter until we find one that satisfies
      # VERSION.
      AC_CACHE_CHECK([for a Python interpreter with version >= $1],
	[am_cv_pathless_PYTHON],[
	for am_cv_pathless_PYTHON in _AM_PYTHON_INTERPRETER_LIST none; do
	  test "$am_cv_pathless_PYTHON" = none && break
	  AM_PYTHON_CHECK_VERSION([$am_cv_pathless_PYTHON], [$1], [break])
	done])
      # Set $PYTHON to the absolute path of $am_cv_pathless_PYTHON.
      if test "$am_cv_pathless_PYTHON" = none; then
	PYTHON=:
      else
        AC_PATH_PROG([PYTHON], [$am_cv_pathless_PYTHON])
      fi
      am_display_PYTHON=$am_cv_pathless_PYTHON
    fi
  ])

  if test "$PYTHON" = :; then
  dnl Run any user-specified action, or abort.
    m4_default([$3], [AC_MSG_ERROR([no suitable Python interpreter found])])
  else

  dnl Query Python for its version number.  Getting [:3] seems to be
  dnl the best way to do this; it's what "site.py" does in the standard
  dnl library.

  AC_CACHE_CHECK([for $am_display_PYTHON version], [am_cv_python_version],
    [am_cv_python_version=`$PYTHON -c "import sys; sys.stdout.write(sys.version[[:3]])"`])
  AC_SUBST([PYTHON_VERSION], [$am_cv_python_version])

  dnl Use the values of $prefix and $exec_prefix for the corresponding
  dnl values of PYTHON_PREFIX and PYTHON_EXEC_PREFIX.  These are made
  dnl distinct variables so they can be overridden if need be.  However,
  dnl general consensus is that you shouldn't need this ability.

  AC_SUBST([PYTHON_PREFIX], ['${prefix}'])
  AC_SUBST([PYTHON_EXEC_PREFIX], ['${exec_prefix}'])

  dnl At times (like when building shared libraries) you may want
  dnl to know which OS platform Python thinks this is.

  AC_CACHE_CHECK([for $am_display_PYTHON platform], [am_cv_python_platform],
    [am_cv_python_platform=`$PYTHON -c "import sys; sys.stdout.write(sys.platform)"`])
  AC_SUBST([PYTHON_PLATFORM], [$am_cv_python_platform])

  # Just factor out some code duplication.
  am_python_setup_sysconfig="\
import sys
# Prefer sysconfig over distutils.sysconfig, for better compatibility
# with python 3.x.  See automake bug#10227.
try:
    import sysconfig
except ImportError:
    can_use_sysconfig = 0
else:
    can_use_sysconfig = 1
# Can't use sysconfig in CPython 2.7, since it's broken in virtualenvs:
# <https://github.com/pypa/virtualenv/issues/118>
try:
    from platform import python_implementation
    if python_implementation() == 'CPython' and sys.version[[:3]] == '2.7':
        can_use_sysconfig = 0
except ImportError:
    pass"

  dnl Set up 4 directories:

  dnl pythondir -- where to install python scripts.  This is the
  dnl   site-packages directory, not the python standard library
  dnl   directory like in previous automake betas.  This behavior
  dnl   is more consistent with lispdir.m4 for example.
  dnl Query distutils for this directory.
  AC_CACHE_CHECK([for $am_display_PYTHON script directory],
    [am_cv_python_pythondir],
    [if test "x$prefix" = xNONE
     then
       am_py_prefix=$ac_default_prefix
     else
       am_py_prefix=$prefix
     fi
     am_cv_python_pythondir=`$PYTHON -c "
$am_python_setup_sysconfig
if can_use_sysconfig:
    sitedir = sysconfig.get_path('purelib', vars={'base':'$am_py_prefix'})
else:
    from distutils import sysconfig
    sitedir = sysconfig.get_python_lib(0, 0, prefix='$am_py_prefix')
sys.stdout.write(sitedir)"`
     case $am_cv_python_pythondir in
     $am_py_prefix*)
       am__strip_prefix=`echo "$am_py_prefix" | sed 's|.|.|g'`
       am_cv_python_pythondir=`echo "$am_cv_python_pythondir" | sed "s,^$am__strip_prefix,$PYTHON_PREFIX,"`
       ;;
     *)
       case $am_py_prefix in
         /usr|/System*) ;;
         *)
	  am_cv_python_pythondir=$PYTHON_PREFIX/lib/python$PYTHON_VERSION/site-packages
	  ;;
       esac
       ;;
     esac
    ])
  AC_SUBST([pythondir], [$am_cv_python_pythondir])

  dnl pkgpythondir -- $PACKAGE directory under pythondir.  Was
  dnl   PYTHON_SITE_PACKAGE in previous betas, but this naming is
  dnl   more consistent with the rest of automake.

  AC_SUBST([pkgpythondir], [\${pythondir}/$PACKAGE])

  dnl pyexecdir -- directory for installing python extension modules
  dnl   (shared libraries)
  dnl Query distutils for this directory.
  AC_CACHE_CHECK([for $am_display_PYTHON extension module directory],
    [am_cv_python_pyexecdir],
    [if test "x$exec_prefix" = xNONE
     then
       am_py_exec_prefix=$am_py_prefix
     else
       am_py_exec_prefix=$exec_prefix
     fi
     am_cv_python_pyexecdir=`$PYTHON -c "
$am_python_setup_sysconfig
if can_use_sysconfig:
    sitedir = sysconfig.get_path('platlib', vars={'platbase':'$am_py_prefix'})
else:
    from distutils import sysconfig
    sitedir = sysconfig.get_python_lib(1, 0, prefix='$am_py_prefix')
sys.stdout.write(sitedir)"`
     case $am_cv_python_pyexecdir in
     $am_py_exec_prefix*)
       am__strip_prefix=`echo "$am_py_exec_prefix" | sed 's|.|.|g'`
       am_cv_python_pyexecdir=`echo "$am_cv_python_pyexecdir" | sed "s,^$am__strip_prefix,$PYTHON_EXEC_PREFIX,"`
       ;;
     *)
       case $am_py_exec_prefix in
         /usr|/System*) ;;
         *)
	   am_cv_python_pyexecdir=$PYTHON_EXEC_PREFIX/lib/python$PYTHON_VERSION/site-packages
	   ;;
       esac
       ;;
     esac
    ])
  AC_SUBST([pyexecdir], [$am_cv_python_pyexecdir])

  dnl pkgpyexecdir -- $(pyexecdir)/$(PACKAGE)

  AC_SUBST([pkgpyexecdir], [\${pyexecdir}/$PACKAGE])

  dnl Run any user-specified action.
  $2
  fi

])


# AM_PYTHON_CHECK_VERSION(PROG, VERSION, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ---------------------------------------------------------------------------
# Run ACTION-IF-TRUE if the Python interpreter PROG has version >= VERSION.
# Run ACTION-IF-FALSE otherwise.
# This test uses sys.hexversion instead of the string equivalent (first
# word of sys.version), in order to cope with versions such as 2.2c1.
# This supports Python 2.0 or higher. (2.0 was released on October 16, 2000).
AC_DEFUN([AM_PYTHON_CHECK_VERSION],
 [prog="import sys
# split strings by '.' and convert to numeric.  Append some zeros
# because we need at least 4 digits for the hex conversion.
# map returns an iterator in Python 3.0 and a list in 2.x
minver = list(map(int, '$2'.split('.'))) + [[0, 0, 0]]
minverhex = 0
# xrange is not present in Python 3.0 and range returns an iterator
for i in list(range(0, 4)): minverhex = (minverhex << 8) + minver[[i]]
sys.exit(sys.hexversion < minverhex)"
  AS_IF([AM_RUN_LOG([$1 -c "$prog"])], [$3], [$4])])

# Copyright (C) 2001-2017 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_RUN_LOG(COMMAND)
# -------------------
# Run COMMAND, save the exit status in ac_status, and log it.
# (This has been adapted from Autoconf's _AC_RUN_LOG macro.)
AC_DEFUN([AM_RUN_LOG],
[{ echo "$as_me:$LINENO: $1" >&AS_MESSAGE_LOG_FD
   ($1) >&AS_MESSAGE_LOG_FD 2>&AS_MESSAGE_LOG_FD
   ac_status=$?
   echo "$as_me:$LINENO: \$? = $ac_status" >&AS_MESSAGE_LOG_FD
   (exit $ac_status); }])

m4_include([acinclude.m4])
