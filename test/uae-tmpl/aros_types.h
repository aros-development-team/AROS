
#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H

typedef char BYTE;
typedef unsigned char UBYTE;

#include "be_ptr"

typedef be_val<long>  LONG;
typedef be_val<short> WORD;
typedef be_ptr<void>  APTR;
typedef be_ptr<char>  STRPTR;

#endif /* EXEC_TYPES_H */
