#include <aros/cpu.h>

/* Define if building universal (internal helper macro) */
#undef AC_APPLE_UNIVERSAL_BUILD

/* Whether we have alarm() */
#undef HAVE_ALARM

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Whether we have feenableexcept() */
#undef HAVE_FEENABLEEXCEPT

/* Define to 1 if we have <fenv.h> */
#define HAVE_FENV_H 1

/* Define to 1 if you have the `getisax' function. */
#undef HAVE_GETISAX

/* Whether we have getpagesize() */
#undef HAVE_GETPAGESIZE

/* Whether we have gettimeofday() */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `pixman-1' library (-lpixman-1). */
#undef HAVE_LIBPIXMAN_1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Whether we have mprotect() */
#undef HAVE_MPROTECT

/* Whether we have posix_memalign() */
#define HAVE_POSIX_MEMALIGN 1

/* Whether pthread_setspecific() is supported */
#undef HAVE_PTHREAD_SETSPECIFIC

/* Whether we have sigaction() */
#undef HAVE_SIGACTION

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if we have <sys/mman.h> */
#undef HAVE_SYS_MMAN_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#undef LT_OBJDIR

/* Name of package */
#define PACKAGE

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the home page for this package. */
#undef PACKAGE_URL

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* enable TIMER_BEGIN/TIMER_END macros */
#undef PIXMAN_TIMERS

/* The size of `long', as computed by sizeof. */
#undef SIZEOF_LONG

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Whether the tool chain supports __attribute__((constructor)) */
#undef TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR

/* Whether the tool chain supports __thread */
#undef TOOLCHAIN_SUPPORTS__THREAD

/* use ARM NEON assembly optimizations */
#undef USE_ARM_NEON

/* use ARM SIMD assembly optimizations */
#undef USE_ARM_SIMD

/* use GNU-style inline assembler */
#undef USE_GCC_INLINE_ASM

/* use MMX compiler intrinsics */
#undef USE_MMX

/* use OpenMP in the test suite */
#undef USE_OPENMP

/* use SSE2 compiler intrinsics */
#undef USE_SSE2

/* use VMX compiler intrinsics */
#undef USE_VMX

/* Version number of package */
#undef VERSION

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if AROS_BIG_ENDIAN
#  define WORDS_BIGENDIAN 1
#else
#  undef WORDS_BIGENDIAN
#endif

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#undef inline
#endif
