#ifndef ENVOY_ACCOUNTS_H
#define ENVOY_ACCOUNTS_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy accounts.library public definitions (as published in the
          Envoy developer kit).
*/

#include <exec/types.h>

struct AuthorityInfo
{
    UBYTE   ai_UserName[32];
    UBYTE   ai_Password[32];
};

struct UserInfo
{
    UBYTE   ui_UserName[32];
    UWORD   ui_UserID;
    UWORD   ui_PrimaryGroupID;
    ULONG   ui_Flags;
};

struct GroupInfo
{
    UBYTE   gi_GroupName[32];
    UWORD   gi_GroupID;
    UWORD   gi_AdminID;
    ULONG   gi_Flags;
};

/* Error codes returned by accounts.library */
#define ACCERROR_NORESOURCES    100
#define ACCERROR_NOPRIVS        101
#define ACCERROR_NOAUTHORITY    102
#define ACCERROR_UNKNOWNUSER    103     /* obsolete: ENVOYERR_UNKNOWNUSER   */
#define ACCERROR_UNKNOWNGROUP   104     /* obsolete: ENVOYERR_UNKNOWNGROUP  */
#define ACCERROR_LASTUSER       105     /* obsolete: ENVOYERR_LASTUSER      */
#define ACCERROR_LASTGROUP      106     /* obsolete: ENVOYERR_LASTGROUP     */
#define ACCERROR_LASTMEMBER     107     /* obsolete: ENVOYERR_LASTMEMBER    */
#define ACCERROR_GROUPEXISTS    108
#define ACCERROR_NOFREEGROUPS   109
#define ACCERROR_UNKNOWNMEMBER  110     /* obsolete: ENVOYERR_UNKNOWNMEMBER */
#define ACCERROR_NOFREEUSERS    111
#define ACCERROR_USEREXISTS     112

/* ui_Flags */
#define UFLAGB_AdminName        0       /* may change their own name            */
#define UFLAGB_AdminPassword    1       /* may change their own password        */
#define UFLAGB_NeedsPassword    2       /* a password is required               */
#define UFLAGB_AdminGroups      3       /* may create and delete groups         */
#define UFLAGB_AdminAll         4       /* may do anything                      */

#define UFLAGF_AdminName        (1 << UFLAGB_AdminName)
#define UFLAGF_AdminPassword    (1 << UFLAGB_AdminPassword)
#define UFLAGF_NeedsPassword    (1 << UFLAGB_NeedsPassword)
#define UFLAGF_AdminGroups      (1 << UFLAGB_AdminGroups)
#define UFLAGF_AdminAll         (1 << UFLAGB_AdminAll)

#endif /* ENVOY_ACCOUNTS_H */
