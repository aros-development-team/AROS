/*
   Copyright © 2003-2019, The AROS Development Team. All rights reserved.
   $Id$
 */

#include <aros/debug.h>

#include <proto/alib.h>
#include <proto/battclock.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <devices/timer.h>
#include <libraries/locale.h>
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <stdio.h>

#include "locale.h"
#include "registertab.h"
#include "misc.h"
#include "prefs.h"
#include "page_language.h"
#include "page_region.h"
#include "page_timezone.h"

/*** Instance Data **********************************************************/

struct LocaleRegister_DATA
{
    int i;

    Object *child;
    Object *language;
    Object *region;
    Object *timezone;

    const char *LocaleRegisterLabels[3];
    char *tab_label;

    BOOL save;
};

STATIC VOID LocalePrefs2Gadgets(struct LocaleRegister_DATA *data);
STATIC VOID Gadgets2LocalePrefs(struct LocaleRegister_DATA *data);

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct LocaleRegister_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/


static Object *handle_New_error(Object *obj, struct IClass *cl, char *error)
{
    struct LocaleRegister_DATA *data;

    ShowMessage(error);
    D(bug("[Register class] %s\n", error));

    if(!obj)
        return NULL;

    data = INST_DATA(cl, obj);

    if(data->language)
    {
        D(bug("[Register class] DisposeObject(data->language);\n"));
        DisposeObject(data->language);
        data->language = NULL;
    }

    if(data->region)
    {
        D(bug("[Register class] DisposeObject(data->region);\n"));
        DisposeObject(data->region);
        data->region = NULL;
    }

    if(data->timezone)
    {
        D(bug("[Register class] DisposeObject(data->timezone);\n"));
        DisposeObject(data->timezone);
        data->timezone = NULL;
    }

    if(data->tab_label)
    {
        FreeVec(data->tab_label);
        data->tab_label = NULL;
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

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, _(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/locale.prefs",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Locale",
        TAG_DONE
    );
    if (self == NULL)
    {
        return handle_New_error(self, CLASS, "ERROR: Unable to create register object!\n");
    }

    SETUP_INST_DATA;

    data->language = NewObject(Language_CLASS->mcc_Class, NULL,
            MUIA_UserData, self,
            TAG_DONE);

    if(!data->language)
        return handle_New_error(self, CLASS, "ERROR: Unable to create language object!\n");

    data->region = ListviewObject, MUIA_Listview_List,
        NewObject(Region_CLASS->mcc_Class, 0,
                MUIA_UserData, self,
                TAG_DONE),
        End;

    if(!data->region)
        return handle_New_error(self, CLASS, "ERROR: Unable to create region object!\n");

    data->timezone = NewObject(Timezone_CLASS->mcc_Class, NULL,
            MUIA_UserData, self,
            TAG_DONE);

    if(!data->timezone)
    {
        D(bug("icall register handle error\n"));
        return handle_New_error(self, CLASS,"ERROR: Unable to create timezone object!\n");
    }


    /*
     * Maybe, it would be easier to change the catalog,
     * but for me it's easier this way ;)
     */
    data->tab_label = AllocVec( strlen(_(MSG_GAD_TAB_LANGUAGE)) +
            strlen(_(MSG_GAD_TAB_REGION)) +
            strlen(" / ") + 1,
            MEMF_ANY);

    if(!data->tab_label)
        return handle_New_error(self, CLASS, "ERROR: Unable to allocate tab_label!\n");

    sprintf(data->tab_label, "%s / %s", _(MSG_GAD_TAB_REGION),
            _(MSG_GAD_TAB_LANGUAGE));

    data->LocaleRegisterLabels[0] = data->tab_label;
    data->LocaleRegisterLabels[1] = _(MSG_GAD_TAB_TIMEZONE);
    data->LocaleRegisterLabels[2] = NULL;

    data->child = RegisterGroup(data->LocaleRegisterLabels),
        MUIA_Register_Frame, TRUE,
        Child, HGroup,
            MUIA_Group_SameSize, TRUE,
            Child, HGroup,
                MUIA_Frame, MUIV_Frame_Group,
                MUIA_FrameTitle, _(MSG_GAD_TAB_REGION),
                Child, data->region,
            End,
            Child, HGroup,
                MUIA_Frame, MUIV_Frame_Group,
                MUIA_FrameTitle, _(MSG_GAD_TAB_LANGUAGE),
                Child, data->language,
            End,
        End,
        Child, data->timezone,
    End;

    if(!data->child)
        return handle_New_error(self, CLASS, "ERROR: unable to create registergroup\n");

    DoMethod(self, OM_ADDMEMBER, data->child);

    DoMethod(data->region, MUIM_Region_Fill);

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
    IPTR newtz;
    IPTR gmtclock;
    char **preferred = NULL;
    ULONG i;
    ULONG newflags;
    BOOL sync_clock = FALSE;

    if (GetAttr(MUIA_Region_Regionname, data->region, (IPTR *)&tmp))
    {
        strncpy(localeprefs.lp_CountryName, tmp, sizeof(localeprefs.lp_CountryName) - 1);
        Prefs_LoadRegion(localeprefs.lp_CountryName, &localeprefs.lp_CountryData);
    }

    if(GET(data->language, MUIA_Language_Preferred, &preferred))
    {
        for(i = 0; i < 10; i++)
        {
            if(preferred[i])
            {
                strncpy(localeprefs.lp_PreferredLanguages[i], preferred[i], 30);
            }
            else
            {
                localeprefs.lp_PreferredLanguages[i][0] = 0;
            }
        }
    }
    GetAttr(MUIA_Language_Characterset, data->language, (IPTR *)&tmp);
    if (tmp)
        strcpy(character_set, tmp);
    else
        character_set[0] = 0;
    D(bug("[locale prefs] New character set is %s\n", character_set));

    GetAttr(MUIA_Timezone_Timeoffset, data->timezone, &newtz);

    newflags = localeprefs.lp_Flags;
    GetAttr(MUIA_Timezone_GMTClock, data->timezone, &gmtclock);
    if (gmtclock)
    	newflags |= LOCF_GMT_CLOCK;
    else
    	newflags &= ~LOCF_GMT_CLOCK;

    /*
     * If we change "GMT clock" setting, update system time from hardware clock.
     * We do it in order to prevent time de-synchronization and clobbering
     * hardware clock.
     */
    if ((newflags ^ localeprefs.lp_Flags) & LOCF_GMT_CLOCK)
    	sync_clock = TRUE;

    /* Do the same if 'GMT clock' is active and we change the time zone */
    if (gmtclock && (newtz != localeprefs.lp_GMTOffset))
    	sync_clock = TRUE;

    if (sync_clock)
    {
	struct Library *BattClockBase = OpenResource("battclock.resource");

	if (BattClockBase)
	{
	    struct MsgPort *mp = CreateMsgPort();
	    
	    if (mp)
	    {
	    	struct timerequest *tr = (struct timerequest *)CreateIORequest(mp, sizeof(struct timerequest));
	    
	    	if (tr)
	    	{
		    if (OpenDevice("timer.device", UNIT_VBLANK, &tr->tr_node, 0) == 0)
		    {
			ULONG time = ReadBattClock();

			if (tmp)
			{
			    /* loc_GMTOffset actually expresses difference from local time to GMT */
			    time -= localeprefs.lp_GMTOffset * 60;
			}

			/* Set timer.device clock */
			tr->tr_node.io_Command = TR_SETSYSTIME;
			tr->tr_time.tv_secs    = time;
			tr->tr_time.tv_micro   = 0;
			tr->tr_node.io_Flags   = IOF_QUICK;
			DoIO(&tr->tr_node);

			CloseDevice(&tr->tr_node);
		    }
		    DeleteIORequest(&tr->tr_node);
		}
		DeleteMsgPort(mp);
	    }
	}
    }
    localeprefs.lp_GMTOffset = newtz;
    localeprefs.lp_Flags = newflags;
}

/*
 * update gadgets with values of struct localeprefs
 */
STATIC VOID LocalePrefs2Gadgets(struct LocaleRegister_DATA *data)
{

    SET(data->region, MUIA_Region_Regionname, localeprefs.lp_CountryName);

    SET(data->language, MUIA_Language_Preferred, TRUE);
    SET(data->language, MUIA_Language_Characterset, character_set);

    SET(data->timezone, MUIA_Timezone_Timeoffset, -localeprefs.lp_GMTOffset);
    SET(data->timezone, MUIA_Timezone_GMTClock, localeprefs.lp_Flags & LOCF_GMT_CLOCK);
}

IPTR LocaleRegister__MUIM_PrefsEditor_ImportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[localeregister class] LocaleRegister Class Import\n"));

    success = Prefs_ImportFH(message->fh);
    if (success) LocalePrefs2Gadgets(data);

    return success;
}

IPTR LocaleRegister__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[localeregister class] LocaleRegister Class Export\n"));

    Gadgets2LocalePrefs(data);
    success = Prefs_ExportFH(message->fh);

    return success;
}

IPTR LocaleRegister__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[localeregister class] LocaleRegister Class SetDefaults\n"));

    success = Prefs_Default();
    if (success) LocalePrefs2Gadgets(data);

    return success;
}

IPTR LocaleRegister__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    if(data->tab_label)
    {
        FreeVec(data->tab_label);
        data->tab_label = NULL;
    }

    return DoSuperMethodA(CLASS, self, message);
}

IPTR LocaleRegister__MUIM_PrefsEditor_Save(Class *cl, Object *obj, Msg msg)
{
    IPTR res;
    struct LocaleRegister_DATA *data = INST_DATA(cl, obj);

    /* We set this flag in order not to call Prefs_SaveCharset() twice */
    data->save = TRUE;
    res = DoSuperMethodA(cl, obj, msg);
    data->save = FALSE;

    return res && Prefs_SaveCharset(TRUE);
}

IPTR LocaleRegister__MUIM_PrefsEditor_Use(Class *cl, Object *obj, Msg msg)
{
    struct LocaleRegister_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, msg))
    	return FALSE;

    if (data->save)
    	return TRUE;

    return Prefs_SaveCharset(FALSE);
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_7
(
    LocaleRegister, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    OM_DISPOSE,                   Msg,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg,
    MUIM_PrefsEditor_Save,	  Msg,
    MUIM_PrefsEditor_Use,	  Msg
);
