/*
 * Extra data for m68k-all
 */
#include <exec/types.h>

struct Exec_PlatformData
{
	APTR  realRawDoFmt;     /* AOS 3.1 locale.library workaround - see
	                           exec.library/SetFunction() */
};
