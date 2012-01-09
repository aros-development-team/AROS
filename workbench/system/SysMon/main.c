/*
    Copyright 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/alib.h>
#include <proto/muimaster.h>
#include <dos/dos.h>

#include "sysmon_intern.h"

#include "locale.h"

#define VERSION "$VER: SysMon 1.1 (09.18.2011) ©2011 The AROS Development Team"

AROS_UFH3(VOID, tasklistrefreshbuttonfunction,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    UpdateTasksInformation(h->h_Data);

    AROS_USERFUNC_EXIT
}

BOOL CreateApplication(struct SysMonData * smdata)
{
    Object * cpucolgroup;
    Object * tasklistrefreshbutton;
    Object * tasklistautorefreshcheckmark;
    Object * cpuusagegroup;
    Object * cpufreqgroup;
    ULONG i;
    LONG processorcount = GetProcessorCount();

    smdata->tabs[0] = _(MSG_TAB_TASKS);
    smdata->tabs[1] = _(MSG_TAB_CPU);
    smdata->tabs[2] = _(MSG_TAB_SYSTEM);
    smdata->tabs[3] = NULL;

    smdata->tasklistdisplayhook.h_Entry = (APTR)TasksListDisplayFunction;
    smdata->tasklistrefreshbuttonhook.h_Entry = (APTR)tasklistrefreshbuttonfunction;
    smdata->tasklistrefreshbuttonhook.h_Data = (APTR)smdata;
    
    smdata->tasklistautorefresh = (IPTR)0;

    smdata->application = ApplicationObject,
        MUIA_Application_Title, __(MSG_APP_NAME),
        MUIA_Application_Version, (IPTR) VERSION,
        MUIA_Application_Author, (IPTR) "Krzysztof Smiechowicz",
        MUIA_Application_Copyright, (IPTR)"©2011, The AROS Development Team",
        MUIA_Application_Base, (IPTR)"SYSMON",
        MUIA_Application_Description, __(MSG_APP_TITLE),
        SubWindow, 
            smdata->mainwindow = WindowObject,
                MUIA_Window_Title, __(MSG_WINDOW_TITLE),
                MUIA_Window_ID, MAKE_ID('S','Y','S','M'),
                MUIA_Window_Height, MUIV_Window_Height_Visible(45),
                MUIA_Window_Width, MUIV_Window_Width_Visible(35),
                WindowContents,
                    RegisterGroup(smdata->tabs),
                        Child, VGroup,
                            Child, ListviewObject, 
                                MUIA_Listview_List, smdata->tasklist = ListObject,
                                    ReadListFrame,
                                    MUIA_List_Format, "MIW=50 BAR,BAR,",
                                    MUIA_List_DisplayHook, &smdata->tasklistdisplayhook,
                                    MUIA_List_Title, (IPTR)TRUE,
                                End,
                            End,
                            Child, ColGroup(2),
                                Child, VGroup,
                                    Child, tasklistrefreshbutton = MUI_MakeObject(MUIO_Button, _(MSG_LIST_REFRESH)),
                                    Child, ColGroup(2),
                                        Child, Label(_(MSG_AUTO_REFRESH)),
                                        Child, tasklistautorefreshcheckmark = MUI_MakeObject(MUIO_Checkmark, NULL),
                                    End,
                                End,
                                Child, HVSpace,
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
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"2097152 Kb", 
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
                                                MUIA_Text_PreParse, (IPTR)"\33r", MUIA_Text_Contents, (IPTR)"2097152 Kb", 
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

    DoMethod(tasklistrefreshbutton, MUIM_Notify, MUIA_Pressed, FALSE,
        smdata->application, 2, MUIM_CallHook, (IPTR)&smdata->tasklistrefreshbuttonhook);

    DoMethod(tasklistautorefreshcheckmark, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        tasklistrefreshbutton, 3, MUIM_Set, MUIA_Disabled, MUIV_TriggerValue);

    DoMethod(tasklistautorefreshcheckmark, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        tasklistrefreshbutton, 3, MUIM_WriteLong, MUIV_TriggerValue, &smdata->tasklistautorefresh);

    /* Adding cpu usage gauges */
    cpucolgroup = ColGroup(processorcount + 1), End;
    
    smdata->cpuusagegauges = AllocVec(sizeof(Object *) * processorcount, MEMF_ANY | MEMF_CLEAR);
    
    for (i = 0; i < processorcount; i++)
    {
        smdata->cpuusagegauges[i] = GaugeObject, GaugeFrame, MUIA_Gauge_InfoText, (IPTR) " CPU XX : XXX% ",
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

VOID DeInitModules(struct SysMonModule ** modules, LONG lastinitedmodule)
{
    LONG i;
    
    for (i = lastinitedmodule; i >= 0; i--)
        modules[i]->DeInit();
}

LONG InitModules(struct SysMonModule ** modules)
{
    LONG lastinitedmodule = -1;

    while(modules[lastinitedmodule + 1] != NULL)
    {
        if (modules[lastinitedmodule + 1]->Init())
            lastinitedmodule++;
        else
        {
            DeInitModules(modules, lastinitedmodule);
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

    if ((lastinitedmodule = InitModules(modules)) == -1)
        return 1;

    if (!CreateApplication(&smdata))
        return 1;

    UpdateProcessorStaticInformation(&smdata);
    UpdateProcessorInformation(&smdata);
    UpdateTasksInformation(&smdata);
    UpdateMemoryStaticInformation(&smdata);
    UpdateMemoryInformation(&smdata);
    UpdateVideoStaticInformation(&smdata);
    UpdateVideoInformation(&smdata);
    SignalMeAfter(250);

    set(smdata.mainwindow, MUIA_Window_Open, TRUE);

    while (DoMethod(smdata.application, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit)
    {
        if (signals)
        {
            signals = Wait(signals | SIGBREAKF_CTRL_C | GetSIG_TIMER());
            if (signals & SIGBREAKF_CTRL_C) break;
            if (signals & GetSIG_TIMER())
            {
                UpdateProcessorInformation(&smdata);

                if ((itercounter % 4) == 0)
                {
                    UpdateMemoryInformation(&smdata);
                    UpdateVideoInformation(&smdata);
                    
                    if (smdata.tasklistautorefresh)
                        UpdateTasksInformation(&smdata);
                }

                itercounter++;

                SignalMeAfter(250);
            }
        }
    }

    set(smdata.mainwindow, MUIA_Window_Open, FALSE);

    DisposeApplication(&smdata);

    DeInitModules(modules, lastinitedmodule);

    return 0;
}
