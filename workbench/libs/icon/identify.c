/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/utility.h>
#include <proto/datatypes.h>

#include <string.h>

#   include <aros/debug.h>

#define SysBase       ((struct ExecBase *) iim->iim_SysBase)
#define DOSBase       ((struct DosLibrary *) iim->iim_DOSBase)
#define UtilityBase   (iim->iim_UtilityBase)
#define IconBase      (iim->iim_IconBase)
#define DataTypesBase (hook->h_Data)

AROS_UFH3
(
    struct DiskObject *, FindDefaultIcon,
    AROS_UFHA(struct Hook *,            hook,     A0),
    AROS_UFHA(APTR,                     reserved, A2),
    AROS_UFHA(struct IconIdentifyMsg *, iim,      A1) 
)
{
    struct DiskObject *icon          = NULL;
    
    /* Parse taglist -------------------------------------------------------*/
    STRPTR label = (STRPTR) GetTagData(ICONGETA_Label, NULL, iim->iim_Tags);
    
    /* Identify object -----------------------------------------------------*/
    if (DataTypesBase == NULL)
    {
        // FIXME: implement a primitive identification here?
        return NULL;
    }
    else
    {
        if (iim->iim_FIB->fib_DirEntryType == ST_ROOT)
        {
            // FIXME: not a very good way to detect it...
            /* It's a disk/volume/root -------------------------------------*/
            if (strcasecmp(label, "Ram Disk") == 0)
            {
                icon = GetIconTags
                (
                    NULL, 
                    ICONGETA_GetDefaultName, (IPTR) "RAM", 
                    TAG_MORE,                (IPTR) iim->iim_Tags
                );
            }
            else if
            (
                   strcasecmp(label, "System") == 0 
                || strcasecmp(label, "Home")   == 0
            )
            {
                icon = GetIconTags
                (
                    NULL, 
                    ICONGETA_GetDefaultName, (IPTR) "Harddisk", 
                    TAG_MORE,                (IPTR) iim->iim_Tags       
                );
            }
                
            // FIXME: return specific icons for CD, harddisk, floppy, etc
            
            if (icon == NULL)
            {
                icon = GetIconTags
                (
                    NULL, 
                    ICONGETA_GetDefaultType,        WBDISK, 
                    TAG_MORE,                (IPTR) iim->iim_Tags
                );
            }
        }
        else if (iim->iim_FIB->fib_DirEntryType > 0)
        {
            /* It's a directory --------------------------------------------*/
            // FIXME: detect trashcan
            icon = GetIconTags
            (
                NULL, 
                ICONGETA_GetDefaultType,        WBDRAWER, 
                TAG_MORE,                (IPTR) iim->iim_Tags
            );
        }
        else
        {
            /* It's a file -------------------------------------------------*/
            struct DataType *dt = ObtainDataType
            (
                DTST_FILE, iim->iim_FileLock, TAG_DONE
            );
            
            if (dt != NULL)
            {
                struct DataTypeHeader *dth = dt->dtn_Header;
                
                if
                (
                       dth->dth_GroupID == GID_SYSTEM 
                    && dth->dth_ID      == ID_EXECUTABLE
                )
                {
                    /* It's a exutable file --------------------------------*/
                    icon = GetIconTags
                    (
                        NULL,
                        ICONGETA_GetDefaultType,        WBTOOL, 
                        TAG_MORE,                (IPTR) iim->iim_Tags
                    );
                }
                else
                {
                    /* It's a project file of some kind --------------------*/
                    icon = GetIconTags
                    (
                        NULL,
                        ICONGETA_GetDefaultName, (IPTR) dth->dth_Name, 
                        TAG_MORE,                (IPTR) iim->iim_Tags
                    );
                    
                    if (icon == NULL)
                    {
                        STRPTR name = NULL;
                        
                        switch (dth->dth_GroupID)
                        {
                            case GID_SYSTEM:     name = "System";     break;
                            case GID_TEXT:       name = "Text";       break;
                            case GID_DOCUMENT:   name = "Document";   break;
                            case GID_SOUND:      name = "Sound";      break;
                            case GID_INSTRUMENT: name = "Instrument"; break;
                            case GID_MUSIC:      name = "Music";      break;
                            case GID_PICTURE:    name = "Picture";    break;
                            case GID_ANIMATION:  name = "Animation";  break;
                            case GID_MOVIE:      name = "Movie";      break;
                        }
                        
                        if (name != NULL)
                        {
                            icon = GetIconTags
                            (
                                NULL, 
                                ICONGETA_GetDefaultName, (IPTR) name,
                                TAG_MORE,                (IPTR) iim->iim_Tags
                            );
                        }
                    }
                    
                    if (icon == NULL)
                    {
                        icon = GetIconTags
                        (
                            NULL,
                            ICONGETA_GetDefaultType,        WBPROJECT,
                            TAG_MORE,                (IPTR) iim->iim_Tags
                        );
                    }
                }
                
                ReleaseDataType(dt);
            }
            else
            {
                icon = GetIconTags
                (
                    NULL,
                    ICONGETA_GetDefaultType,        WBPROJECT,
                    TAG_MORE,                (IPTR) iim->iim_Tags
                );
            }
        }
    }
    
    return icon;
}

#undef SysBase
#undef DOSBase
#undef UtilityBase
#undef IconBase
#undef DataTypesBase
