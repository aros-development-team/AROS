/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_defs.h
 *
 * 
 */


#ifndef _ADF_DEFS_H
#define _ADF_DEFS_H 1

#define ADFLIB_VERSION "0.7.9b"
#define ADFLIB_DATE "10 september, 2000"

#define SECTNUM long
#define RETCODE long

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

#define ULONG   unsigned long
#define USHORT  unsigned short
#define UCHAR   unsigned char
#define BOOL    int


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



#endif /* _ADF_DEFS_H */
/*##########################################################################*/
