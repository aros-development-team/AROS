/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library - ownership and protection of the database
*/
#ifndef _SECURITY_PROTECT_H
#define _SECURITY_PROTECT_H

#include <exec/types.h>

struct SecurityBase;

/* Names used while the password and group files are rewritten */
#define secPasswd_NewName       "passwd.new"
#define secPasswd_OldName       "passwd.old"
#define secGroup_NewName        "group.new"
#define secGroup_OldName        "group.old"

/* Server context, current directory = the file's directory: finish an
 * interrupted rewrite of a database file */
extern void RecoverDbFile(struct SecurityBase *secBase, CONST_STRPTR name, CONST_STRPTR newname, CONST_STRPTR oldname);
#define RecoverPasswdFile(secBase)  RecoverDbFile(secBase, PasswdFileName, secPasswd_NewName, secPasswd_OldName)
#define RecoverGroupFile(secBase)   RecoverDbFile(secBase, GroupFileName, secGroup_NewName, secGroup_OldName)
extern const char PasswdFileName[];
extern const char GroupFileName[];

/* Server context: make the database, the configuration and the PAM
 * service files and modules root's and not writable (the password file:
 * not readable) by others; log what had to be corrected */
extern void CheckDatabaseProtection(struct SecurityBase *secBase);

/* Once per boot: hint when S:Security-Startup is older than AMUInit's */
extern void CheckStartupScript(struct SecurityBase *secBase);

/* Owner and protection for a file only root may read and write; with
 * readable, others may read it */
extern void ProtectRootOnly(struct SecurityBase *secBase, CONST_STRPTR name);
extern void ProtectRootWritable(struct SecurityBase *secBase, CONST_STRPTR name);

#endif /* _SECURITY_PROTECT_H */
