#ifndef ___AROSC_PRIVDATA_H
#    define DO_STATIC
#    include "__arosc_privdata.h"
#endif

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
    #ifdef DO_STATIC

    static struct arosc_privdata acpd_static =
    {
        .acpd_acud.acud_ctype_b       = &__ctype_b_array[128],
        .acpd_acud.acud_ctype_toupper = &__ctype_toupper_array[128],
        .acpd_acud.acud_ctype_tolower = &__ctype_tolower_array[128],
    };

    return &acpd_static.acpd_acud;

    #else

    struct arosc_privdata *acpd = GetIntETask(FindTask(NULL))->iet_acpd;

    return &acpd->acpd_acud;

    #endif
}
