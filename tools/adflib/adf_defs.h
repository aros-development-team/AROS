/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_defs.h
 *
 * 
 */


#ifndef _ADF_DEFS_H
#define _ADF_DEFS_H 1

#include <sys/types.h>
#include <stdint.h>

#define ADFLIB_VERSION "0.8.0"
#define ADFLIB_DATE "10 September, 2012"

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

#define BOOL    int
#define UQUAD   uint64_t
#define ULONG   uint32_t
#define USHORT  uint16_t
#define UCHAR   uint8_t
#define UBYTE   uint8_t
#define TEXT    char

#define SECTNUM ULONG
#define RETCODE long

/* defines max and min */

#ifndef max
#define max(a,b)        (a)>(b) ? (a) : (b)
#endif
#ifndef min
#define min(a,b)        (a)<(b) ? (a) : (b)
#endif


/* (*byte) to (*short) and (*byte) to (*long) conversion */

#define Short(p) ((p)[0]<<8 | (p)[1])
#define Long(p) (Short(p)<<16 | Short(p+2))


/* swap short and swap long macros for little endian machines */

#define swapShort(p) ((p)[0]<<8 | (p)[1])
#define swapLong(p) (swapShort(p)<<16 | swapShort(p+2))

#ifdef WIN32DLL
#define PREFIX __declspec(dllexport)
#else
#define PREFIX 
#endif /* WIN32DLL */

#endif /* _ADF_DEFS_H */
/*##########################################################################*/
