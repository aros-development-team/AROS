/*
    Copyright  2003-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

// #define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/locale.h>
#include <prefs/prefhdr.h>
//#define DEBUG 1
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
#include "registertab.h"
#include "page_language.h"
#include "page_country.h"
#include "page_timezone.h"

/*** Instance Data **********************************************************/

struct LocaleRegister_DATA
{
  int i;

  Object *child;
  Object *language;
  Object *country;
  Object *timezone;

  char *LocaleRegisterLabels[3];
  char *tab_label;
};

STATIC VOID LocalePrefs2Gadgets(struct LocaleRegister_DATA *data);
STATIC VOID Gadgets2LocalePrefs(struct LocaleRegister_DATA *data);
       VOID ShowMsg(char *msg);


/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct LocaleRegister_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/


static Object *handle_New_error(Object *obj, struct IClass *cl, char *error)
{
    struct LocaleRegister_DATA *data;

    ShowMsg(error);
    D(bug("[Register class] %s\n",error));

    if(!obj)
	return NULL;

    data = INST_DATA(cl, obj);

    if(data->language)
    {
	D(bug("[Register class] DisposeObject(data->language);\n"));
	DisposeObject(data->language);
	data->language=NULL;
    }

    if(data->country)
    {
	D(bug("[Register class] DisposeObject(data->country);\n"));
	DisposeObject(data->country);
	data->country=NULL;
    }

    if(data->timezone)
    {
	D(bug("[Register class] DisposeObject(data->timezone);\n"));
	DisposeObject(data->timezone);
	data->timezone=NULL;
    }

    if(data->tab_label)
    {
	FreeVec(data->tab_label);
	data->tab_label=NULL;
    }
 
    CoerceMethod(cl, obj, OM_DISPOSE);
    return NULL;
}

Object *LocaleRegister__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    D(bug("[register class] LocaleRegister Class New\n"));

    /* 
     * we create self first and then create the child,
     * so we have self->data available already
     */

    self = (Object *) DoSuperNewTags(
			  CLASS, self, NULL,
	     		  MUIA_PrefsEditor_Name, MSG(MSG_WINTITLE),
 			  MUIA_PrefsEditor_Path, (IPTR) "SYS/locale.prefs",
			  TAG_DONE
		      );
    if (self == NULL) 
    {
	return handle_New_error(self, CLASS, "ERROR: Unable to create register object!\n");
    }

    SETUP_INST_DATA;

    data->language=NewObject(Language_CLASS->mcc_Class,NULL,
			      MA_PrefsObject, (ULONG) self,
			      TAG_DONE);

    if(!data->language)
	return handle_New_error(self,CLASS,"ERROR: Unable to create language object!\n");

    data->country= ListviewObject, MUIA_Listview_List,
			NewObject(Country_CLASS->mcc_Class,0,
				  MA_PrefsObject, (ULONG) self,
			  	  TAG_DONE),
		   End;

    if(!data->country)
	return handle_New_error(self,CLASS,"ERROR: Unable to create country object!\n");

    data->timezone=NewObject(Timezone_CLASS->mcc_Class,NULL,
                             MA_PrefsObject, (ULONG) self,
			     TAG_DONE);

    if(!data->timezone)
    {
      D(bug("icall register handle error\n"));
	return handle_New_error(self,CLASS,"ERROR: Unable to create timezone object!\n");
    }


    /*
     * Maybe, it would be easier to change the catalog,
     * but for me it's easier this way ;)
     */
    data->tab_label=AllocVec( strlen(MSG(MSG_GAD_TAB_LANGUAGE)) +
                              strlen(MSG(MSG_GAD_TAB_COUNTRY)) +
			      strlen(" / ") + 1,
			      MEMF_ANY);

    if(!data->tab_label)
	return handle_New_error(self,CLASS,"ERROR: Unable to allocate tab_label!\n");

    sprintf(data->tab_label,"%s / %s",MSG(MSG_GAD_TAB_COUNTRY),
                                      MSG(MSG_GAD_TAB_LANGUAGE));

    data->LocaleRegisterLabels[0]=data->tab_label;
    data->LocaleRegisterLabels[1]=MSG(MSG_GAD_TAB_TIMEZONE);
    data->LocaleRegisterLabels[2]=NULL;

    data->child=
    RegisterGroup(data->LocaleRegisterLabels),
    MUIA_Register_Frame, TRUE,
    Child,
	HGroup,
	MUIA_Group_SameSize, TRUE, 
	Child,
	    HGroup,
	    MUIA_Frame, MUIV_Frame_Group,
	    MUIA_FrameTitle, MSG(MSG_GAD_TAB_COUNTRY),
	    Child,
  		data->country,
	    End,
	Child,
	    HGroup,
	    MUIA_Frame, MUIV_Frame_Group,
	    MUIA_FrameTitle, MSG(MSG_GAD_TAB_LANGUAGE),
	    Child,
  		data->language,
	    End,
	End,
    Child,
	data->timezone,
    End;

    if(!data->child)
	return handle_New_error(self, CLASS, "ERROR: unable to create registergroup\n");

    DoMethod(self,OM_ADDMEMBER,(ULONG) data->child);

    DoMethod(data->country,COUNTRY_FILL);

    LocalePrefs2Gadgets(data);
  
    return self;
}

/*
 * update struct localprefs with actual data selected in gadgets:
 *
 * see prefs/locale.h
 *
 * struct LocalePrefs {
 *     char  lp_CountryName[32];
 *     char  lp_PreferredLanguages[10][30];
 *     LONG  lp_GMTOffset;
 *     ULONG lp_Flags;
 *
 *     struct CountryPrefs lp_CountryData;
 * };
 *
 */
STATIC VOID Gadgets2LocalePrefs (struct LocaleRegister_DATA *data)
{
    char *tmp;
    char **preferred;
    ULONG i;

    if(get(data->country,MA_CountryName,&tmp))
    {
	strncpy(localeprefs.lp_CountryName,tmp,32);
    }

    if(get(data->language,MA_Preferred,&preferred))
    {
	for(i=0;i<10;i++)
	{
	    if(preferred[i])
	    {
		strncpy(localeprefs.lp_PreferredLanguages[i],preferred[i],30);
	    }
	    else
	    {
		localeprefs.lp_PreferredLanguages[i][0]=(char) 0;
	    }
	}
    }

    get(data->timezone, MA_TimeOffset, &localeprefs.lp_GMTOffset);
}

/*
 * update gadgets with values of struct localeprefs 
 */
STATIC VOID LocalePrefs2Gadgets(struct LocaleRegister_DATA *data)
{

    set(data->country, MA_CountryName, localeprefs.lp_CountryName);

    set(data->language, MA_Preferred, TRUE);

    set(data->timezone, MA_TimeOffset, -localeprefs.lp_GMTOffset);

}

IPTR LocaleRegister__MUIM_PrefsEditor_ImportFH (
    Class *CLASS, Object *self, 
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;

    if (!LoadPrefsFH(message->fh)) {
	D(bug("[register class] LocaleRegister__MUIM_PrefsEditor_ImportFH failed\n"));
	return FALSE;
    }

    BackupPrefs();
    LocalePrefs2Gadgets(data);
    SET(self, MUIA_PrefsEditor_Changed, FALSE);
    SET(self, MUIA_PrefsEditor_Testing, FALSE);

    return TRUE;
}

IPTR LocaleRegister__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    D(bug("[register class] SerEdit Class Export\n"));

    Gadgets2LocalePrefs(data);
    return SavePrefsFH(message->fh);
}

IPTR LocaleRegister__MUIM_PrefsEditor_Test
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL result;

    D(bug("[register class] SerEdit Class Test\n"));

    Gadgets2LocalePrefs(data);

    result=SaveEnv();

    if(result) { /* TRUE -> success */
	SET(self, MUIA_PrefsEditor_Changed, FALSE);
	SET(self, MUIA_PrefsEditor_Testing, TRUE);
    }

    return result; 
}

IPTR LocaleRegister__MUIM_PrefsEditor_Revert
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL result;

    D(bug("[register class] SerEdit Class Revert\n"));

    RestorePrefs();
    LocalePrefs2Gadgets(data);

    result=SaveEnv();

    if(result) {
	SET(self, MUIA_PrefsEditor_Changed, FALSE);
	SET(self, MUIA_PrefsEditor_Testing, FALSE);
    }

    return result;
}

IPTR LocaleRegister__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    if(data->tab_label)
    {
	FreeVec(data->tab_label);
	data->tab_label=NULL;
    }
	 
    return DoSuperMethodA(CLASS, self, message);
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_6
(
    LocaleRegister, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                    struct opSet *,
    OM_DISPOSE,                Msg,
    MUIM_PrefsEditor_ImportFH, struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH, struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_Test,     Msg,
    MUIM_PrefsEditor_Revert,   Msg
);

