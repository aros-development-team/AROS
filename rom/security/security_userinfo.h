/************************************************************
* MultiUser - MultiUser Task/File Support System				*
* ---------------------------------------------------------	*
* User Information Management											*
* ---------------------------------------------------------	*
* © Copyright 1993-1994 Geert Uytterhoeven						*
* All Rights Reserved.													*
************************************************************/

#include <libraries/mufs.h>

/*
 *		Private User Information Structure
 *
 *		This is a sub class of the Public User Information Structure
 */

struct secPrivUserInfo {
    struct secUserInfo  Pub;            /* The public part                      */
    BOOL                Password;       /* TRUE if the User has a password      */
    STRPTR              Pattern;        /* Pattern matching temp                */
    ULONG               Count;          /* last info                            */
    UWORD               Tgid;           /* gid for muKeyType_gidNext            */
};
