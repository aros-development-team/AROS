#ifndef _SECURITY_SERVER_H
#define _SECURITY_SERVER_H

#define SERVERNAME		"Heimdall.server"

/*
 *      Private Server Packet
 */

struct secSPacket {
    struct Message      Msg;
    LONG                Type;                   /* See definitions below */
    SIPTR                Arg1;
    SIPTR                Arg2;
    SIPTR                Arg3;
    SIPTR                Arg4;
    SIPTR                Res1;
    ULONG               Stage;                  /* How far the request has gone: index into plugin stack for this operation */
    ULONG               AsyncResult;            /* Result of an asynchronous operation : Plugins */
};

#define secSAction_Quit                 0       /* Server Quit */
#define secSAction_CheckUser            1       /* Login User */
#define secSAction_Passwd               2       /* Change User Password */
#define secSAction_GetUserInfo          3       /* Get User Information */
#define secSAction_CheckPasswd          4       /* Check Password */
#define secSAction_PasswdDirLock        5       /* Get PasswdDirLock */
#define secSAction_ConfigDirLock        6       /* Get ConfigDirLock */
#define secSAction_GetGroupInfo         7       /* Get Group Information */
#define secSAction_InitModule		8       /* Call the init function for a module */
#define secSAction_FiniModule		9       /* Call the fini function for a module */

/*
 *		Private Function Prototypes
 */

extern struct Process *CreateServer(struct SecurityBase *secBase);
extern BOOL StartServer(struct SecurityBase *secBase);
extern SIPTR SendServerPacket(struct SecurityBase *secBase, SIPTR type, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4);

#endif /* _SECURITY_SERVER_H */
