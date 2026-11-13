	#include "aros/x86_64/asm.h"
	
/* sizeof(long) == sizeof(long long) */
#define	fn	llrint
#include "s_lrint.s"
