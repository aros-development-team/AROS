#ifndef ADF_DEFENDIAN_H
#define ADF_DEFENDIAN_H

#include <sys/param.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
/* Little Endian */
#define LITT_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
/* Big Endian */
#else
/* Unknown Endian */
#error Unknown Endian type!
#endif

#endif /* ADF_DEFENDIAN_H */
