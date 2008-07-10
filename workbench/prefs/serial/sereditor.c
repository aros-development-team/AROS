/*
    Copyright  2003-2008, The AROS Development Team. All rights reserved.
    $Id: ipeditor.c 21816 2007-09-25 12:35:29Z chodorowski, dariusb $
*/

// #define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/serial.h>
#include <prefs/prefhdr.h>
/* #define DEBUG 1 */
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <aros/debug.h>

#include "global.h"
#include "locale.h"
#include "sereditor.h"

/*** String Data ************************************************************/

CONST_STRPTR BaudrateLabels[] =
{
	(CONST_STRPTR)    "50",
	(CONST_STRPTR)    "75",
	(CONST_STRPTR)   "110",
	(CONST_STRPTR)   "134",
	(CONST_STRPTR)   "150",
	(CONST_STRPTR)   "200",
	(CONST_STRPTR)   "300",
	(CONST_STRPTR)   "600",
	(CONST_STRPTR)  "1200",
	(CONST_STRPTR)  "2400",
	(CONST_STRPTR)  "4800",
	(CONST_STRPTR)  "9600",
	(CONST_STRPTR) "19200",
	(CONST_STRPTR) "38400",
	(CONST_STRPTR) "57600",
	(CONST_STRPTR)"115200",
	(CONST_STRPTR)   NULL
};

CONST_STRPTR StopBitsLabels[] =
{
	(CONST_STRPTR) "1",
	(CONST_STRPTR) "1.5",
	(CONST_STRPTR) "2",
	NULL
};

CONST_STRPTR DataBitsLabels[] =
{
	(CONST_STRPTR) "8",
	(CONST_STRPTR) "7",
	(CONST_STRPTR) "6",
	(CONST_STRPTR) "5",
	NULL
};

CONST_STRPTR BufferSizeLabels[] =
{
	(CONST_STRPTR)  "512",
	(CONST_STRPTR) "1024",
	(CONST_STRPTR) "2048",
	(CONST_STRPTR) "4096",
	NULL
};

/*** Instance Data **********************************************************/

struct SerEditor_DATA
{
  int i;

  CONST_STRPTR ParityLabels[5];
  Object *child;
  Object *baudrate;
  Object *stopbits;
  Object *databits;
  Object *parity;
  Object *inputbuffersize;
  Object *outputbuffersize;

};

struct SerialPrefs          serialprefs;

STATIC VOID SerPrefs2Gadgets(struct SerEditor_DATA *data);
STATIC VOID Gadgets2SerPrefs(struct SerEditor_DATA *data);
       VOID ShowMsg(char *msg);


/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct SerEditor_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *SerEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
#if SHOWICON
    Object *icon;
#endif

    D(bug("[seredit class] SerEdit Class New\n"));

    /* 
     * we create self first and then create the child,
     * so we have self->data available already
     */

    self = (Object *) DoSuperNewTags(
			  CLASS, self, NULL,
	     		  MUIA_PrefsEditor_Name, MSG(MSG_WINTITLE),
 			  MUIA_PrefsEditor_Path, (IPTR) "SYS/Serial.prefs",
			  TAG_DONE
		      );
    
    if (self == NULL) 
    {
	printf("UPS..TODO\n");
	return NULL;
    }

    SETUP_INST_DATA;

    data->ParityLabels[0] = MSG(MSG_PARITY_NONE);
    data->ParityLabels[1] = MSG(MSG_PARITY_EVEN);
    data->ParityLabels[2] = MSG(MSG_PARITY_ODD);
    data->ParityLabels[3] = MSG(MSG_PARITY_MARK);
    data->ParityLabels[4] = MSG(MSG_PARITY_SPACE);
    data->ParityLabels[5] = NULL;

#if SHOWPIC
    icon=MUI_NewObject("Dtpic.mui",MUIA_Dtpic_Name,"PROGDIR:Serial.info",TAG_DONE);

    if(!icon) icon=HVSpace;
#endif

    data->child=
#if SHOWPIC
    HGroup,
      Child,
	icon,
      Child,
#endif

    VGroup,
      Child, 
	ColGroup(2),
	  Child, Label1("Baudrate"),
	  Child, data->baudrate = CycleObject,
			    MUIA_Cycle_Entries, BaudrateLabels, 
			  End,
	  Child, Label1("Data Bits"),
	  Child, data->databits = CycleObject,
			    MUIA_Cycle_Entries, DataBitsLabels, 
			  End,
	  Child, Label1("Parity"),
	  Child, data->parity = CycleObject,
			    MUIA_Cycle_Entries, data->ParityLabels, 
			  End,
	  Child, Label1("Stop Bits"),
	  Child, data->stopbits = CycleObject,
			    MUIA_Cycle_Entries, StopBitsLabels, 
			  End,
	  Child, Label1("Input Buffer Size"),
	  Child, data->inputbuffersize = CycleObject,
			    MUIA_Cycle_Entries, BufferSizeLabels, 
			  End,
	  Child, Label1("Output Buffer Size"),
	  Child, data->outputbuffersize = CycleObject,
			    MUIA_Cycle_Entries, BufferSizeLabels, 
			  End,

	End, /* ColGroup */
#if SHOWPIC
      End, 
#endif
    End; 

    DoMethod(self,OM_ADDMEMBER,(ULONG) data->child);

    DoMethod(data->baudrate, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
    DoMethod(data->stopbits, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
    DoMethod(data->databits, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
    DoMethod(data->parity, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
    DoMethod(data->inputbuffersize, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
    DoMethod(data->outputbuffersize, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR) self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);

    SerPrefs2Gadgets(data);
  
    return self;
}

/*
 * update struct serialprefs with actual data selected in gadgets
 */
STATIC VOID Gadgets2SerPrefs (struct SerEditor_DATA *data)
{
    D(bug("Gadgets2SerPrefs\n"));

    serialprefs.sp_BaudRate =    atol((char *) 
                                   BaudrateLabels[
				     XGET(data->baudrate,
				          MUIA_Cycle_Active)
				   ]);

    serialprefs.sp_BitsPerChar = XGET(data->databits, MUIA_Cycle_Active);

    serialprefs.sp_Parity =      XGET(data->parity,MUIA_Cycle_Active);

    serialprefs.sp_StopBits =    XGET(data->stopbits,MUIA_Cycle_Active);

    serialprefs.sp_InputBuffer = atol((char *) 
                                   BufferSizeLabels[
				     XGET(data->inputbuffersize,
				          MUIA_Cycle_Active)
				   ]);
    serialprefs.sp_OutputBuffer = atol((char *) 
                                   BufferSizeLabels[
				     XGET(data->outputbuffersize,
				          MUIA_Cycle_Active)
				   ]);

    D(bug("Gadgets2SerPrefs left\n"));
}

/*
 * helper for SerPrefs2Gadgets
 */
STATIC VOID RefreshGadget(Object *obj, ULONG value, CONST_STRPTR *labels)
{
    ULONG i = 0;

    while (NULL != labels[i]) {
    	if (atol((char *) labels[i]) == value) {
    	    set(obj, MUIA_Cycle_Active, i);
    	    return;
    	}
    	i++;
    }
    set(obj, MUIA_Cycle_Active, 0);
}

/*
 * update gadgets with values of struct serialprefs 
 */
STATIC VOID SerPrefs2Gadgets(struct SerEditor_DATA *data)
{
    RefreshGadget(data->baudrate,
                  serialprefs.sp_BaudRate,
		  BaudrateLabels);

    set(data->databits, MUIA_Cycle_Active, serialprefs.sp_BitsPerChar);

    set(data->parity, MUIA_Cycle_Active, serialprefs.sp_Parity);

    set(data->stopbits, MUIA_Cycle_Active, serialprefs.sp_StopBits);

    RefreshGadget(data->inputbuffersize,
                  serialprefs.sp_InputBuffer,
		  BufferSizeLabels);

    RefreshGadget(data->outputbuffersize,
                  serialprefs.sp_OutputBuffer,
		  BufferSizeLabels);
}

IPTR SerEditor__MUIM_PrefsEditor_ImportFH (
    Class *CLASS, Object *self, 
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;

    D(bug("[seredit class] SerEdit Class Import\n"));

    if (!LoadPrefsFH(message->fh)) {
	D(bug("[seredit class] SerEditor__MUIM_PrefsEditor_ImportFH failed\n"));
	return FALSE;
    }

    BackupPrefs();
    SerPrefs2Gadgets(data);
    SET(self, MUIA_PrefsEditor_Changed, FALSE);
    SET(self, MUIA_PrefsEditor_Testing, FALSE);

    return TRUE;
}

IPTR SerEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    D(bug("[seredit class] SerEdit Class Export\n"));

    Gadgets2SerPrefs(data);
    return SavePrefsFH(message->fh);
}

IPTR SerEditor__MUIM_PrefsEditor_Test
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL result;

    D(bug("[seredit class] SerEdit Class Test\n"));

    Gadgets2SerPrefs(data);

    result=SaveEnv();

    if(result) { /* TRUE -> success */
	SET(self, MUIA_PrefsEditor_Changed, FALSE);
	SET(self, MUIA_PrefsEditor_Testing, TRUE);
    }

    return result; 
}


IPTR SerEditor__MUIM_PrefsEditor_Revert
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL result;

    D(bug("[seredit class] SerEdit Class Revert\n"));

    RestorePrefs();
    SerPrefs2Gadgets(data);

    result=SaveEnv();

    if(result) {
	SET(self, MUIA_PrefsEditor_Changed, FALSE);
	SET(self, MUIA_PrefsEditor_Testing, FALSE);
    }

    return result;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_5
(
    SerEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                    struct opSet *,
    MUIM_PrefsEditor_ImportFH, struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH, struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_Test,     Msg,
    MUIM_PrefsEditor_Revert,   Msg
);

