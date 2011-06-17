#ifndef PSI_H
#define PSI_H

/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <libraries/gadtools.h>
#include <libraries/mui.h>

#define CATCOMP_NUMBERS
#define CATCOMP_ARRAY
#include "strings.h"

/****************************************************************************/
/* Some Definitions                                                         */
/****************************************************************************/

#define MUISERIALNR_STUNTZI 1
#define TAGBASE_STUNTZI (TAG_USER | ( MUISERIALNR_STUNTZI << 16))

#define RectangleWidth(r)  ((r).MaxX-(r).MinX+1)
#define RectangleHeight(r) ((r).MaxY-(r).MinY+1)

#define SYSPEN_OFFSET   1
#define MUIPEN_OFFSET   1

#define ForEntries(list,entry,succ) for (entry=(APTR)((struct Node *)(((struct List *)list)->lh_Head));(succ=(APTR)((struct Node *)entry)->ln_Succ);entry=(APTR)succ)

#define BETWEEN(a,x,b) ((x)>=(a) && (x)<=(b))

CONST_STRPTR GetStr(int num);
char *getstr(Object *obj);
VOID setstr(APTR str,LONG num);
BOOL getbool(Object *obj);
VOID settxt(APTR str,LONG num);
IPTR xget(Object *obj,IPTR attribute);
Object *MakeLabel    (int num);
Object *MakeCLabel(int num);
Object *MakeLabel1   (int num);
Object *MakeLabel2   (int num);
Object *MakeLLabel   (int num);
Object *MakeLLabel1  (int num);
Object *MakeFreeLabel(int num);
Object *MakeFreeLLabel(int num);
Object *MakeButton(int num);
Object *MakeString(int maxlen,int num);
Object *MakeCheck(int num);
Object *MakeCycle(CONST_STRPTR *array,int num);
Object *MakeSlider(int min,int max,int num);
Object *MakePalette(void);
BOOL TestPubScreen(char *name);

#endif
