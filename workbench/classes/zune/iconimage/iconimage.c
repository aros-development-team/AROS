/*
    Copyright � 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <dos/dos.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include <string.h>

#include "iconimage.h"

#define DEBUG 1
#include <aros/debug.h>

/*** Instance data **********************************************************/

struct IconImage_DATA
{
    struct DiskObject *iid_DiskObject;
};

IPTR DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
  return DoSuperMethod(cl, obj, OM_NEW, (IPTR) &tag1, NULL);
}

/*** Methods ****************************************************************/

IPTR IconImage$OM_NEW
(
    struct IClass *CLASS, Object *self, struct opSet *message 
)
{
    struct IconImage_DATA *data       = NULL; 
    struct TagItem        *tag        = NULL, *tstate = message->ops_AttrList;    
    struct DiskObject     *diskObject = NULL;
    CONST_STRPTR           file       = NULL;
    
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_IconImage_DiskObject:
                diskObject = (struct DiskObject *) tag->ti_Data;
                break;
                
            case MUIA_IconImage_File:
                file = (CONST_STRPTR) tag->ti_Data;
                break;
            
            default:
                continue; /* Don't supress non-processed tags */
        }
        
        tag->ti_Tag = TAG_IGNORE;
    }
    
    if (diskObject == NULL && file == NULL) goto error; /* Must specify one */
    if (diskObject != NULL && file != NULL) goto error; /* Cannot specify both */
    
    if (diskObject == NULL && file != NULL)
    {
        diskObject = GetDiskObjectNew(file);
        if (diskObject == NULL) goto error;
    }
    
    self = (Object *) DoSuperNew
    (
        CLASS, self,
        
        MUIA_FillArea,        FALSE,
        TAG_MORE,      (IPTR) message->ops_AttrList
    );
    
    if (self == NULL) goto error;
    
    data = INST_DATA(CLASS, self);
    data->iid_DiskObject = diskObject;

    return self;
    
error:
    return NULL;
}

IPTR IconImage$MUIM_Draw
(
    struct IClass *CLASS, Object *self, struct MUIP_Draw *message
)
{
    struct IconImage_DATA *data = INST_DATA(CLASS, self); 
    IPTR                   rc   = DoSuperMethodA(CLASS, self, (Msg) message);
    BOOL                   selected;
    
    DoMethod
    (
        self, MUIM_DrawParentBackground,  
        _mleft(self), _mtop(self), _mwidth(self), _mheight(self), 0, 0, 0
    );
    
    get(self, MUIA_Selected, &selected);
    
    DrawIconState
    (
        _rp(self), data->iid_DiskObject, NULL, 
        _mleft(self), _mtop(self), selected ? IDS_SELECTED : IDS_NORMAL, 
        
        ICONDRAWA_Frameless,       TRUE,
        ICONDRAWA_Borderless,      TRUE,
        ICONDRAWA_EraseBackground, FALSE,
        TAG_DONE
    );
    
    return rc;
}

IPTR IconImage$MUIM_AskMinMax
(
    struct IClass *CLASS, Object *self, struct MUIP_AskMinMax *message
)
{
    struct IconImage_DATA *data = INST_DATA(CLASS, self);
    IPTR                   rc   = DoSuperMethodA(CLASS, self, (Msg) message);
    struct Rectangle       size;
    
    memset(&size, 0, sizeof(struct Rectangle));
    
    if
    (
        GetIconRectangle
        (
            _rp(self), data->iid_DiskObject, NULL, &size, 
            
            ICONDRAWA_Borderless, TRUE, 
            TAG_DONE 
        )
    )
    {
        message->MinMaxInfo->MinWidth  += size.MaxX;
        message->MinMaxInfo->MaxWidth  += size.MaxX;
        message->MinMaxInfo->DefWidth  += size.MaxX;
        message->MinMaxInfo->MinHeight += size.MaxY;
        message->MinMaxInfo->MaxHeight += size.MaxY;
        message->MinMaxInfo->DefHeight += size.MaxY;
    }

    return rc;
}
