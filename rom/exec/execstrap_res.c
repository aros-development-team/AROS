/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/nodes.h>

extern const char name[];
extern const char version[];
extern int start(void);
extern const char end;

int entry(void)
{
    return 0;
}

struct Resident resident =
{
    RTC_MATCHWORD,
    &resident,
    (APTR)&end,
    RTF_COLDSTART,
    1,			/* version */
    NT_KICKMEM,
    106,		/* Just above exec.library.
			   Because exec is RTF_SINGLETASK, and this is
			   RTF_COLDSTART, we'll still be started after
			   exec */
    (char *)name,
    (char *)&version[6],
    &start
};

const char name[] = "exec.strap";
const char version[] = "$VER: AROS exec.strap 1.2 (02.02.97)";
