/*
    Copyright (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

/****************************************************************************************/

#ifndef LINKLIBS_COOLIMAGES_H
#define LINKLIBS_COOLIMAGES_H

/****************************************************************************************/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

struct Library;

/****************************************************************************************/

/* Tags for coolbuttonclass */

#define COOLBT_Dummy	    (TAG_USER + 1234)

#define COOLBT_CoolImage    (COOLBT_Dummy + 1) /* I: struct CoolImage * */

/* Tags for coolimageclass */

#define COOLIM_Dummy	    (TAG_USER + 1334)

#define COOLIM_CoolImage    (COOLIM_Dummy + 1) /* I: struct CoolImage * */
#define COOLIM_BgColor	    (COOLIM_Dummy + 2) /* I: ULONG  	    	*/

/****************************************************************************************/

struct CoolImage
{
    const UBYTE	*data;
    const UBYTE	*pal;
    WORD 	width;
    WORD	height;
    WORD	numcolors;
};

/****************************************************************************************/

extern const struct CoolImage cool_saveimage;
extern const struct CoolImage cool_loadimage;
extern const struct CoolImage cool_useimage;
extern const struct CoolImage cool_cancelimage;
extern const struct CoolImage cool_dotimage;
extern const struct CoolImage cool_dotimage2;
extern const struct CoolImage cool_warnimage;
extern const struct CoolImage cool_diskimage;
extern const struct CoolImage cool_switchimage;
extern const struct CoolImage cool_monitorimage;
extern const struct CoolImage cool_mouseimage;
extern const struct CoolImage cool_infoimage;
extern const struct CoolImage cool_askimage;
extern const struct CoolImage cool_keyimage;
extern const struct CoolImage cool_clockimage;
extern const struct CoolImage cool_flagimage;
extern const struct CoolImage cool_headimage;

extern struct IClass * cool_buttonclass;
extern struct IClass * cool_imageclass;

/****************************************************************************************/

BOOL InitCoolButtonClass(struct Library *CyberGfxBase);
BOOL InitCoolImageClass(struct Library *CyberGfxBase);

void CleanupCoolButtonClass(void);
void CleanupCoolImageClass(void);

/****************************************************************************************/

#endif /* LINKLIBS_COOLIMAGES_H */

/****************************************************************************************/
