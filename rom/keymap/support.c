#include <string.h>
#include "keymap_intern.h"

BOOL WriteToBuffer(struct BufInfo *bufinfo, UBYTE *string, LONG numchars)
{
    if (bufinfo->CharsWritten + numchars > bufinfo->BufLength)
    	return (FALSE);
    	
    strncpy(bufinfo->Buffer, string, numchars);
    bufinfo->CharsWritten += numchars;
    
    return (TRUE);
}
