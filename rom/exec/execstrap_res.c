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
    1,
    NT_KICKMEM,
    106,	/* just above exec.library */
    (char *)name,
    (char *)&version[6],
    &start
};

const char name[] = "exec.strap";
const char version[] = "$VER: exec.strap 1.0 (30.12.96)";
