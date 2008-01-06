#include <aros/cpu.h>
#include <exec/libraries.h>

const char stubcode_init[] = STUBCODE_INIT;
const char stubcode[] = STUBCODE;
const char aliascode[] = ALIASCODE;

static inline void *jumpvec(int n)
{
    return &(__AROS_GETJUMPVEC(NULL, (n+1+LIB_RESERVED))->vec);
}
