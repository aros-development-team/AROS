/*
    (C) 1999 AROS - The Amiga Research OS
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

struct IntReqToolsBase
{
    struct ReqToolsBase	rt;

    struct ExecBase 	*rt_SysBase;
/*
    struct Library 	*rt_LocaleBase;
    struct Library 	*rt_LayersBase;
*/
    struct IORequest 	rt_cdevio;   /* For communication with console.device */
};

/****************************************************************************************/

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

/****************************************************************************************/

/* Fix name clashes */
typedef  struct IntuitionBase  IntuiBase;

#define GPB(x) 		((struct IntReqToolsBase *)x)

#define expunge() \
AROS_LC0(BPTR, expunge, struct IntReqToolsBase *, RTBase, 3, ReqTools)

/****************************************************************************************/

#endif /* REQTOOLS_INTERN_H */
