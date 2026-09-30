/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.

    Desc: BCPL support
*/

#define BCPL_GlobVec_NegSize	0xb0
#define BCPL_GlobVec_PosSize	0x21c

/* Our BCPL stub private data */

#define GV_DEBUG_Result2	-0xac
#define GV_PktWaitFrame         -0xa8   /* Active BCPL frame at a C entry */
#define GV_PktWait              0x190   /* Legacy packet-wait callback */
#define GV_IntuitionBase        0x170   /* Not a function, but a value */
#define GV_DOSBase              0x204   /* Not a function, but a value */

/* We can add private data up to -0x88 */
