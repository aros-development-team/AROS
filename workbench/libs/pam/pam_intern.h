/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library internals
*/
#ifndef PAM_INTERN_H
#define PAM_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <utility/hooks.h>
#include <libraries/pam.h>
#include <libraries/pam_module.h>

#include <aros/debug.h>
#include LC_LIBDEFS_FILE

#define DEBUG_NAME_STR          "[pam.library]"

struct PamBase
{
    struct Library              LibNode;
    BPTR                        SegList;
    struct Library              *pam_DOSBase;
    struct Library              *pam_UtilityBase;
    struct SignalSemaphore      ModuleSem;
    struct MinList              Modules;            /* loaded external modules (struct PamModule) */
    STRPTR                      ModuleDir;          /* volume-based path of PAM_MODULE_DIR, or NULL */
};

#define DOSBase                 ((struct DosLibrary *)PamBase->pam_DOSBase)
#define UtilityBase             (PamBase->pam_UtilityBase)

/* Management groups */
#define PAM_GROUP_AUTH          0
#define PAM_GROUP_ACCOUNT       1
#define PAM_GROUP_SESSION       2
#define PAM_GROUP_PASSWORD      3
#define PAM_NUM_GROUPS          4

/* Control words */
#define PAM_CTL_REQUIRED        0
#define PAM_CTL_REQUISITE       1
#define PAM_CTL_SUFFICIENT      2
#define PAM_CTL_OPTIONAL        3

/* Which module function a stack run calls */
#define PAM_OP_AUTHENTICATE     0
#define PAM_OP_SETCRED          1
#define PAM_OP_ACCT_MGMT        2
#define PAM_OP_OPEN_SESSION     3
#define PAM_OP_CLOSE_SESSION    4
#define PAM_OP_CHAUTHTOK        5

/* A built-in module */
typedef LONG (*PamBuiltinFunc)(struct PamBase *PamBase, struct PamHandle *handle, ULONG flags, LONG argc, CONST_STRPTR *argv);

struct PamBuiltin
{
    CONST_STRPTR        Name;
    PamBuiltinFunc      Func[6];        /* indexed by PAM_OP_#?, NULL = PAM_IGNORE */
};

/* A loaded external module */
struct PamModule
{
    struct MinNode      Node;
    struct Library      *Base;
    ULONG               Users;
    char                Name[32];
};

/* One line of a service file */
struct PamEntry
{
    struct MinNode      Node;
    UBYTE               Group;
    UBYTE               Control;
    const struct PamBuiltin *Builtin;   /* one of the two is set */
    struct PamModule    *Module;
    LONG                Argc;
    CONST_STRPTR        *Argv;          /* points into Line */
    STRPTR              Line;           /* AllocVec'd copy of the arguments */
};

/* Module data */
struct PamData
{
    struct MinNode      Node;
    STRPTR              Name;
    APTR                Data;
    PamDataCleanup      Cleanup;
};

#define PAM_MAXTOKLEN           256

struct PamHandle
{
    struct PamBase      *Base;
    STRPTR              Service;
    STRPTR              User;
    STRPTR              UserPrompt;
    STRPTR              RHost;
    STRPTR              RUser;
    STRPTR              Tty;
    STRPTR              AuthTokType;
    STRPTR              AuthTok;            /* wiped on free/replace */
    STRPTR              OldAuthTok;
    IPTR                AuthTokFormat;      /* PAMTOK_#? */
    struct Hook         *Conv;
    APTR                ConvData;
    BOOL                Interactive;
    BPTR                Input;
    BPTR                Output;
    STRPTR              PubScreen;
    struct Task         *Task;
    ULONG               FailDelay;          /* microseconds requested by modules */
    struct MinList      Data;
    struct MinList      Stack[PAM_NUM_GROUPS];
    BOOL                StackLoaded;
    LONG                LastStatus;
    STRPTR              ErrorText;          /* PamStrError() scratch */
};

/* pam_handle.c */
extern STRPTR PamStrDup(struct PamBase *PamBase, CONST_STRPTR s);
extern void PamStrFree(struct PamBase *PamBase, STRPTR s, BOOL wipe);
extern BOOL PamSetString(struct PamBase *PamBase, STRPTR *slot, CONST_STRPTR s, BOOL wipe);

/* pam_config.c */
extern BOOL PamLoadStack(struct PamBase *PamBase, struct PamHandle *h);
extern void PamFreeStack(struct PamBase *PamBase, struct PamHandle *h);

/* pam_stack.c */
extern LONG PamRunStack(struct PamBase *PamBase, struct PamHandle *h, ULONG group, ULONG op, ULONG flags);

/* pam_conv.c */
extern LONG PamDoConverse(struct PamBase *PamBase, struct PamHandle *h, LONG nmsg, const struct pam_message **msgs, struct pam_response **resp);
extern void PamFreeResponses(struct PamBase *PamBase, LONG nmsg, struct pam_response *resp);

/* pam_modules.c */
extern struct PamModule *PamLoadModule(struct PamBase *PamBase, CONST_STRPTR name);
extern void PamReleaseModule(struct PamBase *PamBase, struct PamModule *mod);
extern void PamExpungeModules(struct PamBase *PamBase);

/* pam_builtin.c */
extern const struct PamBuiltin *PamFindBuiltin(CONST_STRPTR name);
extern const struct PamBuiltin PamBuiltinPermit, PamBuiltinDeny, PamBuiltinSecurity, PamBuiltinUnixPw;

/* machine state, for the default stacks and the modules */
#define PAM_STATE_ABSENT        0   /* no security.library                       */
#define PAM_STATE_UNCONFIGURED  1   /* resident, no password database            */
#define PAM_STATE_CONFIGURED    2
extern LONG PamSecurityState(struct PamBase *PamBase);

extern BOOL PamArgPresent(LONG argc, CONST_STRPTR *argv, CONST_STRPTR word);
extern CONST_STRPTR PamArgValue(LONG argc, CONST_STRPTR *argv, CONST_STRPTR key);

#endif /* PAM_INTERN_H */
