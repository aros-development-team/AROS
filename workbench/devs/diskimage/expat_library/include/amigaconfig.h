#ifndef AMIGACONFIG_H
#define AMIGACONFIG_H

#include "endian.h"

/* 1234 = LIL_ENDIAN, 4321 = BIGENDIAN */
#if WORDS_BIGENDIAN
#define BYTEORDER 4321
#else
#define BYTEORDER 1234
#endif

/* Define to 1 if you have the `bcopy' function. */
#define HAVE_BCOPY 1

/* Define to 1 if you have the <check.h> header file. */
#undef HAVE_CHECK_H

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to specify how much context to retain around the current parse
   point. */
#define XML_CONTEXT_BYTES 1024

/* Define to make parameter entity parsing functionality available. */
#define XML_DTD

/* Define to make XML Namespaces functionality available. */
#define XML_NS

#endif  /* AMIGACONFIG_H */
