/*
 *  ADF Library. (C) 1997-1998 Laurent Clevy
 *
 *  adf_raw.h
 *
 */

#ifndef _ADF_RAW_H
#define _ADF_RAW_H 1

#include "adf_str.h"

#define SW_LONG  4
#define SW_SHORT 2
#define SW_CHAR  1

#define MAX_SWTYPE 12

#define SWBL_BOOTBLOCK    0
#define SWBL_ROOT         1
#define SWBL_DATA         2
#define SWBL_FILE         3
#define SWBL_ENTRY        3
#define SWBL_DIR          3
#define SWBL_CACHE        4
#define SWBL_BITMAP       5
#define SWBL_FEXT         5
#define SWBL_LINK         6
#define SWBL_BITMAPE      5
#define SWBL_RDSK         7
#define SWBL_BADB         8
#define SWBL_PART         9
#define SWBL_FSHD         10 
#define SWBL_LSEG         11
#define SWBL_BOOT         12    /* AmigaOS 4 Second Level Boot blocks */

RETCODE adfReadRootBlock(struct Volume*, ULONG nSect, struct bRootBlock* root);
RETCODE adfWriteRootBlock(struct Volume* vol, ULONG nSect, struct bRootBlock* root);
RETCODE adfReadBootBlock(struct Volume*, struct bBootBlock* boot);
RETCODE adfWriteBootBlock(struct Volume* vol, struct bBootBlock* boot);

ULONG adfBootSum(unsigned char *buf);
ULONG adfNormalSum( unsigned char *buf, int offset, int bufLen );

void swapEndian( unsigned char *buf, int type );

#endif /* _ADF_RAW_H */

/*##########################################################################*/
