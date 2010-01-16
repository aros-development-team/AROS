#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(parthenope, svn91, cjg)
AC_CONFIG_SRCDIR([src/parthenope.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

# Checks for library functions.

# Conditional compilation
AC_ARG_ENABLE(platform,
	      AC_HELP_STRING([--enable-platform=platform], 
			     [Select for which should be built: uboot (default) or openfirmware]),
		[case "${enableval}" in
		 uboot)
			platform=uboot
			;;
		openfirmware)
			platform=openfirmware
			;;
		*)
			AC_MSG_ERROR([bad value ${enableval} for --enable-platform option])
			;;
		esac],
		[platform=uboot]
)
AM_CONDITIONAL(USE_UBOOT, test x"$platform" = xuboot)
AM_CONDITIONAL(USE_OPENFIRMWARE, test x"$platform" = xopenfirmware)
AC_CONFIG_FILES([Makefile
                 src/Makefile
		 tools/Makefile])
AC_OUTPUT
