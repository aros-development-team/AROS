/************************************************************
* MultiUser - MultiUser Task/File Support System				*
* ---------------------------------------------------------	*
* Configuration															*
* ---------------------------------------------------------	*
* © Copyright 1993-1994 Geert Uytterhoeven						*
* All Rights Reserved.													*
************************************************************/


#include <libraries/security.h>

/*
 *		MultiUser Configuration
 */

struct secConfig {
    ULONG                               Flags;								/* See definitions below */
    ULONG                               LogFlags;							/* See definitions below */
    UWORD                               PasswduidLevel;					/* Lowest uid for users who can */
                                                                                                    /* change their passwords */
    UWORD                               PasswdgidLevel;					/* Lowest gid for users who can */
													/* change their passwords */
};

#define muCFGB_LimitDOSSetProtection	(0)	/* LimitDOSSetProtection */
#define muCFGB_Profile						(1)	/* Execute the .profile */
#define muCFGB_LastLoginReq				(2)	/* Display the lastlogin date */
#define muCFGB_UseFSTab						(3)	/* Use the fstab file for extra FS */
#define muCFGB_RT								(4)	/* Enable resourcetracking */

#define muLogB_Startup						(0)	/* Startup Information */
#define muLogB_Login							(1)	/* Log successful Login/Logout */
#define muLogB_LoginFail					(2)	/* Log unsuccessful Login/Logout */
#define muLogB_Passwd						(3)	/* Log successful Passwd */
#define muLogB_PasswdFail					(4)	/* Log unsuccessful Passwd */
#define muLogB_CheckPasswd					(5)	/* Log successful CheckPasswd */
#define muLogB_CheckPasswdFail			(6)	/* Log unsuccessful CheckPasswd */

#define muCFGF_LimitDOSSetProtection	(1<<muCFGB_LimitDOSSetProtection)
#define muCFGF_Profile						(1<<muCFGB_Profile)
#define muCFGF_LastLoginReq				(1<<muCFGB_LastLoginReq)
#define muCFGF_UseFSTab						(1<<muCFGB_UseFSTab)
#define muCFGF_RT								(1<<muCFGB_RT)

#define muLogF_Startup						(1<<muLogB_Startup)
#define muLogF_Login							(1<<muLogB_Login)
#define muLogF_LoginFail					(1<<muLogB_LoginFail)
#define muLogF_Passwd						(1<<muLogB_Passwd)
#define muLogF_PasswdFail					(1<<muLogB_PasswdFail)
#define muLogF_CheckPasswd					(1<<muLogB_CheckPasswd)
#define muLogF_CheckPasswdFail			(1<<muLogB_CheckPasswdFail)

/*
 *		Private User Definition Entry
 */

struct secUserDef {
    struct secUserDef                   *Next;
    STRPTR                              UserID;
    STRPTR                              Password;
    UWORD                               uid;
    UWORD                               gid;
    STRPTR                              UserName;
    STRPTR                              HomeDir;
    STRPTR                              Shell;
    UWORD                               NumSecGroups;
    UWORD                               *SecGroups;
};

/*
 *		Private Group Definition Entry
 */

struct secGroupDef {
    struct secGroupDef                  *Next;
    STRPTR                              GroupID;
    UWORD                               gid;
    UWORD                               MgrUid;
    STRPTR                              GroupName;
};

/*
 *		Function Prototypes
 */

extern void LoadConfig(struct Library *_Base);
extern BOOL ReadKeyFiles(struct Library *_Base);

extern struct secUserDef *GetUserDefs(struct Library *secBase);
extern struct secGroupDef *GetGroupDefs(struct Library *secBase);
extern void FreeDefs(void);
extern BOOL UpdateUserDefs(struct Library *secBase);

void ClearBuffer(void);
void FreeBuffer(void);
void PurgeKeyBuffer(void);
extern void VLogF(struct Library *secBase, STRPTR fmt, SIPTR *argv);
