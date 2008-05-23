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
    asm volatile("\n#define LIB_VECTSIZE @SED@%0"::"i"(LIB_VECTSIZE));
}
