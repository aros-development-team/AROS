/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library - registered hardware authentication tokens
          (secKeyAdminA())
*/
#ifndef _SECURITY_KEYS_H
#define _SECURITY_KEYS_H

#include <exec/types.h>
#include <libraries/security.h>

struct SecurityBase;
struct secKARequest;

/* One registered token, as the server keeps it (SYS:Security/Keys/<user>) */
struct secKeyRec
{
    struct secKeyRec    *Next;
    ULONG               Type;                       /* secKEY_#?                                */
    char                Scheme[secKEYSCHEMESIZE];   /* the plugin's scheme                      */
    char                Label[secKEYLABELSIZE];
    char                PublicID[secKEYPUBIDSIZE];
    UBYTE               Secret[secKEYSECRETMAX];    /* the scheme's secret                      */
    ULONG               SecretLen;
    ULONG               Counter;                    /* last accepted                            */
    char                Registered[12];             /* yyyy-mm-dd                               */
};

/* Server context: the user's tokens (NULL if none); free with FreeKeys() */
extern struct secKeyRec *LoadKeys(struct SecurityBase *secBase, CONST_STRPTR userid);
extern void FreeKeys(struct secKeyRec *list);
extern BOOL UserHasKeys(struct SecurityBase *secBase, CONST_STRPTR userid);
/* Server context: a token's counter after an accepted code */
extern BOOL UpdateKeyCounter(struct SecurityBase *secBase, CONST_STRPTR userid, CONST_STRPTR publicid, ULONG counter);
/* Server context: secKeyAdminA() */
extern ULONG KeyAdmin(struct SecurityBase *secBase, struct Task *client, struct secKARequest *req);

/* Any context: the plugin of a record's scheme (NULL if none is loaded) */
extern struct plugin_hwauth_ops *KeyScheme(struct SecurityBase *secBase, ULONG type, CONST_STRPTR scheme);
/* Any context: the plugin calls for a record; secpiNOTSUPP without a plugin */
extern ULONG KeyMatchCode(struct SecurityBase *secBase, const struct secKeyRec *rec, CONST_STRPTR typed, ULONG *codelen);
extern ULONG KeyVerifyCode(struct SecurityBase *secBase, const struct secKeyRec *rec, CONST_STRPTR code, ULONG *counter);
extern ULONG RegisterHwAuthHandler(struct SecurityBase *secBase, struct plugin_ops *ops);
extern ULONG UnRegisterHwAuthHandler(struct SecurityBase *secBase, struct plugin_ops *ops);

#endif /* _SECURITY_KEYS_H */
