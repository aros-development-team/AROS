/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

extern struct IntuitionBase 	*IntuitionBase;
extern struct GfxBase 		*GfxBase;
extern struct Library		*GadToolsBase;
extern struct Screen 		*scr;
extern struct DrawInfo 		*dri;
extern APTR			vi;
extern struct Menu		*menus;
extern struct Window		*win;

extern ULONG 			gotomask, findmask;
extern UBYTE			filenamebuffer[300];

/****************************************************************************************/

/* more.c */

VOID Cleanup(CONST_STRPTR msg);

/* locale.c */

void InitLocale(STRPTR catname, ULONG version);
void CleanupLocale(void);
CONST_STRPTR MSG(ULONG id);

/* misc.c */

void InitMenus(void);
void MakeMenus(void);
void KillMenus(void);
STRPTR GetFile(void);

/****************************************************************************************/
