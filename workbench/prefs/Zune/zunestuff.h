#ifndef _ZUNE_ZUNESTUFF_H
#define _ZUNE_ZUNESTUFF_H

/*
    Copyright  2002-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <intuition/classusr.h>
#include <libraries/mui.h>
#include <libraries/asl.h>
#include "locale.h"

/* listview class */
extern struct MUI_CustomClass *ClassListview_CLASS;
struct MUI_CustomClass *create_listview_class(void);
void delete_listview_class(void);


struct page_entry
{
    char *name;
    struct MUI_CustomClass *cl; /* The class pointer,  maybe NULL */
    Object *group;  /* The group which should be is displayed, maybe NULL */
    Object *mcp_image;  /* Result of MCC_Query(2) */
    APTR mcp_listimage; /* mcp_image translated to list image */
    const struct __MUIBuiltinClass *desc;
    struct Library *mcp_library;
    UBYTE mcp_namebuffer[MAXFILENAMELENGTH + 1];
    UBYTE mcp_imagespec[30]; /* Image specification of MCP image */
};


Object *MakeButton (CONST_STRPTR str);
Object *MakeCycle (CONST_STRPTR label, CONST_STRPTR entries[]);
Object *MakeCheck (CONST_STRPTR label);
Object *MakeSpacingSlider (void);
Object *MakeBackgroundPopimage(void);
Object *MakePopframe(void);
Object *MakePoppen(void);
Object *MakeString(void);
Object *MakePopfont(BOOL fixed);
Object *MakePopfile(BOOL fixed, CONST_STRPTR pattern);

long aslfilerequest(char *msg,char *dirpart,char *filepart,char *fullname, struct TagItem *tags);

void SliderToConfig (Object *slider, Object *configdata, ULONG cfg);
void CheckmarkToConfig (Object *checkmark, Object *configdata, ULONG cfg);
void FrameToConfig (Object *popframe, Object *configdata, ULONG cfg);
void PenToConfig (Object *poppen, Object *configdata, ULONG cfg);
void CycleToConfig (Object *cycle, Object *configdata, ULONG cfg);
void StringToConfig (Object *string, Object *configdata, ULONG cfg);

void ConfigToSlider (Object *configdata, ULONG cfg, Object *slider);
void ConfigToCheckmark (Object *configdata, ULONG cfg, Object *checkmark);
void ConfigToFrame (Object *configdata, ULONG cfg, Object *popframe);
void ConfigToPen (Object *configdata, ULONG cfg, Object *poppen);
void ConfigToCycle (Object *configdata, ULONG cfg, Object *cycle);
void ConfigToString (Object *configdata, ULONG cfg, Object *string);

#ifndef __GNUC__
LONG XGET(Object * obj, ULONG attr);
#endif

#define getstring(obj) (char*)XGET(obj,MUIA_String_Contents)
#define FindFont(id) (void*)DoMethod(msg->configdata,MUIM_Dataspace_Find,id)

#ifdef __amigaos4__
Object *VARARGS68K DoSuperNewTags(struct IClass *cl, Object *obj, void *dummy, ...);
#endif


#endif /* _ZUNE_ZUNESTUFF_H */
