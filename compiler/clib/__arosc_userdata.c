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

    static void __arosc_userdata_exitcode(LONG returncode, struct arosc_privdata *acpd);

    struct Task *me = FindTask(NULL);
    struct arosc_privdata *acpd = GetIntETask(me)->iet_acpd;

    /* If acpd == NULL it means that this function has been invoked
       by a library which has been opened by a process which hasn't
       been linked against libarosc.a, and thus hasn't invoked
       arosc_internalinit(), which means that we have to allocate
       the needed memory on the fly.

       Unfortunately this also means that we can't gracefully
       report any error to the calling program, thus we can only bail out.

       In addition to the above, we have also to find a way to call
       arosc_internalexit() ONLY when the process terminates. We do this
       by means of pr_ExitCode and pr_ExitData fields of the process structure,
       which let us execute code when the process terminates.  */

    if (!acpd)
    {

        /* This may seem strange, why opening _ourselves_ again?
	   Well, this is just a way to increase the library's open count,
	   so that the library doesn't get expunged before the process exits:
	   if that were to happen, when the process were about to exit, it
	   would jump to code which wouldn't be there anymore.  */

	/* Openlibrary() cannot fail here, because arosc's Open() vector doesn't
	   allocate memory nor does anything else which could fail, and of course
	   the library is already in memory.  */
        OpenLibrary("arosc.library", 0);

	/* If this fails, there's no much we can do but bail out...  */
        if (arosc_internalinit(&acpd))
	    abort();

	/* Setup the exit code properly.  */
	acpd->acpd_oldexitcode = ((struct Process *)me)->pr_ExitCode;
	acpd->acpd_oldexitdata = ((struct Process *)me)->pr_ExitData;
	((struct Process *)me)->pr_ExitCode = __arosc_userdata_exitcode;
	((struct Process *)me)->pr_ExitData = (IPTR)acpd;
    }

    return &acpd->acpd_acud;

    #endif
}

#ifndef DO_STATIC
static void __arosc_userdata_exitcode(LONG returncode, struct arosc_privdata *acpd)
{
    void (*oldexitcode)() = acpd->acpd_oldexitcode;
    IPTR oldexitdata      = acpd->acpd_oldexitdata;

    /* Unfortunately I can't find any way around this: I need Forbid() here
       becase I need to clear the LIBF_DELEXP flag from the libbase, so that
       the library doesn't get expunged while it is still executing its code!
       It would happen if the library were the last opener of itself and the LIBF_DELEXP
       flag were set. It could also happen if someone attempted to expunge the library
       right after CloseLibrary returned, and if that someone were the last opener of
       the library, which is another reason to use Forbid().  */

    Forbid();

    arosc_internalexit();

    aroscbase->lib_Flags &= ~LIBF_DELEXP;
    CloseLibrary(aroscbase);

    if (oldexitcode)
        oldexitcode(returncode, oldexitdata);

    /* No Permit() here, because exec will reenable multitasking as soon
       as the process is terminated, which is what we really want.  */
}
#endif
