/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library configuration and user/group database
*/
#ifndef _SECURITY_CONFIG_H
#define _SECURITY_CONFIG_H

#include <exec/types.h>
#include <libraries/security.h>

struct SecurityBase;

/* Size of the general purpose line buffer */
#define secGENBUFSIZE                   1024

/*
 * Configuration
 */
struct secConfig
{
    ULONG               Flags;                  /* See definitions below                        */
    ULONG               LogFlags;               /* See definitions below                        */
    UWORD               PasswduidLevel;         /* Highest uid for users who can change passwd  */
    UWORD               PasswdgidLevel;         /* Highest gid for users who can change passwd  */
};

#define secCFGB_LimitDOSSetProtection   (0)     /* LimitDOSSetProtection                */
#define secCFGB_Profile                 (1)     /* Execute the .profile                 */
#define secCFGB_LastLoginReq            (2)     /* Display the lastlogin date           */
#define secCFGB_UseFSTab                (3)     /* Use the fstab file for extra FS      */
#define secCFGB_RT                      (4)     /* Enable resourcetracking (unused)     */

#define secCFGF_LimitDOSSetProtection   (1 << secCFGB_LimitDOSSetProtection)
#define secCFGF_Profile                 (1 << secCFGB_Profile)
#define secCFGF_LastLoginReq            (1 << secCFGB_LastLoginReq)
#define secCFGF_UseFSTab                (1 << secCFGB_UseFSTab)
#define secCFGF_RT                      (1 << secCFGB_RT)

#define secLogB_Startup                 (0)     /* Startup Information                  */
#define secLogB_Login                   (1)     /* Log successful Login/Logout          */
#define secLogB_LoginFail               (2)     /* Log unsuccessful Login/Logout        */
#define secLogB_Passwd                  (3)     /* Log successful Passwd                */
#define secLogB_PasswdFail              (4)     /* Log unsuccessful Passwd              */
#define secLogB_CheckPasswd             (5)     /* Log successful CheckPasswd           */
#define secLogB_CheckPasswdFail         (6)     /* Log unsuccessful CheckPasswd         */

#define secLogF_Startup                 (1 << secLogB_Startup)
#define secLogF_Login                   (1 << secLogB_Login)
#define secLogF_LoginFail               (1 << secLogB_LoginFail)
#define secLogF_Passwd                  (1 << secLogB_Passwd)
#define secLogF_PasswdFail              (1 << secLogB_PasswdFail)
#define secLogF_CheckPasswd             (1 << secLogB_CheckPasswd)
#define secLogF_CheckPasswdFail         (1 << secLogB_CheckPasswdFail)

/*
 * Private User Definition Entry
 */
struct secUserDef
{
    struct secUserDef   *Next;
    STRPTR              UserID;
    STRPTR              Password;
    UWORD               uid;
    UWORD               gid;
    STRPTR              UserName;
    STRPTR              HomeDir;
    STRPTR              Shell;
    UWORD               NumSecGroups;
    UWORD               *SecGroups;
};

/*
 * Private Group Definition Entry
 */
struct secGroupDef
{
    struct secGroupDef  *Next;
    STRPTR              GroupID;
    UWORD               gid;
    UWORD               MgrUid;
    STRPTR              GroupName;
};

/*
 * Function Prototypes (all run in the server's context unless noted)
 */
extern void LoadConfig(struct SecurityBase *secBase);
/* ReadKeyFile() results */
#define KEYFILE_NONE    (0)     /* the volume has no key file */
#define KEYFILE_OK      (1)     /* valid, key matches */
#define KEYFILE_BAD     (-1)    /* present but unreadable/mismatched/conflicting */

extern BOOL ReadKeyFiles(struct SecurityBase *secBase);
/* Does the volume behind 'fs' carry a valid key file? (server context) */
extern BOOL ProbeKeyFile(struct SecurityBase *secBase, struct MsgPort *fs);

extern struct secUserDef *GetUserDefs(struct SecurityBase *secBase);
extern struct secGroupDef *GetGroupDefs(struct SecurityBase *secBase);
extern void FreeDefs(struct SecurityBase *secBase);
extern BOOL UpdateUserDefs(struct SecurityBase *secBase);

extern BOOL ClearBuffer(struct SecurityBase *secBase);
extern void FreeBuffer(struct SecurityBase *secBase);
extern void PurgeKeyBuffer(struct SecurityBase *secBase);

/* Logging: VLogF takes a RawDoFmt style mem stream of SIPTRs, formats
 * written for 32-bit MuFS ("%ld") are converted on the fly */
extern void VLogF(struct SecurityBase *secBase, CONST_STRPTR fmt, SIPTR *argv);
extern void FixFormat(CONST_STRPTR src, STRPTR dst, ULONG dstsize);

#endif /* _SECURITY_CONFIG_H */
