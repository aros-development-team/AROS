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
	/* FIXME: This is a quick hack to make old programs which use
	 *        the libc from inside newly created tasks/processes
	 *        without going trough special libc routines
	 *       (yet to be written).
	 */
        struct Task *parent = GetETask(curtask)->et_Parent;
	    while (parent != NULL)
	    {
            acpd = GetIntETask(parent)->iet_acpd;
            if(acpd != NULL) break;
            parent = GetETask(parent)->et_Parent;
	    }
    }
    #endif 

    if (acpd == NULL)
    {
#ifdef AROSC_ROM
        /* JUST the user data - we really only want the ctype arrays */
        static const struct arosc_userdata acud = {
            .acud_ctype_b       = &__ctype_b_array[128],
            .acud_ctype_toupper = &__ctype_toupper_array[128],
            .acud_ctype_tolower = &__ctype_tolower_array[128],
        };

        /* Remove the 'const' when passing back */
        return (struct arosc_userdata *)&acud;
#else
        /* No acpd found? Fall back to the builtin one.  */
        static struct arosc_privdata acpd_static;

        acpd_static.acpd_acud.acud_ctype_b       = &__ctype_b_array[128];
        acpd_static.acpd_acud.acud_ctype_toupper = &__ctype_toupper_array[128];
        acpd_static.acpd_acud.acud_ctype_tolower = &__ctype_tolower_array[128];

        acpd = &acpd_static;
#endif
    }
   
    return &acpd->acpd_acud;
}
