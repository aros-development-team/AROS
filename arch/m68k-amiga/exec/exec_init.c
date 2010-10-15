#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/arossupportbase.h>
#include <exec/execbase.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>

#include <proto/exec.h>

#include <stdarg.h>
#include <strings.h>
#include <inttypes.h>

int exec_main(struct TagItem *msg, void *entry);

const char exec_name[] = "exec.library";
const char exec_idstring[] = "$VER: exec 41.11 (16.12.2000)\r\n";
const char exec_chipname[] = "Chip Memory";
const char exec_fastname[] = "Fast Memory";

const short exec_Version = 41;
const short exec_Revision = 11;

const struct __attribute__((section(".text"))) Resident Exec_resident =
{
        RTC_MATCHWORD,          /* Magic value used to find resident */
        &Exec_resident,         /* Points to Resident itself */
        &Exec_resident+1,       /* Where could we find next Resident? */
        0,                      /* There are no flags!! */
        41,                     /* Version */
        NT_LIBRARY,             /* Type */
        126,                    /* Very high startup priority. */
        (STRPTR)exec_name,      /* Pointer to name string */
        (STRPTR)exec_idstring,  /* Ditto */
        exec_main               /* Library initializer (for exec this value is irrelevant since we've jumped there at the begining to bring the system up */
};

int exec_main(struct TagItem *msg, void *entry) {
    return 0;
}

