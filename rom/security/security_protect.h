/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library - ownership and protection of the database
*/
#ifndef _SECURITY_PROTECT_H
#define _SECURITY_PROTECT_H

#include <exec/types.h>

struct SecurityBase;

/* Names used while the password file is rewritten */
#define secPasswd_NewName       "passwd.new"
#define secPasswd_OldName       "passwd.old"

/* Server context, current directory = the password directory: finish an
 * interrupted rewrite of the password file */
extern void RecoverPasswdFile(struct SecurityBase *secBase);

/* Server context: make the database, the configuration and the PAM
 * service files and modules root's and not writable (the password file:
 * not readable) by others; log what had to be corrected */
extern void CheckDatabaseProtection(struct SecurityBase *secBase);

/* Once per boot: hint when S:Security-Startup is older than AMUInit's */
extern void CheckStartupScript(struct SecurityBase *secBase);

/* Owner and protection for a file only root may read and write */
extern void ProtectRootOnly(struct SecurityBase *secBase, CONST_STRPTR name);

#endif /* _SECURITY_PROTECT_H */
