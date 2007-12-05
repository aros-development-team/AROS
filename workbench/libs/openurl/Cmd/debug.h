/* Debuggin defines used by SmartReadArgs */

#ifdef __AROS__

#ifdef DEBUG_CODE
#define DEBUG 1
#endif /* DEBUG_CODE */

#include <aros/debug.h>

#else /* __AROS__ */

#ifdef __GNUC__
/* Debug output goes to stdout for GCC as debug.lib doesn't seem to be
 * included in Geek Gadgets' distribution */
#include <clib/alib_stdio_protos.h>
#define bug printf

#else
/* Debug output goes to serial port/sushi debug console */
#define bug kprintf
extern void kprintf(char *fmt,...);
#endif

#ifdef DEBUG_CODE
#define D(x)     x
#else
#define D(x)
#endif

#endif /* __AROS__ */
