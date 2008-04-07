#include <aros/cpu.h>
#include <exec/libraries.h>
#include <exec/types.h>

#define _STR(x) #x
#define STR(x) _STR(x)

asm ("\n#define STUBCODE_INIT " STR(STUBCODE_INIT));
asm ("\n#define STUBCODE " STR(STUBCODE));
asm ("\n#define ALIASCODE " STR(ALIASCODE));

void foo()
{
    asm volatile("\n#define JUMPVEC(n) ((n)+1+%0)*%1"::"i"(LIB_RESERVED),"i"(-LIB_VECTSIZE));
}
