#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/codesets.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "globals.h"
#include "charset.h"
#include "aros_stuff.h"

extern struct Globals *global;

#define SysBase      global->SysBase
#define CodesetsBase global->CodesetsBase

void InitCharset(void)
{
    if (!CodesetsBase)
    {
	CodesetsBase = OpenLibrary("codesets.library", 0);
	BUG(dbprintf("[CDVDFS] CodesetsBase 0x%p\n", CodesetsBase));

	if (CodesetsBase)
	{
	    global->uniCodeset = CodesetsFindA("UTF-16", NULL);
	    BUG(dbprintf("[CDVDFS] Unicode codeset: 0x%p\n", global->uniCodeset));
	}
    }
}

int UTF16ToSystem(char *from, char *to, unsigned char len)
{
    ULONG l = -1;

    if (global->uniCodeset)
    {
	char *str;
	struct TagItem tags[5];

	/*
	 * I have to fill in tags in this weird way because otherwise
	 * MorphOS gcc v2.95.3 generates memcpy() call here, and we have
	 * no global SysBase variable.
	 * And i can't use -lcodesets because it also requires global
	 * CodesetsBase.
	 * Please don't change this.
	 */
	tags[0].ti_Tag  = CSA_SourceCodeset;
	tags[0].ti_Data = (IPTR)global->uniCodeset;
	tags[1].ti_Tag  = CSA_Source;
	tags[1].ti_Data = (IPTR)from;
	tags[2].ti_Tag  = CSA_SourceLen;
	tags[2].ti_Data = len;
	tags[3].ti_Tag  = CSA_DestLenPtr;
	tags[3].ti_Data = (IPTR)&l;
	tags[4].ti_Tag  = TAG_DONE;

	/*
	 * Unfortunately codesets.library does not allow
	 * to specify CSA_Dest and CSA_DestLen to
	 * CodesetsConvertA(), so we have to do this copy-deallocate
	 * operation.
	 */
	str = CodesetsConvertStrA(tags);
	if (str)
	{
	    CopyMem(str, to, l);
	    CodesetsFreeA(str, NULL);
	}
    }

    return l;
}

