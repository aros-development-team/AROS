/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$ 
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classusr.h>
#include <libraries/desktop.h>
#include <libraries/mui.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "desktop_intern.h"
#include "support.h"

#include "iconclass.h"
#include "iconobserver.h"
#include "iconcontainerclass.h"
#include "iconcontainerobserver.h"
#include "observer.h"
#include "presentation.h"

#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */

        #include <proto/desktop.h>

        AROS_LH2(Object *, CreateDesktopObjectA,
     /*
        SYNOPSIS 
      */
            AROS_LHA(ULONG, kind, D0), 
            AROS_LHA(struct TagItem *, tags, A0),
     /*
        LOCATION 
      */
            struct DesktopBase *, DesktopBase, 8, Desktop)
/*
    FUNCTION
    
    INPUTS
    
    RESULT
    
    NOTES This function is sloppy - sort it out!
    
    EXAMPLE
    
    BUGS
    
    SEE ALSO
    
    INTERNALS
    
    *****************************************************************************
*/
{
    AROS_LIBFUNC_INIT 
    
    Object *newObject      = NULL,
           *semanticObject = NULL;

    switch (kind)
    {
        case CDO_IconContainer:
            {
                STRPTR          dir;
                struct TagItem *tag;

                tag = FindTagItem(ICOA_Directory, tags);
                if (tag)
                {
                    dir = tag->ti_Data;
                    tag->ti_Tag = TAG_IGNORE;
                }

                newObject = NewObjectA(IconContainer->mcc_Class, NULL, tags);

                semanticObject = NewObject
                (
                    IconContainerObserver->mcc_Class, NULL,
                    
                    OA_Presentation, newObject, 
                    ICOA_Directory,  dir,
                    
                    TAG_END
                );

                set(newObject, PA_Observer, semanticObject);

                break;
            }

        case CDO_DiskIcon:
            {
                STRPTR          label   = NULL;
                struct TagItem *labelTI = FindTagItem(IA_Label, tags);
                
                if (labelTI != NULL)
                {
                    label = (STRPTR) labelTI->ti_Data;
                }
                
                newObject = NewObjectA(DiskIcon->mcc_Class, NULL, tags);
                
                semanticObject = NewObject
                (       
                    DiskIconObserver->mcc_Class, NULL, 
                    
                    IOA_Name,        label, 
                    OA_Presentation, newObject,
                                    
                    TAG_DONE
                );

                break;
            }

        case CDO_DrawerIcon:
            {
                STRPTR          label       = NULL,
                                directory   = NULL;
                struct TagItem *labelTI     = FindTagItem(IA_Label, tags),
                               *directoryTI = FindTagItem(IOA_Directory, tags);
                
                if (labelTI != NULL)
                {
                    label = (STRPTR) labelTI->ti_Data;
                }
                
                if (directoryTI != NULL)
                {
                    directory = (STRPTR) directoryTI->ti_Data;
                }
                
                newObject = NewObjectA(DrawerIcon->mcc_Class, NULL, tags);

                semanticObject = NewObject
                (
                    DrawerIconObserver->mcc_Class, NULL, 
                
                    IOA_Name,        label,
                    OA_Presentation, newObject,
                    IOA_Directory,   directory,
                
                    TAG_DONE
                );

                break;
            }

        case CDO_ToolIcon:

            newObject = NewObjectA(ToolIcon->mcc_Class, NULL, tags);

            semanticObject = NewObject(ToolIconObserver->mcc_Class, NULL,
                                       OA_Presentation, newObject, TAG_END);
            break;

        case CDO_ProjectIcon:
            newObject = NewObjectA(ProjectIcon->mcc_Class, NULL, tags);
            
            semanticObject = NewObject
            (
                ProjectIconObserver->mcc_Class, NULL,
                OA_Presentation, newObject, 
                TAG_END
            );
            break;

        case CDO_TrashcanIcon:
            newObject = NewObjectA(TrashcanIcon->mcc_Class, NULL, tags);

            semanticObject = NewObject
            (
                TrashcanIconObserver->mcc_Class, NULL,
                OA_Presentation, newObject, 
                TAG_END
            );
            break;

        case CDO_Desktop:
            newObject = NewObjectA
            (
                DesktopBase->db_Desktop->mcc_Class, NULL, tags
            );

            semanticObject = NewObject
            (
                DesktopObserver->mcc_Class, NULL,
                OA_Presentation, newObject, 
                TAG_END
            );
            
            break;

        case CDO_DirectoryWindow:
            {
                Class  *windowClass  = NULL;
                Object *windowObject = NULL;

                if (DesktopBase->db_DefaultWindow) 
                {
                    windowClass = DesktopBase->db_DefaultWindow;
                }
                else
                {
                    windowClass = MUIC_Window;
                }
                
                windowObject = NewObject
                (
                    windowClass, NULL, 
                
                    MUIA_Window_UseBottomBorderScroller, TRUE,
                    MUIA_Window_UseRightBorderScroller,  TRUE,
                    
                    WindowContents, CreateDesktopObjectA
                    (
                        CDO_IconContainer, tags
                    ),
                
                    TAG_DONE
                );
            }
            break;
    }

    return newObject;

    AROS_LIBFUNC_EXIT
} /* CreateWorkbenchObjectA */
