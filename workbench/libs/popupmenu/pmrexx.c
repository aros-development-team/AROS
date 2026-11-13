//
// popupmenu.library - ARexx Interface
//

#include "pmpriv.h"

#include <rexx/errors.h>
#include <rexx/rexxio.h>
#include <rexx/storage.h>
#include <proto/rexxsyslib.h>

extern char *version;

struct RexxReturnValues {
    ULONG d0;
    UBYTE *a0;
};

#define rxreturn(x, y)  { rxret=AllocVec(sizeof(struct RexxReturnValues), 0);\
                if(!rxret) { return 0L; }\
                rxret->d0=x; rxret->a0=y;\
                return rxret;\
            }

__asm ULONG a0hack(register __d0 ULONG return1, register __a0 UBYTE *return2)
{
    /* lame hack to return an argstring in A0 as arexx expects */
    return(return1);
}

__asm __saveds ULONG PM_RexxHost(register __a0 struct RexxMsg *rxmsg)
{
    UBYTE *argstr;
    STRPTR resstr;
    struct RexxReturnValues *rxret;

        rxreturn(ERR10_010, 0L); /*  invalid message packet      */

}
