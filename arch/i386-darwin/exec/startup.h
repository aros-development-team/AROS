#ifndef _STARTUP_H
#define _STARTUP_H

#include <exec/types.h>
#include "macros.h"

/* external data definitions */

extern const UBYTE core_id[];
extern const UBYTE _END;

extern const UBYTE Exec_Start;
extern const UBYTE Exec_End;

extern const ULONG ExecFunctions[];

/* external function definitions */

void startup(void);
ULONG ** RomTagScanner(struct ExecBase *, UWORD **);
ULONG Exec_MakeFunctions(APTR, APTR, APTR, APTR);

/* inlined functions taken from c library, needed by bootup */

static inline __attribute__((always_inline)) 
char *strncpy(char *dest, const char *src, ULONG n)
{
    char *ret = dest;
    while(n--)
    {
	*dest++ = *src++;
	if (*src == 0)
	    break;
    }
    return ret;
}

static inline __attribute__((always_inline))
APTR memcpy(APTR dest, APTR src, ULONG n)
{
    while(n--)
    {
	((UBYTE*)dest)[n] = ((UBYTE*)src)[n];
    }
}

static inline __attribute__((always_inline))
void bzero(APTR s, ULONG n)
{
    while(n--)
	((UBYTE*)s)[n] = 0;
}

#define offsetof(struct_, field) ((int)(&((struct struct_*)0UL)->field))

#endif /* _STARTUP_H */

