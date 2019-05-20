#ifndef _SECURITY_TASK_H
#define _SECURITY_TASK_H

#ifndef TASKHASHVALUE
#define TASKHASHVALUE 23
#endif

struct secTaskNode;
struct SecurityBase;

struct secSession {
    struct MinNode      Session;
    struct MinList      SessionMembers;
    struct secTaskNode  *Leader;
    STRPTR              LoginName;
    int                 sid;                    /* Session ID number */
};

struct secContextList	{                       /* list of context nodes */
    struct MinNode      Node;
    struct MinList      Context;
};

struct secContextNode	{	                /* variable length */
    struct MinNode      Node;
    secPluginModule     *mod;                   /* 'Owning' module */
    ULONG               id;                     /* module dependent id */
    /* memory for the context starts here */
};

/*      
    Capabilities
*/
struct secCaps	{
    ULONG               effective;
    ULONG               inheritable;
    ULONG               permitted;
};

struct secTaskNode {
    struct MinNode      SessionNode;            /* Links into SessionMembers in our secSession */
    struct MinNode      ListNode;		/* Link into secBase->TaskOwnerList */
    struct MinNode      Siblings;		/* Links into our parents Children list */
    struct secSession   *Session;		/* Session we belong to; may be NULL */
    struct MinList      Children;		/* Children of this task */
    struct secTaskNode  *Parent;                /* our parent */
    struct Task         *Task;			/* Exec's data */
    struct MinList      OwnedMem;		/* RT. Unused at the moment */
    ULONG               DefProtection;          /* 'umask' */
    UWORD               RealUID, SavedUID;      /* Extra credentials for POSIX */
    UWORD               RealGID, SavedGID;
    UWORD               FSUID, FSGID;           /* optional; for NFS */
    struct secExtOwner  *Owner;			/* Effective UID/GID + secondary groups */
                                                /* May be NULL if task is owned by nobody */
    UWORD               ChildrenCount;          /* How many children do I have? */
    int                 pid;
    struct MinList      Context;                /* List of context lists - used by plugin modules */
    struct secCaps      Capabilities;           /* What we are allowed to do */
};

#if !defined(ROOTEXTINLIBBASE)
/*
 *      Extended Owner Information Structure for root
 */

extern const struct secExtOwner RootExtOwner;
#endif

/*
 *      Private Function Prototypes
 */

extern void CleanUpBody(void);

extern void InitTaskList(struct SecurityBase *secBase);
extern struct secTaskNode *AllocTaskNode(struct Task *task, ULONG defprotection, struct secTaskNode *parent);
extern void FreeTaskNode(struct secTaskNode *node);
extern struct secTaskNode *CreateOrphanTask(struct SecurityBase *secBase, struct Task *task, ULONG defprotection);
extern struct secTaskNode *FindTaskNode(struct SecurityBase *secBase, struct Task *task);
extern struct secTaskNode *FindTaskNodePid(struct SecurityBase *secBase, int pid);
extern ULONG GetTaskOwner(struct SecurityBase *secBase, struct Task *task);
extern struct secExtOwner *GetTaskExtOwner(struct SecurityBase *secBase, struct Task *task);
extern BOOL SetTaskExtOwner(struct SecurityBase *secBase, struct Task *task, struct secExtOwner *owner);

#endif /* _SECURITY_TASK_H */
