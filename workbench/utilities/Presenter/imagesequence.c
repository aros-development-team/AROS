/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <libraries/mui.h>

#include <string.h>
#include <stdio.h>

#include "imagesequence.h"

/*** Instance data **********************************************************/

#define ISD_BUFFERSIZE  128

struct ImageSequence_DATA
{
    ULONG                        isd_CurrentImage;
    struct MUI_EventHandlerNode  isd_EHN;
    char                         isd_Buffer[ISD_BUFFERSIZE];
};

/*** Methods ****************************************************************/

static IPTR ImageSequence$OM_NEW
(
    struct IClass *CLASS, Object *self, struct opSet *message 
)
{
    struct ImageSequence_DATA *data;
    // struct TagItem            *tag, *tags;
        
    self = (Object *) DoSuperNewTags
    ( 
        CLASS, self, NULL,
        
        InnerSpacing( 0, 0 ),
        MUIA_Image_Spec, (IPTR) "3:1",
        
        TAG_MORE,        (IPTR) message->ops_AttrList
    );

    if( !self ) return FALSE;

    data = INST_DATA( CLASS, self );

    data->isd_CurrentImage = 1;
    data->isd_Buffer[0]    = '\0';
    
    /* FIXME: segfaults
    snprintf( data->isd_Buffer, ISD_BUFFERSIZE, "Presenter [%ld]", data->isd_CurrentImage );
    SetAttrs( _win( self ), MUIA_Window_Title, data->isd_Buffer, TAG_DONE );
    */ 
    
    return (IPTR) self;
}


/*
static IPTR ImageSequence$OM_SET
(
    struct IClass *CLASS, Object *self, struct opSet *message 
)
{
    return NULL;
}

static IPTR ImageSequence$OM_GET
(
    struct IClass *CLASS, Object *self, struct opGet *message 
)
{
    return NULL;
}
*/

static IPTR ImageSequence$MUIM_Setup
(
    struct IClass *CLASS, Object *self, struct MUIP_Setup *message 
)
{
    struct ImageSequence_DATA *data;

    if( !(DoSuperMethodA( CLASS, self, (Msg) message )) ) return 0;

    data = INST_DATA( CLASS, self );

    data->isd_EHN.ehn_Events   = IDCMP_RAWKEY;
    data->isd_EHN.ehn_Priority = 0;
    data->isd_EHN.ehn_Flags    = 0;
    data->isd_EHN.ehn_Object   = self;
    data->isd_EHN.ehn_Class    = CLASS;

    DoMethod
    ( 
        _win( self ), MUIM_Window_AddEventHandler, (IPTR) &data->isd_EHN 
    );
    
    return 1;
}

static IPTR ImageSequence$MUIM_Cleanup
(
    struct IClass *CLASS, Object *self, struct MUIP_Cleanup *message
)
{
    struct ImageSequence_DATA *data = INST_DATA( CLASS, self );

    DoMethod
    ( 
        _win( self ), MUIM_Window_RemEventHandler, (IPTR) &data->isd_EHN 
    );

    return DoSuperMethodA( CLASS, self, (Msg) message );
}

static ULONG ImageSequence$MUIM_HandleEvent
(
    struct IClass *CLASS, Object *self, struct MUIP_HandleEvent *message
)
{
    struct ImageSequence_DATA *data   = INST_DATA( CLASS, self );
    BOOL                       update = FALSE;
    
    if( message->imsg )
    {
        switch( message->imsg->Class )
        {
            case IDCMP_RAWKEY:
                switch( message->imsg->Code )
                {
                    case CURSORUP:
                        /* Previous image */
                        data->isd_CurrentImage--;
                        update = TRUE;
                        break;
                        
                    case CURSORDOWN:
                        /* Next image */
                        data->isd_CurrentImage++;
                        update = TRUE;
                        break;
                }
                break;
        }
    }

    if( update )
    {
        snprintf( data->isd_Buffer, ISD_BUFFERSIZE, "3:%ld", data->isd_CurrentImage );
        SET(self, MUIA_Image_Spec, (IPTR) data->isd_Buffer);
        snprintf( data->isd_Buffer, ISD_BUFFERSIZE, "Presenter [%ld]", data->isd_CurrentImage );
        SET(_win( self ), MUIA_Window_Title, (IPTR) data->isd_Buffer);
    }
    
    return MUI_EventHandlerRC_Eat;
}

/*** Dispatcher *************************************************************/

BOOPSI_DISPATCHER( IPTR, ImageSequence_Dispatcher, CLASS, self, message )
{
    switch( message->MethodID )
    {
        case OM_NEW:           return ImageSequence$OM_NEW( CLASS, self, (struct opSet *) message );
        //case OM_GET:           return ImageSequence$OM_GET( CLASS, self, (struct opGet *) message );
        //case OM_SET:           return ImageSequence$OM_SET( CLASS, self, (struct opSet *) message );
        case MUIM_Setup:       return ImageSequence$MUIM_Setup( CLASS, self, (APTR) message );
        case MUIM_Cleanup:     return ImageSequence$MUIM_Cleanup( CLASS, self, (APTR) message );
        case MUIM_HandleEvent: return ImageSequence$MUIM_HandleEvent( CLASS, self, (APTR) message );
        default:               return DoSuperMethodA( CLASS, self, message );
    }
    
    return NULL;
}

/*** Setup ******************************************************************/

struct MUI_CustomClass *ImageSequence_CLASS = NULL;

void ImageSequence_Create()
{
    ImageSequence_CLASS = MUI_CreateCustomClass
    (
        NULL, "Image.mui", NULL, 
        sizeof( struct ImageSequence_DATA ), ImageSequence_Dispatcher
    );
}

void ImageSequence_Destroy()
{
    MUI_DeleteCustomClass( ImageSequence_CLASS );
}
