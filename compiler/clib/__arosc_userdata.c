#include "__arosc_privdata.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <sys/arosc.h>

#include <stdlib.h>

#include "arosc_init.h"

#include "__ctype.h"
#include "etask.h"

#include <aros/debug.h>

extern struct Library *aroscbase;

struct arosc_userdata * __get_arosc_userdata(void)
{
    struct arosc_privdata *acpd = NULL;
    
    #ifdef AROSC_SHARED
    struct Task *curtask = FindTask(NULL);
    
    acpd = GetIntETask(curtask)->iet_acpd;
    
    if (acpd == NULL)
    {
	#warning FIXME: This is a quick hack to make old programs which use
	#warning        the libc from inside newly created tasks/processes
	#warning        without going trough special libc routines
	#warning        (yet to be written).

        struct Task *parent = GetETask(curtask)->et_Parent;
	
	if (parent)
            acpd = GetIntETask(parent)->iet_acpd;
    }
    #endif 

    if (acpd == NULL)
    {
        /* No acpd found? Fall back to the builtin one.  */
        static struct arosc_privdata acpd_static =
	{
            .acpd_acud.acud_ctype_b       = &__ctype_b_array[128],
            .acpd_acud.acud_ctype_toupper = &__ctype_toupper_array[128],
            .acpd_acud.acud_ctype_tolower = &__ctype_tolower_array[128],
	};

        return &acpd_static.acpd_acud;
    }
   
    return &acpd->acpd_acud;
}
