#ifdef __AROS__
#include <aros/debug.h>
#else
#if DEBUG > 0
#include <clib/debug_protos.h>
#define D(x) x
#define bug kprintf

static inline void RawPutChars(const UBYTE *string, int len)
{
	while (len--)
		kputc(*string++);
}

#else
#define D(x)
#endif
#if DEBUG > 1
#define DB2(x) x
#else
#define DB2(x)
#endif
#endif

