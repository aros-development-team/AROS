#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>

#include <zune/customclasses.h>
#include <zune/prefseditor.h>
#include <zune/systemprefswindow.h>

#include <prefs/screenmode.h>
#include <prefs/prefhdr.h>

#include <stdio.h>

#include "smeditor.h"
#include "smselector.h"
#include "smproperties.h"


struct SMEditor_DATA
{
    Object *selector, *properties;
};

#define SMEditorObject BOOPSIOBJMACRO_START(SMEditor_CLASS->mcc_Class)
#define SETUP_INST_DATA struct SMEditor_DATA *data = INST_DATA(CLASS, self)

Object *SMEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *selector, *properties;
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_PrefsEditor_Name, (IPTR)"Screen Mode Preferences",
        MUIA_PrefsEditor_Path, (IPTR)"SYS/screenmode.prefs",
        
	Child, (IPTR)VGroup,
	    Child, (IPTR)(selector   = ScreenModeSelectorObject, End),
	    Child, (IPTR)(properties = ScreenModePropertiesObject, GroupFrame, End),
	End,
        
        TAG_DONE
    );
    
    if (self)
    {
        SETUP_INST_DATA;
        
	data->selector   = selector;
        data->properties = properties;
	     
        /*-- Setup notifications -------------------------------------------*/
        DoMethod
	(
	    selector, MUIM_Notify, MUIA_ScreenModeSelector_Active, MUIV_EveryTime,
	    (IPTR)properties, 3,
	    MUIM_Set, MUIA_ScreenModeProperties_DisplayID, MUIV_TriggerValue
	);
		
        DoMethod
	(
	    properties, MUIM_Notify, MUIA_ScreenModeProperties_DisplayID, MUIV_EveryTime,
	    (IPTR)self, 3,
	    MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
	);
    
        DoMethod
	(
	    properties, MUIM_Notify, MUIA_ScreenModeProperties_Width, MUIV_EveryTime,
	    (IPTR)self, 3,
	    MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
	);
        
	DoMethod
	(
	    properties, MUIM_Notify, MUIA_ScreenModeProperties_Height, MUIV_EveryTime,
	    (IPTR)self, 3,
	    MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
	);
        
	DoMethod
	(
	    properties, MUIM_Notify, MUIA_ScreenModeProperties_Depth, MUIV_EveryTime,
	    (IPTR)self, 3,
	    MUIM_Set, MUIA_PrefsEditor_Changed, TRUE
	);
    }
    
    return self;
}

void SMPByteSwap(struct ScreenModePrefs *smp)
{
#if 0
    #undef  SWAP
    #define SWAP(type, field) smp->smp_ ## field = AROS_BE2 ## type(smp->smp_ ## field)
    
    int i;
    for (i = 0; i < sizeof(smp->smp_Reserved)/sizeof(ULONG); i++)
        SWAP(LONG, Reserved[i]);
	
        SWAP(LONG, DisplayID);
	SWAP(WORD, Width);
	SWAP(WORD, Height);
	SWAP(WORD, Depth);
	SWAP(WORD, Control);
#endif
}

IPTR SMEditor__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self, 
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    struct ContextNode     *context;
    struct IFFHandle       *handle;
    struct ScreenModePrefs  smp;
    BOOL                    success = TRUE;
    LONG                    error;
    
    if (!(handle = AllocIFF()))
        return FALSE;
    
    handle->iff_Stream = (IPTR) message->fh;
    InitIFFasDOS(handle);

    if ((error = OpenIFF(handle, IFFF_READ)) == 0)
    {
	
        BYTE i;
        
        // FIXME: We want some sanity checking here!
        for (i = 0; i < 1; i++)
        {
            if ((error = StopChunk(handle, ID_PREF, ID_SCRM)) == 0)
            {
                if ((error = ParseIFF(handle, IFFPARSE_SCAN)) == 0)
                {
                    context = CurrentChunk(handle);
                    
                    error = ReadChunkBytes
                    (
                        handle, &smp, sizeof(struct ScreenModePrefs)
                    );
                    
                    if (error < 0)
                    {
                        printf("Error: ReadChunkBytes() returned %ld!\n", error);
                    }                    
                }
                else
                {
                    printf("ParseIFF() failed, returncode %ld!\n", error);
                    success = FALSE;
                    break;
                }
            }
            else
            {
                printf("StopChunk() failed, returncode %ld!\n", error);
                success = FALSE;
            }
        }

        CloseIFF(handle);
    }
    else
    {
        //ShowError(_(MSG_CANT_OPEN_STREAM));
    }

    FreeIFF(handle);
    
    
    if (success)
    {
        SMPByteSwap(&smp);
	
	nnset(data->selector, MUIA_ScreenModeSelector_Active, smp.smp_DisplayID);
	SetAttrs
	(
	    data->properties,
	    MUIA_NoNotify,                       TRUE,
	    MUIA_ScreenModeProperties_DisplayID, smp.smp_DisplayID,
	    MUIA_ScreenModeProperties_Width,     smp.smp_Width,
	    MUIA_ScreenModeProperties_Height,    smp.smp_Height,
	    MUIA_ScreenModeProperties_Depth,     smp.smp_Depth,
	    TAG_DONE
        );
    }
    
    return success;
}

IPTR SMEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    
    struct PrefHeader header = { 0 }; 
    struct IFFHandle *handle;
    BOOL              success = TRUE;
    LONG              error   = 0;
        
    if ((handle = AllocIFF()))
    {
        handle->iff_Stream = (IPTR) message->fh;
        
        InitIFFasDOS(handle);
        
        if (!(error = OpenIFF(handle, IFFF_WRITE))) /* NULL = successful! */
        {
            struct ScreenModePrefs smp;
	    
	    memset(&smp, 0, sizeof(smp));
            
	    BYTE i;
            
            PushChunk(handle, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */
            
            header.ph_Version = PHV_CURRENT;
            header.ph_Type    = 0;
            
            PushChunk(handle, ID_PREF, ID_PRHD, IFFSIZE_UNKNOWN); /* FIXME: IFFSIZE_UNKNOWN? */
            
            WriteChunkBytes(handle, &header, sizeof(struct PrefHeader));
            
            PopChunk(handle);
            
            for (i = 0; i < 1; i++)
            {
                error = PushChunk(handle, ID_PREF, ID_SCRM, sizeof(struct ScreenModePrefs));
                
                if (error != 0) // TODO: We need some error checking here!
                {
                    printf("error: PushChunk() = %ld ", error);
                }
                
	        smp.smp_DisplayID = XGET(data->properties, MUIA_ScreenModeProperties_DisplayID);
                smp.smp_Width     = XGET(data->properties, MUIA_ScreenModeProperties_Width);
                smp.smp_Height    = XGET(data->properties, MUIA_ScreenModeProperties_Height);
                smp.smp_Depth     = XGET(data->properties, MUIA_ScreenModeProperties_Depth);
                smp.smp_Control   = 0;
                
		SMPByteSwap(&smp);
			
                error = WriteChunkBytes(handle, &smp, sizeof(struct ScreenModePrefs));
                error = PopChunk(handle);
                                
                if (error != 0) // TODO: We need some error checking here!
                {
                    printf("error: PopChunk() = %ld ", error);
                }
            }

            // Terminate the FORM
            PopChunk(handle);
        }
        else
        {
            //ShowError(_(MSG_CANT_OPEN_STREAM));
            success = FALSE;
        }
        
        CloseIFF(handle);
	FreeIFF(handle);
    }
    else // AllocIFF()
    {
        // Do something more here - if IFF allocation has failed, something isn't right
        //ShowError(_(MSG_CANT_ALLOCATE_IFFPTR));
        success = FALSE;
    }

    return success;
}

IPTR SMEditor__MUIM_PrefsEditor_Test
(
    Class *CLASS, Object *self, Msg message
)
{
   return FALSE;
}

IPTR SMEditor__MUIM_PrefsEditor_Use
(
    Class *CLASS, Object *self, Msg message
)
{
    #warning "FIXME: Closing the window here only works because we're lucky   "
    #warning "       and nothing needs to access anything that is put in the  "
    #warning "       RenderInfo structure, which gets deallocated when closing"
    #warning "       the window. This needs to be fixed directly in the       "
    #warning "       PrefsEditor class.                                       "
    if (muiRenderInfo(self) && _win(self))
        set(_win(self), MUIA_Window_Open, FALSE);
	    
    return DoSuperMethodA(CLASS, self, message);
}

#undef ALIAS
#define ALIAS(old, new) \
    AROS_MAKE_ALIAS(SMEditor__MUIM_PrefsEditor_ ## old, SMEditor__MUIM_PrefsEditor_ ## new)

ALIAS(Test, Revert);
ALIAS(Use, Save);
ALIAS(Use, Cancel);

ZUNE_CUSTOMCLASS_8
(
    SMEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                    struct opSet *,
    MUIM_PrefsEditor_ImportFH, struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH, struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_Test,     Msg,
    MUIM_PrefsEditor_Revert,   Msg,
    MUIM_PrefsEditor_Use,      Msg,
    MUIM_PrefsEditor_Save,     Msg,
    MUIM_PrefsEditor_Cancel,   Msg
);
