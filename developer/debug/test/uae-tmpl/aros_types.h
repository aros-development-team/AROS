
#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H

typedef char BYTE;
typedef unsigned char UBYTE;

#include "be_ptr"

typedef be_val<long>  LONG;
typedef be_val<unsigned long>  ULONG;
typedef be_val<short> WORD;
typedef be_val<unsigned short> UWORD;
typedef be_ptr<void>  APTR;
typedef be_ptr<UBYTE>  STRPTR;

#endif /* EXEC_TYPES_H */
