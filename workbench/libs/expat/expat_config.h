/* expat_config.h.  Generated from expat_config.h.in by configure.  */
/* expat_config.h.in.  Generated from configure.ac by autoheader.  */

#ifndef EXPAT_CONFIG_H
#define EXPAT_CONFIG_H 1

#include <aros/cpu.h>
#define ADEBUG 1
#include <aros/debug.h>

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* 1234 = LIL_ENDIAN, 4321 = BIGENDIAN */
#if AROS_BIG_ENDIAN
#   define BYTEORDER 4321
#else
#   define BYTEORDER 1234
#endif

/* Define to 1 if you have the `arc4random' function. */
#undef HAVE_ARC4RANDOM

/* Define to 1 if you have the `arc4random_buf' function. */
#undef HAVE_ARC4RANDOM_BUF

/* define if the compiler supports basic C++11 syntax */
#define HAVE_CXX11 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `getpagesize' function. */
#undef HAVE_GETPAGESIZE

/* Define to 1 if you have the `getrandom' function. */
#undef HAVE_GETRANDOM

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `bsd' library (-lbsd). */
#undef HAVE_LIBBSD

/* Define to 1 if you have a working `mmap' system call. */
#undef HAVE_MMAP

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have `syscall' and `SYS_getrandom'. */
#undef HAVE_SYSCALL_GETRANDOM

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#undef LT_OBJDIR

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#define PACKAGE_NAME "expat"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "expat 2.7.3"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "expat"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.7.3"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "2.7.3"

/* whether byteorder is bigendian */
#if AROS_BIG_ENDIAN
#   define WORDS_BIGENDIAN 1
#else
#   undef WORDS_BIGENDIAN
#endif

/* Define to allow retrieving the byte offsets for attribute names and values.
   */
/* #undef XML_ATTR_INFO */

/* Define to specify how much context to retain around the current parse
   point. */
#define XML_CONTEXT_BYTES 1024

/* Define to include code reading entropy from `/dev/urandom'. */
#undef XML_DEV_URANDOM

/* Define to make parameter entity parsing functionality available. */
#define XML_DTD 1

/* Define as 1/0 to enable/disable support for general entities. */
#define XML_GE 1

/* Define to make XML Namespaces functionality available. */
#define XML_NS 1

/* Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C. */
#undef __func__

/* Define to empty if `const' does not conform to ANSI C. */
#undef const

/* Define to `long int' if <sys/types.h> does not define. */
#undef off_t

/* Define to `unsigned int' if <sys/types.h> does not define. */
#undef size_t

#endif // ndef EXPAT_CONFIG_H
