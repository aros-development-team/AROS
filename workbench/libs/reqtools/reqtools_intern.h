/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

#ifdef _AROS

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

/* Fix name clashes */
typedef  struct IntuitionBase  IntuiBase;

#define GPB(x) 		((struct ReqToolsBase *)x)

#ifdef _AROS
#define expunge() \
AROS_LC0(BPTR, expunge, struct ReqToolsBase *, RTBase, 3, ReqTools)
#endif

/****************************************************************************************/

#endif /* REQTOOLS_INTERN_H */
