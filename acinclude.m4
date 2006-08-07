dnl Some autoconf macros for AROS
dnl
dnl Search for a file, and place the result into the cache.

dnl AROS_REQUIRED(name,var)
dnl Checks if var is defined, and aborts otherwise. name is just for
dnl presentation to the user.
dnl
AC_DEFUN(AROS_REQUIRED,
[if test "$2" = ""; then
    AC_MSG_ERROR($1 is required to build AROS. Please install and run configure again.)
fi])

dnl AROS_PROG(var,prog,args)
dnl Checks if prog is on the path. If it is, sets var to "prog args".
dnl args is optional.
dnl
AC_DEFUN(AROS_PROG,
[AC_CHECK_PROG([$1],[$2],[$2 $3])])

dnl AROS_TOOL_CCPATH(var,prog)
dnl This will first look for the tool in the CC path and then in the
dnl normal path (CC path only supported for gcc at the moment)
AC_DEFUN(AROS_TOOL_CCPATH,
[if test "$GCC" = "yes"; then
    aros_gcc_[$2]=`$CC -print-prog-name=[$2]`
    AC_PATH_PROG([$1], [`basename $aros_gcc_[$2]`], , [`dirname $aros_gcc_[$2]`])
fi
if test "$[$1]" = ""; then
    AC_PATH_PROG([$1],[$2])
fi])

dnl AROS_TOOL_TARGET(var,prog)
dnl This is effectively the same as AROS_PROG, but adds the appropriate
dnl arch prefix when cross compiling. 
dnl
AC_DEFUN(AROS_TOOL_TARGET,
[
if test "$cross_compiling" = "yes" ; then
    AC_PATH_PROG([$1],${target_tool_prefix}[$2])
else
    AROS_TOOL_CCPATH($1, $2)
fi
])

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
