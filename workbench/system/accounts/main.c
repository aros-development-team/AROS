/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Accounts - manage the user accounts and groups of the system

          Works on both kinds of system: with security.library (multi-user;
          only root may change other accounts, anybody may change their own
          password) and without it (single-user; the network password
          database of usergroup.library / netinfo.device). See accounts.c.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>

#include "accounts.h"
#include "gui.h"
#include "locale.h"

const TEXT version[] = "$VER: Accounts 1.1 (15.10.2026)";

static void Fail(CONST_STRPTR text)
{
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, (STRPTR)"Accounts", (STRPTR)"%s", (STRPTR)"OK" };

    if (IntuitionBase)
        EasyRequest(NULL, &es, NULL, (IPTR)text);
    else
        PutStr(text);
}

int main(void)
{
    struct AccountsState st;
    struct Library *ug;
    int rc = RETURN_FAIL;

    Locale_Initialize();

    /* ug_UserAdminA() arrived with usergroup.library 4.11, its groups with 4.12 */
    if (!(ug = OpenLibrary("usergroup.library", 4)) || (ug->lib_Version == 4 && ug->lib_Revision < 12))
        Fail(MSG(MSG_ERR_NOUSERGROUP));
    else if (Acc_Init(&st))
    {
        if (Gui_Run(&st))
            rc = RETURN_OK;
        else
            Fail(MSG(MSG_ERR_NOMUI));
        Acc_Cleanup(&st);
    }
    if (ug)
        CloseLibrary(ug);

    Locale_Deinitialize();
    return rc;
}
