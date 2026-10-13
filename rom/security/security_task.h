/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library task credential tracking
*/
#ifndef _SECURITY_TASK_H
#define _SECURITY_TASK_H

#include <exec/lists.h>
#include <exec/tasks.h>
#include <utility/hooks.h>
#include <libraries/security.h>

#ifndef TASKHASHVALUE
#define TASKHASHVALUE 23
#endif

struct secTaskNode;
struct SecurityBase;

/*
 * A session groups all tasks descending from one login.
 */
struct secSession
{
    struct MinNode      Node;                   /* Links into secBase->SessionsList         */
    struct MinList      SessionMembers;         /* secTaskNode.SessionNode links            */
    struct secTaskNode  *Leader;                /* Task that created the session            */
    STRPTR              LoginName;              /* UserID used to log in (owned copy)       */
    int                 sid;                    /* Session ID number                        */
    ULONG               MemberCount;
};

struct secContextList                           /* list of context nodes */
{
    struct MinNode      Node;
    struct MinList      Context;
};

struct secContextNode                           /* variable length */
{
    struct MinNode      Node;
    secPluginModule     *mod;                   /* 'Owning' module      */
    ULONG               id;                     /* module dependent id  */
    /* memory for the context starts here */
};

/* Capabilities (reserved for the finer grained privilege model) */
struct secCaps
{
    ULONG               effective;
    ULONG               inheritable;
    ULONG               permitted;
};

/* Saved credentials, pushed on login and popped on logout */
struct secOwnerStackEntry
{
    struct MinNode      Node;
    struct secExtOwner  *Owner;                 /* NULL = nobody */
    struct secSession   *Session;
    UWORD               RealUID, RealGID;
    UWORD               SavedUID, SavedGID;
};

struct secTaskNode
{
    struct MinNode      SessionNode;            /* Links into SessionMembers of our secSession  */
    struct MinNode      ListNode;               /* Link into secBase->TaskOwnerList             */
    struct MinNode      Siblings;               /* Links into our parent's Children list        */
    struct secSession   *Session;               /* Session we belong to; may be NULL            */
    struct MinList      Children;               /* Children of this task                        */
    struct secTaskNode  *Parent;                /* our parent                                   */
    struct Task         *Task;                  /* Exec's data                                  */
    ULONG               DefProtection;          /* 'umask'                                      */
    UWORD               RealUID, SavedUID;      /* Extra credentials for POSIX                  */
    UWORD               RealGID, SavedGID;
    UWORD               FSUID, FSGID;           /* optional; for NFS                            */
    struct secExtOwner  *Owner;                 /* Effective UID/GID + secondary groups         */
                                                /* NULL if task is owned by nobody              */
    UWORD               ChildrenCount;          /* How many children do I have?                 */
    UWORD               Flags;                  /* see below                                    */
    int                 pid;
    struct MinList      Context;                /* List of context lists - used by plugins      */
    struct secCaps      Capabilities;           /* What we are allowed to do                    */
    /* setuid support: owner saved while running a setuid segment */
    struct secExtOwner  *SetUIDSavedOwner;
    UWORD               SetUIDSavedRealUID;
    UWORD               SetUIDDepth;
    /* login/logout stack */
    struct MinList      OwnerStack;
    /* freeze support: the exec state the task had when it was frozen */
    UBYTE               FrozenState;
};

/* secTaskNode.Flags */
#define secTNF_Frozen           (1 << 0)
#define secTNF_Zombie           (1 << 1)
#define secTNF_NoLimit          (1 << 2)    /* inside secSetProtection(): bypass LIMITDOSSETPROTECTION */

/* Extended Owner Information Structure for root */
extern const struct secExtOwner RootExtOwner;

/* container-of helpers */
#define TASKNODE_FROM_LISTNODE(n)   ((struct secTaskNode *)((IPTR)(n) - offsetof(struct secTaskNode, ListNode)))
#define TASKNODE_FROM_SIBLINGS(n)   ((struct secTaskNode *)((IPTR)(n) - offsetof(struct secTaskNode, Siblings)))
#define TASKNODE_FROM_SESSION(n)    ((struct secTaskNode *)((IPTR)(n) - offsetof(struct secTaskNode, SessionNode)))

/*
 * Private Function Prototypes
 */
extern BOOL InitTaskList(struct SecurityBase *secBase);
extern void CleanUpTaskList(struct SecurityBase *secBase);

/* Callers of the following must hold TaskOwnerSem (exclusive for anything that modifies) */
extern struct secTaskNode *AllocTaskNode(struct SecurityBase *secBase, struct Task *task, ULONG defprotection, struct secTaskNode *parent);
extern void FreeTaskNode(struct SecurityBase *secBase, struct secTaskNode *node);
extern void AddTaskNode(struct SecurityBase *secBase, struct secTaskNode *node, struct secTaskNode *parent);
extern void RemTaskNode(struct SecurityBase *secBase, struct secTaskNode *node);
extern struct secTaskNode *FindTaskNode(struct SecurityBase *secBase, struct Task *task);
extern struct secTaskNode *FindTaskNodePid(struct SecurityBase *secBase, int pid);
extern struct secTaskNode *FindOrCreateTaskNode(struct SecurityBase *secBase, struct Task *task);

/* Login stack helpers - TaskOwnerSem must be held exclusively */
extern BOOL PushOwner(struct SecurityBase *secBase, struct secTaskNode *node);
extern BOOL PopOwner(struct SecurityBase *secBase, struct secTaskNode *node);
extern void ClearOwnerStack(struct SecurityBase *secBase, struct secTaskNode *node);
/* Set the owner of a node (copied); TaskOwnerSem must be held exclusively */
extern BOOL SetNodeOwner(struct SecurityBase *secBase, struct secTaskNode *node, const struct secExtOwner *owner);

/* These obtain the semaphore themselves */
extern struct secTaskNode *CreateOrphanTask(struct SecurityBase *secBase, struct Task *task, ULONG defprotection);
extern ULONG GetTaskOwner(struct SecurityBase *secBase, struct Task *task);
extern struct secExtOwner *GetTaskExtOwner(struct SecurityBase *secBase, struct Task *task);
extern BOOL SetTaskExtOwner(struct SecurityBase *secBase, struct Task *task, const struct secExtOwner *owner);
extern ULONG GetTaskDefProtection(struct SecurityBase *secBase, struct Task *task);
extern BOOL SetTaskDefProtection(struct SecurityBase *secBase, struct Task *task, ULONG defprotection);
extern BOOL CallerIsRoot(struct SecurityBase *secBase);

extern struct secExtOwner *CloneExtOwner(const struct secExtOwner *owner);
extern ULONG ExtOwnerSize(const struct secExtOwner *owner);

/* Sessions - TaskOwnerSem must be held exclusively */
extern struct secSession *AllocSession(struct SecurityBase *secBase, CONST_STRPTR loginname);
extern void FreeSession(struct SecurityBase *secBase, struct secSession *session);
extern void JoinSession(struct SecurityBase *secBase, struct secTaskNode *node, struct secSession *session);
extern void LeaveSession(struct SecurityBase *secBase, struct secTaskNode *node);

#endif /* _SECURITY_TASK_H */
