/*
    Copyright (C) 2003-2026, The AROS Development Team. All rights reserved.
    This file is part of the IconImage class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <dos/dos.h>

#include <proto/alib.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include <string.h>

#include "iconimage.h"
#include "iconimage_private.h"


/****** IconImage.mcc/MUIC_IconImage ************
*
*   NAME
*       MUIC_IconImage
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Display a Workbench icon without its label or border. Supply one
*       non-NULL DiskObject or File attribute when creating the object.
*       The last occurrence of either attribute takes precedence.
*       Drawing uses the current selected state and does not scale the icon.
*
*   NOTES
*       The class borrows DiskObject pointers and has no local Set, Get, or
*       Dispose handler. LayoutIcon adjusts the icon for the current screen.
*
*   BUGS
*       Icons loaded through File are never freed by this class, including
*       after construction failure. Prefer a caller-owned DiskObject when
*       resource cleanup is required.
*
*/
/****** IconImage.mcc/MUIA_IconImage_DiskObject ************
*
*   NAME
*       MUIA_IconImage_DiskObject
*
*   FUNCTION
*       Use an existing DiskObject for the displayed icon.
*
*   INPUTS
*       Initialize only; struct DiskObject *.
*
*   NOTES
*       The object borrows the pointer without copying or freeing it. Keep
*       the icon valid until this object is disposed. LayoutIcon may change
*       its screen-dependent layout. Do not supply a non-NULL File as well.
*
*/
/****** IconImage.mcc/MUIA_IconImage_File ************
*
*   NAME
*       MUIA_IconImage_File
*
*   FUNCTION
*       Load an icon from a file name during object construction.
*
*   INPUTS
*       Initialize only; CONST_STRPTR.
*
*   NOTES
*       GetDiskObjectNew() may return a default icon. The file name is not
*       retained. Do not supply a non-NULL DiskObject as well.
*
*   BUGS
*       The loaded icon is not released on disposal or construction failure.
*
*/

/*** Methods ****************************************************************/
/* Select one icon source; return NULL if it is missing or ambiguous.
   Icons loaded from File are not released on later failure. */
Object *IconImage__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct IconImage_DATA *data       = NULL;
    struct TagItem        *tstate = message->ops_AttrList;
    struct TagItem        *tag        = NULL;
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
        }
    }
    
    if (diskObject == NULL && file == NULL) goto error; /* Must specify one */
    if (diskObject != NULL && file != NULL) goto error; /* Cannot specify both */
    
    if (diskObject == NULL && file != NULL)
    {
        diskObject = GetDiskObjectNew(file);
        if (diskObject == NULL) goto error;
    }
    
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
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

/* Draw the parent background and selected/normal icon without a border.
   The icon functions' results are not checked. */
IPTR IconImage__MUIM_Draw
(
    Class *CLASS, Object *self, struct MUIP_Draw *message
)
{
    struct IconImage_DATA *data = INST_DATA(CLASS, self);
    IPTR                   rc   = DoSuperMethodA(CLASS, self, (Msg) message);
    IPTR                   selected = 0;
    
    DoMethod
    (
        self, MUIM_DrawParentBackground,
        _mleft(self), _mtop(self), _mwidth(self), _mheight(self), 0, 0, 0
    );

    get(self, MUIA_Selected, &selected);

    LayoutIcon(data->iid_DiskObject, _screen(self), TAG_END);

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

/* Add the borderless icon's inclusive rectangle dimensions to superclass
   limits when GetIconRectangle succeeds. */
IPTR IconImage__MUIM_AskMinMax
(
    Class *CLASS, Object *self, struct MUIP_AskMinMax *message
)
{
    struct IconImage_DATA *data = INST_DATA(CLASS, self);
    IPTR                   rc   = DoSuperMethodA(CLASS, self, (Msg) message);
    struct Rectangle       size;
    
    memset(&size, 0, sizeof(struct Rectangle));
    
    LayoutIcon(data->iid_DiskObject, _screen(self), TAG_END);
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
        WORD w = size.MaxX - size.MinX + 1;
        WORD h = size.MaxY - size.MinY + 1;
        
        message->MinMaxInfo->MinWidth  += w;
        message->MinMaxInfo->MaxWidth  += w;
        message->MinMaxInfo->DefWidth  += w;
        message->MinMaxInfo->MinHeight += h;
        message->MinMaxInfo->MaxHeight += h;
        message->MinMaxInfo->DefHeight += h;
    }

    return rc;
}
