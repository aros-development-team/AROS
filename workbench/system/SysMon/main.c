/*
    Copyright 2010-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

//#define DEBUG 1
#include <aros/debug.h>

#include <proto/alib.h>
#include <proto/muimaster.h>
#include <dos/dos.h>

#include "sysmon_intern.h"

#include "locale.h"

#define VERSION "$VER: SysMon 1.3 (19.01.2017) ©2011-2017 The AROS Development Team"

//#define NOTYET_USED

#ifdef NOTYET_USED
AROS_UFH3(VOID, pageactivefunction,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    IPTR thisPage = 0;

    D(bug("[SysMon] pageactivefunction(0x%p)\n", object));

    // get the selected page
    get(object, MUIA_Group_ActivePage, &thisPage);

    D(bug("[SysMon] page #%d\n", thisPage));

    // TODO: change the menu for the active page.

    AROS_USERFUNC_EXIT
}
#endif

BOOL CreateApplication(struct SysMonData * smdata)
{
    Object * cpucolgroup;
    Object * menuitemfast;
    Object * menuitemnormal;
    Object * menuitemslow;
    Object * cpuusagegroup;
    Object * cpufreqgroup;
    ULONG i;
    LONG processorcount = GetProcessorCount();

    smdata->tabs[0] = _(MSG_TAB_TASKS);
    smdata->tabs[1] = _(MSG_TAB_CPU);
    smdata->tabs[2] = _(MSG_TAB_SYSTEM);
    smdata->tabs[3] = NULL;

    smdata->tasklistconstructhook.h_Entry = (APTR)TasksListConstructFunction;
    smdata->tasklistconstructhook.h_Data = (APTR)smdata;
    smdata->tasklistdestructhook.h_Entry = (APTR)TasksListDestructFunction;
    smdata->tasklistdestructhook.h_Data = (APTR)smdata;
    smdata->tasklistdisplayhook.h_Entry = (APTR)TasksListDisplayFunction;
    smdata->taskselectedhook.h_Entry = (APTR)TaskSelectedFunction;
    smdata->taskselectedhook.h_Data = (APTR)smdata;
    smdata->taskcomparehook.h_Entry = (APTR)TaskCompareFunction;
    smdata->taskcomparehook.h_Data = (APTR)smdata;

#ifdef NOTYET_USED
    smdata->pageactivehook.h_Entry = (APTR)pageactivefunction;
    smdata->pageactivehook.h_Data = (APTR)smdata;
#endif

    smdata->application = ApplicationObject,
        MUIA_Application_Title, __(MSG_APP_NAME),
        MUIA_Application_Version, (IPTR) VERSION,
        MUIA_Application_Author, (IPTR) "Krzysztof Smiechowicz",
        MUIA_Application_Copyright, (IPTR)"©2011-2017, The AROS Development Team",
        MUIA_Application_Base, (IPTR)"SYSMON",
        MUIA_Application_Description, __(MSG_APP_TITLE),
        SubWindow, 
            smdata->mainwindow = WindowObject,
                MUIA_Window_Title, __(MSG_WINDOW_TITLE),
                MUIA_Window_ID, MAKE_ID('S','Y','S','M'),
                MUIA_Window_Height, MUIV_Window_Height_Visible(45),
                MUIA_Window_Width, MUIV_Window_Width_Visible(35),
                MUIA_Window_Menustrip, (MenustripObject,
                            MUIA_Family_Child, (MenuObject, 
                                MUIA_Menu_Title, (IPTR)"Project", 
                                MUIA_Family_Child, (MenuitemObject, 
                                    MUIA_Menuitem_Title, (IPTR)"Refresh Speed", 
                                    MUIA_Family_Child, (menuitemfast = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Fast", MUIA_Menuitem_Shortcut, (IPTR)"F",End), 
                                    MUIA_Family_Child, (menuitemnormal = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Normal", MUIA_Menuitem_Shortcut, (IPTR)"N",End), 
                                    MUIA_Family_Child, (menuitemslow = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Slow", MUIA_Menuitem_Shortcut, (IPTR)"S",End), 
                                End), 
                            End),
                        End),
                WindowContents,
                    smdata->pages = RegisterGroup(smdata->tabs),
                        Child, VGroup,
                            Child, ListviewObject, 
                                MUIA_Listview_List, smdata->tasklist = ListObject,
                                    ReadListFrame,
                                    MUIA_List_Format, "MIW=50 BAR,BAR P=\033r,",
                                    MUIA_List_ConstructHook, &smdata->tasklistconstructhook,
                                    MUIA_List_DestructHook, &smdata->tasklistdestructhook,
                                    MUIA_List_DisplayHook, &smdata->tasklistdisplayhook,
                                    MUIA_List_CompareHook, &smdata->taskcomparehook,
                                    MUIA_List_Title, (IPTR)TRUE,
                                End,
                            End,
                            Child, VGroup,
                                Child, smdata->tasklistinfo = TextObject,
                                    NoFrame,
                                    MUIA_Font, MUIV_Font_Tiny,
                                    MUIA_Text_PreParse, (IPTR)"\33r",
                                    MUIA_Text_Contents, (IPTR)"- ready, - waiting",
                                End,
                            End,
                        End,
                        Child, VGroup,
                            Child, cpuusagegroup = HGroup, GroupFrameT(_(MSG_USAGE)), 
                            End,
                            Child, VGroup, GroupFrameT(_(MSG_FREQUENCY)),
                                Child, cpufreqgroup = ColGroup(3), 
                                End,
                            End,
                        End,
                        Child, VGroup,
                            Child, ColGroup(2),
                                Child, VGroup, GroupFrameT(_(MSG_MEMORY_SIZE)),
                                        Child, ColGroup(2), 
                                        Child, Label(_(MSG_TOTAL_RAM)),
                                        Child, smdata->memorysize[MEMORY_RAM] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"---- Kb", 
                                        End,
                                        Child, Label(_(MSG_CHIP_RAM)),
                                        Child, smdata->memorysize[MEMORY_CHIP] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"", 
                                        End,
                                        Child, Label(_(MSG_FAST_RAM)),
                                        Child, smdata->memorysize[MEMORY_FAST] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"", 
                                        End,
                                    End,
                                End,
                                Child, VGroup, GroupFrameT(_(MSG_MEMORY_FREE)),
                                    Child, ColGroup(2), 
                                        Child, Label(_(MSG_TOTAL_RAM)),
                                        Child, smdata->memoryfree[MEMORY_RAM] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"---- Kb", 
                                        End,
                                        Child, Label(_(MSG_CHIP_RAM)),
                                        Child, smdata->memoryfree[MEMORY_CHIP] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"", 
                                        End,
                                        Child, Label(_(MSG_FAST_RAM)),
                                        Child, smdata->memoryfree[MEMORY_FAST] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"", 
                                        End,
                                    End,
                                End,
                            End,
                            Child, ColGroup(2),
                                Child, VGroup, GroupFrameT(_(MSG_VIDEO_SIZE)),
                                    Child, ColGroup(2), 
                                        Child, Label(_(MSG_VIDEO_RAM)),
                                        Child, smdata->memorysize[MEMORY_VRAM] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"", 
                                        End,
                                        Child, Label(_(MSG_GART_APER)),
                                        Child, smdata->memorysize[MEMORY_GART] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"", 
                                        End,
                                    End,
                                End,
                                Child, VGroup, GroupFrameT(_(MSG_VIDEO_FREE)),
                                    Child, ColGroup(2), 
                                        Child, Label(_(MSG_VIDEO_RAM)),
                                        Child, smdata->memoryfree[MEMORY_VRAM] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"", 
                                        End,
                                        Child, Label(_(MSG_GART_APER)),
                                        Child, smdata->memoryfree[MEMORY_GART] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"", 
                                        End,
                                    End,
                                End,
                            End,
                            Child, HVSpace,
                        End,
                    End,
            End,
    End;

    if (!smdata->application)
        return FALSE;

    DoMethod(smdata->mainwindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        smdata->application, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(smdata->tasklist, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
        smdata->application, 2, MUIM_CallHook, (IPTR)&smdata->taskselectedhook);

    DoMethod(menuitemfast, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        smdata->application, 3, MUIM_WriteLong, 500, &smdata->updateSpeed);
    DoMethod(menuitemnormal, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        smdata->application, 3, MUIM_WriteLong, 1000, &smdata->updateSpeed);
    DoMethod(menuitemslow, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        smdata->application, 3, MUIM_WriteLong, 2000, &smdata->updateSpeed);

#ifdef NOTYET_USED
    DoMethod(smdata->pages, MUIM_Notify, MUIA_Group_ActivePage, MUIV_EveryTime,
        smdata->pages, 2, MUIM_CallHook, (IPTR)&smdata->pageactivehook);
#endif

    /* Adding cpu usage gauges */
    cpucolgroup = ColGroup(processorcount + 1), End;

    smdata->cpuusagegauges = AllocVec(sizeof(Object *) * processorcount, MEMF_ANY | MEMF_CLEAR);

    for (i = 0; i < processorcount; i++)
    {
        smdata->cpuusagegauges[i] = GaugeObject, GaugeFrame, MUIA_Gauge_InfoText, (IPTR) " CPU -- : ---% ",
                        MUIA_Gauge_Horiz, FALSE, MUIA_Gauge_Current, 0, 
                        MUIA_Gauge_Max, 100, End;

        DoMethod(cpucolgroup, OM_ADDMEMBER, smdata->cpuusagegauges[i]);
    }

    DoMethod(cpucolgroup, OM_ADDMEMBER, (IPTR)HVSpace);

    DoMethod(cpuusagegroup, OM_ADDMEMBER, cpucolgroup);

    /* Adding cpu frequency labels */
    smdata->cpufreqlabels = AllocVec(sizeof(Object *) * processorcount, MEMF_ANY | MEMF_CLEAR);
    smdata->cpufreqvalues = AllocVec(sizeof(Object *) * processorcount, MEMF_ANY | MEMF_CLEAR);

    for (i = 0; i < processorcount; i++)
    {
        smdata->cpufreqlabels[i] = TextObject, MUIA_Text_PreParse, "\33l",
                        MUIA_Text_Contents, (IPTR)"", End;
        smdata->cpufreqvalues[i] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                        MUIA_Text_PreParse, (IPTR)"\33l",
                        MUIA_Text_Contents, (IPTR)"", End;

        DoMethod(cpufreqgroup, OM_ADDMEMBER, smdata->cpufreqlabels[i]);
        DoMethod(cpufreqgroup, OM_ADDMEMBER, smdata->cpufreqvalues[i]);
        DoMethod(cpufreqgroup, OM_ADDMEMBER, (IPTR)HVSpace);
    }

    return TRUE;
}

VOID DisposeApplication(struct SysMonData * smdata)
{
    MUI_DisposeObject(smdata->application);

    FreeVec(smdata->cpuusagegauges);
    FreeVec(smdata->cpufreqlabels);
    FreeVec(smdata->cpufreqvalues);
}

VOID DeInitModules(struct SysMonModule ** modules, struct SysMonData *smdata, LONG lastinitedmodule)
{
    LONG i;

    for (i = lastinitedmodule; i >= 0; i--)
        modules[i]->DeInit(smdata);
}

LONG InitModules(struct SysMonModule ** modules, struct SysMonData *smdata)
{
    LONG lastinitedmodule = -1;

    while(modules[lastinitedmodule + 1] != NULL)
    {
        if (modules[lastinitedmodule + 1]->Init(smdata))
            lastinitedmodule++;
        else
        {
            DeInitModules(modules, smdata, lastinitedmodule);
            return -1;
        }
    }

    return lastinitedmodule;
}

int main()
{
    ULONG signals = 0;
    ULONG itercounter = 0;
    struct SysMonData smdata;
    struct SysMonModule * modules [] = {&memorymodule, &videomodule, &processormodule, &tasksmodule, &timermodule, NULL};
    LONG lastinitedmodule = -1;

    if ((lastinitedmodule = InitModules(modules, &smdata)) == -1)
        return 1;

    if (!CreateApplication(&smdata))
        return 1;

    smdata.updateSpeed = 1000;

    UpdateProcessorStaticInformation(&smdata);
    UpdateProcessorInformation(&smdata);
    UpdateTasksInformation(&smdata);
    UpdateMemoryStaticInformation(&smdata);
    UpdateMemoryInformation(&smdata);
    UpdateVideoStaticInformation(&smdata);
    UpdateVideoInformation(&smdata);

    set(smdata.mainwindow, MUIA_Window_Open, TRUE);
    set(smdata.pages, MUIA_Group_ActivePage, 0);
    SignalMeAfter(250);

    while (DoMethod(smdata.application, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit)
    {
        if (signals)
        {
            signals = Wait(signals | SIGBREAKF_CTRL_C | GetSIG_TIMER());
            if (signals & SIGBREAKF_CTRL_C)
                break;

            if (signals & GetSIG_TIMER())
            {
                IPTR currentPage = 0;

                get(smdata.pages, MUIA_Group_ActivePage, &currentPage);
                switch (currentPage)
                {
                    case 0:
                        if ((itercounter % (smdata.updateSpeed/250)) == 0)
                        {
                            UpdateTasksInformation(&smdata);
                        }
                        break;
                    case 1:
                            UpdateProcessorInformation(&smdata);
                        break;
                    case 2:
                        if ((itercounter % 8) == 0)
                        {
                            UpdateMemoryInformation(&smdata);
                            UpdateVideoInformation(&smdata);
                        }
                        break;
                    default:
                        break;
                }

                itercounter++;

                SignalMeAfter(250);
            }
        }
    }

    set(smdata.mainwindow, MUIA_Window_Open, FALSE);

    DisposeApplication(&smdata);

    DeInitModules(modules, &smdata, lastinitedmodule);

    return 0;
}
