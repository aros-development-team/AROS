dnl Some autoconf macros for AROS
dnl
dnl Search for a file, and place the result into the cache.

dnl AROS_REQUIRED(name,var)
dnl Checks if var is defined, and aborts otherwise
AC_DEFUN(AROS_REQUIRED,
[if test "$2" = ""; then
    AC_MSG_ERROR($1 is required to build AROS. Please install and run configure again.)
fi])

dnl AROS_PROG(var,prog,args)
AC_DEFUN(AROS_PROG,
[AC_CHECK_PROG([$1],[$2],[$2])
ifelse($3, ,, $1="$$1 $3")
AC_SUBST($1)])

dnl AROS_TOOL(var,prog,args)
dnl This will later on check the $target-$(tool) stuff, but at the
dnl moment it only does the same as AROS_PROG
dnl
AC_DEFUN(AROS_TOOL,
[AC_PATH_PROG([$1],[$2],[$2])
ifelse($3, ,, $1="$$1 $3")
AC_SUBST($1)])

dnl AROS_TOOL_CC(var,prog,args)
dnl This is effectively the same as AROS_TOOL, but only does the 
dnl test when we are cross compiling.
dnl
AC_DEFUN(AROS_TOOL_CC,
[if test "$cross_compile" = "yes" ; then
    AC_PATH_PROG([$1],[$2],[$2])
else
    $1="$2"
fi
ifelse($3, ,, $1="$$1 $3")
AC_SUBST($1)])

dnl AROS_CACHE_CHECK(message, var, check)
dnl This is similar to the AC_CACHE_CHECK macro, but it hides the
dnl prefix and stuff from the coders. We will get aros_$2 on the
dnl variable, and aros_cv_$2 on the cache variable.
AC_DEFUN(AROS_CACHE_CHECK,
[AC_MSG_CHECKING([$1])
AC_CACHE_VAL(aros_cv_[$2],
[$3
aros_cv_[$2]="[$]aros_[$2]"
])
aros_[$2]="[$]aros_cv_[$2]"
AC_MSG_RESULT([$]aros_$2)])
