	#include "aros/x86_64/asm.h"
	
/* sizeof(long) == sizeof(long long) */
#define	fn	llrintf
#include "s_lrintf.s"
