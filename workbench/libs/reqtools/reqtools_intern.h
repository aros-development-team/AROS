/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/****************************************************************************************/

#ifndef REQTOOLS_INTERN_H
#define REQTOOLS_INTERN_H

/****************************************************************************************/

#include <exec/execbase.h>
#include <dos/dos.h>
#include <libraries/reqtools.h>
#include <libraries/locale.h>
#include <intuition/intuition.h>

/****************************************************************************************/

#ifdef __AROS__

/* filereqalloc.c */

struct RealFileRequester;

APTR AllocRequestA (ULONG type, struct TagItem *taglist);
void FreeRequest (APTR);
void FreeReqBuffer (APTR);
LONG ChangeReqAttrA (APTR, struct TagItem *);
APTR FileRequestA(struct RealFileRequester *,char *,char *,struct TagItem *);
void FreeFileList (struct rtFileList *);

/* general.c */

int GetVScreenSize (struct Screen *scr, int *width, int *height);

/* req.c */

ULONG GetString (UBYTE *stringbuff, LONG maxlen, char *title,
		 ULONG checksum, ULONG *value, LONG mode,
	         struct rtReqInfo *reqinfo, struct TagItem *taglist);

/* palettereq.c */

LONG PaletteRequestA (char *title, struct rtReqInfo *reqinfo, struct TagItem *taglist);

#endif

/****************************************************************************************/

struct ReqToolsBase_intern {
    struct ReqToolsBase ReqToolsBase;
    
    struct ExecBase *rt_SysBase;
};

#define GPB(x) 		((struct ReqToolsBase *)x)

/****************************************************************************************/

#endif /* REQTOOLS_INTERN_H */
