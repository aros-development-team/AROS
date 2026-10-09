/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: filesystem.service - who may mount what, and what a mounted user
          may do (re/spec/efs-protocol.md §5.2, §5.3). Credentials are
          checked through accounts.library, as the original does: a "$"
          password is an ECrypt hash (VerifyUserCrypt), anything else is
          clear text (VerifyUser). accounts.library hands the request to
          the Accounts Server, which uses the system's account store and
          pam.library (envoy-aros-architecture.md §5).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "fs_intern.h"
#include <proto/accounts.h>

#define AccountsBase (srv->AccLib)
#define UtilityBase  (srv->UtilLib)

BOOL AuthVerify(struct FSServer *srv, CONST_STRPTR user, CONST_STRPTR password, struct UserInfo *ui)
{
    ULONG r;

    memset(ui, 0, sizeof(*ui));
    if (!AccountsBase || !user || !user[0])
        return FALSE;
    if (password && password[0] == '$')
        r = VerifyUserCrypt((STRPTR)user, (STRPTR)password + 1, ui);
    else
        r = VerifyUser((STRPTR)user, (STRPTR)(password ? password : (CONST_STRPTR)""), ui);
    FSLOG(srv, "verify user '%s' (%s) -> %lu\n", user, (password && password[0] == '$') ? "hashed" : "clear", (unsigned long)r);
    return r == 0;
}

/* the user is on the access list, personally or through a group */
BOOL AuthMayMount(struct FSServer *srv, struct Export *e, struct UserInfo *ui, BOOL authenticated)
{
    ULONG i;

    if (e->Flags & EXPF_NOSEC)
        return TRUE;
    if (!authenticated)
        return FALSE;
    for (i = 0; i < e->NumAccess; i++)
    {
        if (e->AccKind[i] == 0)
        {
            if (e->AccId[i] == ui->ui_UserID)
                return TRUE;
        }
        else
        {
            struct GroupInfo *gi;
            if (e->AccId[i] == ui->ui_PrimaryGroupID)
                return TRUE;
            if (AccountsBase && (gi = AllocGroupInfo()))
            {
                BOOL member = IDToGroup(e->AccId[i], gi) == 0 && MemberOf(gi, ui) == 0;
                FreeGroupInfo(gi);
                if (member)
                    return TRUE;
            }
        }
    }
    return FALSE;
}

/*
 * §5.2: verify, select the export by its match name, check the access
 * list. Returns the export or NULL; *credfail says whether the password
 * was rejected; *authenticated whether ui is valid.
 */
struct Export *AuthSelectExport(struct FSServer *srv, CONST_STRPTR name, CONST_STRPTR user, CONST_STRPTR password,
                                struct UserInfo *ui, BOOL *authenticated, BOOL *credfail)
{
    struct Export *e;

    *authenticated = AuthVerify(srv, user, password, ui);
    *credfail = !*authenticated;
    if (!*authenticated)
    {
        ui->ui_UserID = EFS_NOUSER;
        ui->ui_PrimaryGroupID = EFS_NOUSER;
        ui->ui_Flags = 0;
    }
    if (!(e = ConfigFindExport(srv, name)) || !e->RootLock)
        return NULL;
    if (!AuthMayMount(srv, e, ui, *authenticated))
        return NULL;
    return e;
}

/* membership test for the rights calculation */
static BOOL InGroup(struct FSServer *srv, struct Mount *m, UWORD gid)
{
    struct GroupInfo *gi;
    struct UserInfo ui;
    BOOL member = FALSE;

    if (gid == m->Gid)
        return TRUE;
    if (!m->Authenticated || !AccountsBase)
        return FALSE;
    memset(&ui, 0, sizeof(ui));
    EfsPutCStr((UBYTE *)ui.ui_UserName, sizeof(ui.ui_UserName), m->User);
    ui.ui_UserID = m->Uid;
    ui.ui_PrimaryGroupID = m->Gid;
    ui.ui_Flags = m->AccFlags;
    if ((gi = AllocGroupInfo()))
    {
        member = IDToGroup(gid, gi) == 0 && MemberOf(gi, &ui) == 0;
        FreeGroupInfo(gi);
    }
    return member;
}

/*
 * Effective rights of the mount's user on an object (§5.3): owner class
 * for administrators and owners (classic inverted bits), else the group
 * class, else the other class.
 */
ULONG AuthRights(struct FSServer *srv, struct Mount *m, const struct FileInfoBlock *fib, BOOL *isowner)
{
    ULONG prot = fib->fib_Protection, r = 0;

    if (isowner)
        *isowner = FALSE;
    if ((m->AccFlags & UFLAGF_AdminAll) || (m->Authenticated && fib->fib_OwnerUID == m->Uid))
    {
        if (isowner)
            *isowner = TRUE;
        if (!(prot & FIBF_READ))    r |= RIGHT_R;
        if (!(prot & FIBF_WRITE))   r |= RIGHT_W;
        if (!(prot & FIBF_EXECUTE)) r |= RIGHT_E;
        if (!(prot & FIBF_DELETE))  r |= RIGHT_D;
        return r;
    }
    if (m->Authenticated && InGroup(srv, m, fib->fib_OwnerGID))
    {
        if (prot & FIBF_GRP_READ)    r |= RIGHT_R;
        if (prot & FIBF_GRP_WRITE)   r |= RIGHT_W;
        if (prot & FIBF_GRP_EXECUTE) r |= RIGHT_E;
        if (prot & FIBF_GRP_DELETE)  r |= RIGHT_D;
        return r;
    }
    if (prot & FIBF_OTR_READ)    r |= RIGHT_R;
    if (prot & FIBF_OTR_WRITE)   r |= RIGHT_W;
    if (prot & FIBF_OTR_EXECUTE) r |= RIGHT_E;
    if (prot & FIBF_OTR_DELETE)  r |= RIGHT_D;
    return r;
}

/*
 * What a Full File Security client is shown (§5.3): for a caller who is
 * not the owner the low four bits become the caller's effective rights in
 * inverted form; when the other class applied, bits 8-11 as well.
 */
LONG AuthRewriteProtection(struct FSServer *srv, struct Mount *m, const struct FileInfoBlock *fib)
{
    ULONG prot = fib->fib_Protection, r, low = 0;
    BOOL owner;

    r = AuthRights(srv, m, fib, &owner);
    if (owner)
        return prot;
    if (!(r & RIGHT_R)) low |= FIBF_READ;
    if (!(r & RIGHT_W)) low |= FIBF_WRITE;
    if (!(r & RIGHT_E)) low |= FIBF_EXECUTE;
    if (!(r & RIGHT_D)) low |= FIBF_DELETE;
    prot = (prot & ~0x0F) | low;
    if (!(m->Authenticated && InGroup(srv, m, fib->fib_OwnerGID)))
    {
        ULONG grp = 0;
        if (r & RIGHT_D) grp |= FIBF_GRP_DELETE;
        if (r & RIGHT_E) grp |= FIBF_GRP_EXECUTE;
        if (r & RIGHT_W) grp |= FIBF_GRP_WRITE;
        if (r & RIGHT_R) grp |= FIBF_GRP_READ;
        prot = (prot & ~0x0F00) | grp;
    }
    return prot;
}
