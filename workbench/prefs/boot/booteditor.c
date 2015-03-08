/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/locale.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <zune/prefseditor.h>
#include <libraries/asl.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>

#include <string.h>
#include <stdio.h>

#include "locale.h"
#include "booteditor.h"

#include <proto/alib.h>

#define DEFAULT_WIDTH 1024
#define DEFAULT_HEIGHT 768
#define DEFAULT_REFRESH 60
#define MAX_LINE_LENGTH 500

static const TEXT grub_config_path[] = "SYS:boot/pc/grub/grub.cfg";
static const TEXT grub_config_path_tmp[] = "SYS:boot/pc/grub/grub.cfg.tmp";
static const TEXT accept_nums[] = "0123456789";
static CONST_STRPTR ata_buses_list[5] = {NULL};
static CONST_STRPTR debug_output_list[4] = {NULL};
static CONST_STRPTR gfx_type_list[5] = {NULL};
static CONST_STRPTR vesa_depth_list[4] = {NULL};
static CONST_STRPTR entry_tabs[3] = {NULL};

struct BootEditor_DATA
{
    Object *gfx_type,
           *gfx_composition,
           *vesa_group,
           *vesa_width,
           *vesa_height,
           *vesa_best_res,
           *vesa_depth,
           *vesa_refresh,
           *vesa_default_refresh,
           *ata_buses,
           *ata_dma,
           *ata_32bit,
           *ata_poll,
           *ata_multi,
           *device_name,
           *device_delay,
           *usb_enable,
           *acpi_enable,
           *floppy_enable,
           *debug_output,
           *debug_mungwall,
           *debug_usb,
           *module_list,
           *add_button,
           *remove_button,
           *module_pop_string,
           *module_path,
           *module_active;
};

struct module_entry
{
    STRPTR path;
    BOOL active;
};

static struct Hook module_display_hook;

static BOOL ReadBootArgs(CONST_STRPTR line, struct BootEditor_DATA *data);
static BOOL WriteBootArgs(BPTR file, struct BootEditor_DATA *data);
static BOOL ReadModule(CONST_STRPTR line, struct BootEditor_DATA *data);
static BOOL WriteModule(Object *obj, BPTR file, struct module_entry *entry);
AROS_UFP3S(LONG, ModuleDisplayHook,
    AROS_UFPA(struct Hook *, hook, A0),
    AROS_UFPA(char **, array, A2),
    AROS_UFPA(struct module_entry *, entry, A1));

static Object *BootEditor__OM_NEW(Class *CLASS, Object *self,
    struct opSet *message)
{
    struct BootEditor_DATA temp_data, *data = &temp_data;

    module_display_hook.h_Entry = HookEntry;
    module_display_hook.h_SubEntry = (HOOKFUNC)ModuleDisplayHook;

    entry_tabs[0] = _(MSG_OPTIONS);
    entry_tabs[1] = _(MSG_MODULES);

    gfx_type_list[0] = _(MSG_GFX_TYPE_AUTO);
    gfx_type_list[1] = _(MSG_GFX_TYPE_NATIVE);
    gfx_type_list[2] = _(MSG_GFX_TYPE_VESA);
    gfx_type_list[3] = _(MSG_GFX_TYPE_VGA);

    vesa_depth_list[0] = _(MSG_GFX_DEPTH_TRUE);
    vesa_depth_list[1] = _(MSG_GFX_DEPTH_HIGH);
    vesa_depth_list[2] = _(MSG_GFX_DEPTH_LOW);

    ata_buses_list[0] = _(MSG_ATA_BUS_ALL);
    ata_buses_list[1] = _(MSG_ATA_BUS_PCI);
    ata_buses_list[2] = _(MSG_ATA_BUS_LEGACY);
    ata_buses_list[3] = _(MSG_ATA_BUS_NONE);

    debug_output_list[0] = _(MSG_DEBUG_OUTPUT_NONE);
    debug_output_list[1] = _(MSG_DEBUG_OUTPUT_MEMORY);
    debug_output_list[2] = _(MSG_DEBUG_OUTPUT_SERIAL);

    self = (Object *)DoSuperNewTags(CLASS, self, NULL,
        MUIA_PrefsEditor_Name, __(MSG_NAME),
        MUIA_PrefsEditor_Path, (IPTR)grub_config_path,
        MUIA_PrefsEditor_CanTest, FALSE,
        MUIA_PrefsEditor_CanUse, FALSE,

        Child, (IPTR)RegisterGroup(entry_tabs),

            /* Options tab */

            Child, (IPTR)ColGroup(2),
                Child, (IPTR)VGroup,
                    GroupFrameT(_(MSG_GFX)),
                    Child, (IPTR)ColGroup(2),
                        Child, (IPTR)Label2(__(MSG_GFX_TYPE)),
                        Child, (IPTR)(data->gfx_type = (Object *)CycleObject,
                            MUIA_Cycle_Entries, (IPTR)gfx_type_list,
                        End),
                        Child, (IPTR)(data->gfx_composition =
                            MUI_MakeObject(MUIO_Checkmark, NULL)),
                        Child, (IPTR)HGroup,
                            Child, (IPTR)Label2(__(MSG_GFX_COMPOSITION)),
                            Child, (IPTR)HVSpace,
                        End,
                        Child, (IPTR)HVSpace,
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)(data->vesa_group = VGroup,
                        GroupFrameT(_(MSG_GFX_VESAMODE)),
                        Child, (IPTR)HGroup,
                            Child, (IPTR)(data->vesa_best_res =
                                MUI_MakeObject(MUIO_Checkmark, NULL)),
                            Child, (IPTR)Label2(__(MSG_GFX_BESTRES)),
                            Child, (IPTR)HVSpace,
                        End,
                        Child, (IPTR)ColGroup(2),
                            Child, (IPTR)Label2(__(MSG_GFX_WIDTH)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(data->vesa_width =
                                    (Object *)StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_Accept, (IPTR)accept_nums,
                                    MUIA_FixWidthTxt, (IPTR)"00000",
                                End),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)Label2(__(MSG_GFX_HEIGHT)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(data->vesa_height =
                                    (Object *)StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_Accept, (IPTR)accept_nums,
                                    MUIA_FixWidthTxt, (IPTR)"00000",
                                End),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)Label2(__(MSG_GFX_DEPTH)),
                            Child, (IPTR)(data->vesa_depth =
                                (Object *)CycleObject,
                                MUIA_Cycle_Entries, (IPTR)vesa_depth_list,
                            End),
                            Child, (IPTR)Label2(__(MSG_GFX_REFRESH)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(data->vesa_refresh =
                                    (Object *)StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_Accept, (IPTR)accept_nums,
                                    MUIA_FixWidthTxt, (IPTR)"0000",
                                End),
                                Child, (IPTR)Label2(__(MSG_GFX_HERTZ)),
                                Child, (IPTR)HVSpace,
                            End,
                        End,
                        Child, (IPTR)HGroup,
                            Child, (IPTR)(data->vesa_default_refresh =
                                MUI_MakeObject(MUIO_Checkmark, NULL)),
                            Child, (IPTR)Label2(__(MSG_GFX_DEFAULTREFRESH)),
                            Child, (IPTR)HVSpace,
                        End,
                    End),
                End,
                Child, (IPTR)VGroup,
                    Child, (IPTR)VGroup,
                        GroupFrameT(_(MSG_ATA)),
                        Child, (IPTR)HGroup,
                            Child, (IPTR)Label2(__(MSG_ATA_BUSES)),
                            Child, (IPTR)(data->ata_buses =
                                (Object *)CycleObject,
                                MUIA_Cycle_Entries, (IPTR)ata_buses_list,
                            End),
                        End,
                        Child, (IPTR)ColGroup(2),
                            Child, (IPTR)(data->ata_dma =
                                MUI_MakeObject(MUIO_Checkmark, NULL)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)Label2(__(MSG_ATA_DMA)),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)(data->ata_multi =
                                MUI_MakeObject(MUIO_Checkmark, NULL)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)Label2(__(MSG_ATA_MULTI)),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)(data->ata_32bit =
                                MUI_MakeObject(MUIO_Checkmark, NULL)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)Label2(__(MSG_ATA_32BIT)),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)(data->ata_poll =
                                MUI_MakeObject(MUIO_Checkmark, NULL)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)Label2(__(MSG_ATA_POLL)),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)HVSpace,
                            Child, (IPTR)HVSpace,
                        End,
                    End,
                    Child, (IPTR)VGroup,
                        GroupFrameT(_(MSG_DEVICE)),
                        Child, (IPTR)ColGroup(2),
                            Child, (IPTR)Label2(__(MSG_DEVICE_NAME)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(data->device_name =
                                    (Object *)StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_Reject, (IPTR)":; ",
                                    MUIA_FixWidthTxt, (IPTR)"000000000",
                                End),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)Label2(__(MSG_DEVICE_DELAY)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(data->device_delay =
                                    (Object *)StringObject,
                                    StringFrame,
                                    MUIA_CycleChain, 1,
                                    MUIA_String_Accept, (IPTR)accept_nums,
                                    MUIA_FixWidthTxt, (IPTR)"000",
                                End),
                                Child, (IPTR)Label2(__(MSG_DEVICE_SECONDS)),
                                Child, (IPTR)HVSpace,
                            End,
                        End,
                    End,
                End,
                Child, (IPTR)ColGroup(2),
                    GroupFrameT(_(MSG_MISC)),
                    Child, (IPTR)(data->usb_enable =
                        MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)Label2(__(MSG_USB_ENABLE)),
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)(data->acpi_enable =
                        MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)Label2(__(MSG_ACPI_ENABLE)),
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)(data->floppy_enable =
                        MUI_MakeObject(MUIO_Checkmark, NULL)),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)Label2(__(MSG_FLOPPY_ENABLE)),
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)HVSpace,
                    Child, (IPTR)HVSpace,
                End,
                Child, (IPTR)VGroup,
                    GroupFrameT(_(MSG_DEBUG)),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)Label2(__(MSG_DEBUG_OUTPUT)),
                        Child, (IPTR)(data->debug_output =
                            (Object *)CycleObject,
                            MUIA_Cycle_Entries, (IPTR)debug_output_list,
                        End),
                    End,
                    Child, (IPTR)ColGroup(2),
                        Child, (IPTR)(data->debug_mungwall =
                            MUI_MakeObject(MUIO_Checkmark, NULL)),
                        Child, (IPTR)HGroup,
                            Child, (IPTR)Label2(__(MSG_DEBUG_MUNGWALL)),
                            Child, (IPTR)HVSpace,
                        End,
                        Child, (IPTR)(data->debug_usb =
                            MUI_MakeObject(MUIO_Checkmark, NULL)),
                        Child, (IPTR)HGroup,
                            Child, (IPTR)Label2(__(MSG_DEBUG_USB)),
                            Child, (IPTR)HVSpace,
                        End,
                    End,
                    Child, (IPTR)HVSpace,
                End,
            End,

            /* Modules tab */

            Child, (IPTR)VGroup,
                Child, (IPTR)ListviewObject,
                    MUIA_Listview_List, (IPTR)(data->module_list =
                        (Object *)ListObject,
                        InputListFrame,
                        MUIA_List_AutoVisible, TRUE,
                        MUIA_List_Title, TRUE,
                        MUIA_List_Format, (IPTR)"P=\033c BAR,",
                        MUIA_List_DisplayHook, (IPTR)&module_display_hook,
                    End),
                    MUIA_CycleChain, 1,
                End,
                Child, (IPTR)VGroup,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(data->add_button =
                            SimpleButton(_(MSG_ADD))),
                        Child, (IPTR)(data->remove_button =
                            SimpleButton(_(MSG_REMOVE))),
                    End,
                    Child, (IPTR)HGroup,
                        GroupFrame,
                        Child, (IPTR)Label2(__(MSG_PATH)),
                        Child, (IPTR)(data->module_pop_string = PopaslObject,
                            MUIA_Popasl_Type, ASL_FileRequest,
                            MUIA_Popstring_String,
                                (IPTR)(data->module_path =
                                    (Object *)StringObject,
                                    StringFrame,
                                    MUIA_Background, MUII_TextBack,
                                    MUIA_CycleChain, 1,
                                    MUIA_Disabled, TRUE,
                                End),
                            MUIA_Popstring_Button,
                                (IPTR)PopButton(MUII_PopFile),
                        End),
                        Child, (IPTR)(data->module_active =
                            MUI_MakeObject(MUIO_Checkmark, NULL)),
                        Child, (IPTR)Label2(_(MSG_ACTIVE)),
                    End,
                End,
            End,

        End,
        TAG_DONE
    );

    if (self != NULL)
    {
        data = INST_DATA(CLASS, self);
        *data = temp_data;

        /* Set up notifications */

        DoMethod(data->gfx_type, MUIM_Notify,
            MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->gfx_composition, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->vesa_width, MUIM_Notify,
            MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->vesa_height, MUIM_Notify,
            MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->vesa_depth, MUIM_Notify,
            MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->vesa_refresh, MUIM_Notify,
            MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->vesa_default_refresh, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->vesa_best_res, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->ata_buses, MUIM_Notify,
            MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->ata_dma, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->ata_multi, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->ata_32bit, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->ata_poll, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->device_name, MUIM_Notify,
            MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->device_delay, MUIM_Notify,
            MUIA_String_Acknowledge, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->usb_enable, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->acpi_enable, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->floppy_enable, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->debug_output, MUIM_Notify,
            MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->debug_mungwall, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(data->debug_usb, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);

        DoMethod(data->gfx_type, MUIM_Notify,
            MUIA_Cycle_Active, 0,
            (IPTR)data->vesa_group, 3, MUIM_Set, MUIA_Disabled, FALSE);
        DoMethod(data->gfx_type, MUIM_Notify,
            MUIA_Cycle_Active, 1,
            (IPTR)data->vesa_group, 3, MUIM_Set, MUIA_Disabled, TRUE);
        DoMethod(data->gfx_type, MUIM_Notify,
            MUIA_Cycle_Active, 2,
            (IPTR)data->vesa_group, 3, MUIM_Set, MUIA_Disabled, FALSE);
        DoMethod(data->gfx_type, MUIM_Notify,
            MUIA_Cycle_Active, 3,
            (IPTR)data->vesa_group, 3, MUIM_Set, MUIA_Disabled, TRUE);

        DoMethod(data->vesa_default_refresh, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)data->vesa_refresh, 3, MUIM_Set, MUIA_Disabled,
            MUIV_TriggerValue);
        DoMethod(data->vesa_best_res, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)data->vesa_width, 3, MUIM_Set, MUIA_Disabled,
            MUIV_TriggerValue);
        DoMethod(data->vesa_best_res, MUIM_Notify,
            MUIA_Selected, MUIV_EveryTime,
            (IPTR)data->vesa_height, 3, MUIM_Set, MUIA_Disabled,
            MUIV_TriggerValue);

        DoMethod(data->module_list, MUIM_Notify, MUIA_List_Active,
            MUIV_EveryTime, (IPTR)self, 1, MUIM_BootEditor_ShowModule);
        DoMethod(data->module_path, MUIM_Notify, MUIA_String_Contents,
            MUIV_EveryTime, (IPTR)self, 1, MUIM_BootEditor_UpdateModule);
        DoMethod(data->module_active, MUIM_Notify, MUIA_Selected,
            MUIV_EveryTime, (IPTR)self, 1, MUIM_BootEditor_UpdateModule);
        DoMethod(data->add_button, MUIM_Notify, MUIA_Pressed,
            TRUE, (IPTR)self, 1, MUIM_BootEditor_AddModule);
        DoMethod(data->remove_button, MUIM_Notify, MUIA_Pressed, TRUE,
            (IPTR)self, 1, MUIM_BootEditor_RemoveModule);

        /* Set default values */

        SET(data->gfx_composition, MUIA_Selected, TRUE);
        SET(data->vesa_width, MUIA_String_Integer, DEFAULT_WIDTH);
        SET(data->vesa_height, MUIA_String_Integer, DEFAULT_HEIGHT);
        SET(data->vesa_refresh, MUIA_String_Integer, DEFAULT_REFRESH);

        SET(data->ata_dma, MUIA_Selected, TRUE);
        SET(data->ata_multi, MUIA_Selected, TRUE);
        SET(data->ata_32bit, MUIA_Selected, TRUE);

        SET(data->device_delay, MUIA_String_Integer, 0);

        SET(data->usb_enable, MUIA_Selected, TRUE);
        SET(data->acpi_enable, MUIA_Selected, TRUE);

        SET(data->remove_button, MUIA_Disabled, TRUE);
        SET(data->module_active, MUIA_Disabled, TRUE);
        SET(data->module_pop_string, MUIA_Disabled, TRUE);
    }

    return self;
}

static IPTR BootEditor__MUIM_Setup(Class *CLASS, Object *self, Msg message)
{
    if (!DoSuperMethodA(CLASS, self, message))
        return FALSE;

    return TRUE;
}

static IPTR BootEditor__MUIM_Cleanup(Class *CLASS, Object *self, Msg message)
{
    return DoSuperMethodA(CLASS, self, message);
}

static IPTR BootEditor__MUIM_PrefsEditor_ImportFH(Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message)
{
    struct BootEditor_DATA *data = INST_DATA(CLASS, self);
    BPTR file;
    TEXT line_buffer[MAX_LINE_LENGTH], *line;
    BOOL success = TRUE, done = FALSE;

    /* Find first AROS boot entry, parse its arguments and put them in
     * the GUI */

    file = Open(grub_config_path, MODE_OLDFILE);
    if (file == BNULL)
        success = FALSE;

    if (success)
    {
        while (!done)
        {
            line = FGets(file, line_buffer, MAX_LINE_LENGTH);
            if (line == NULL)
                done = TRUE;
            else if (strstr(line, "multiboot") != NULL
                && strstr(line, "bootstrap") != NULL)
            {
                if (!ReadBootArgs(line, data))
                    success = FALSE;
            }
            else if (strstr(line, "module ") != NULL)
            {
                if (!ReadModule(line, data))
                    success = FALSE;
            }
            if (strstr(line, "}") != NULL)
                done = TRUE;
        }
    }

    if (file != NULL)
        Close(file);

    return success;
}

static IPTR BootEditor__MUIM_PrefsEditor_ExportFH(Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message)
{
    return FALSE;
}

static IPTR BootEditor__MUIM_PrefsEditor_Save(Class *CLASS, Object *self,
    Msg message)
{
    struct BootEditor_DATA *data = INST_DATA(CLASS, self);
    BPTR old_file, new_file;
    TEXT line_buffer[MAX_LINE_LENGTH], *line;
    BOOL success = TRUE, done = FALSE, found = FALSE;
    struct module_entry *module;
    UWORD i;

    /* Find first AROS boot entry and replace its arguments with those
     * chosen in the GUI */

    old_file = Open(grub_config_path, MODE_OLDFILE);
    new_file = Open(grub_config_path_tmp, MODE_NEWFILE);
    if (old_file == BNULL && new_file == BNULL)
        success = FALSE;

    if (success)
    {
        while (!done)
        {
            line = FGets(old_file, line_buffer, MAX_LINE_LENGTH);
            if (line == NULL)
                done = TRUE;
            else if (!found && strstr(line, "multiboot") != NULL
                && strstr(line, "bootstrap") != NULL)
            {
                strstr(line, ".gz")[3] = '\0';
                if (FPuts(new_file, line) != 0)
                    success = FALSE;
                else if (!WriteBootArgs(new_file, data))
                    success = FALSE;
                else
                {
                    /* Skip past all module lines */

                    while (!found)
                    {
                        line = FGets(old_file, line_buffer, MAX_LINE_LENGTH);
                        if (line == NULL)
                            found = done = TRUE;
                        else if (strchr(line, '}') != NULL)
                            found = TRUE;
                    }

                    /* Write new module lines */

                    for (i = 0; success && DoMethod(data->module_list,
                        MUIM_List_GetEntry, i, &module) != (IPTR)NULL; i++)
                    {
                        if (!WriteModule(self, new_file, module))
                            success = FALSE;
                    }

                    /* Keep line with closing curly bracket */

                    if (FPuts(new_file, line) != 0)
                        success = FALSE;
                }
            }
            else
                if (FPuts(new_file, line) != 0)
                    success = FALSE;
        }
    }

    /* Close both files */

    if (old_file != NULL)
        Close(old_file);
    if (new_file != NULL)
        if (!Close(new_file))
            success = FALSE;

    /* Replace old file with new one */

    if (success)
        success = DeleteFile(grub_config_path);

    if (success)
        success = Rename(grub_config_path_tmp, grub_config_path);

    return success;
}

IPTR BootEditor__MUIM_BootEditor_ShowModule(Class *cl, Object *self,
    Msg message)
{
    struct BootEditor_DATA *data = INST_DATA(cl, self);
    struct module_entry *module;
    BOOL show;

    DoMethod(data->module_list, MUIM_List_GetEntry,
        MUIV_List_GetEntry_Active, &module);
    show = module != NULL;
    SET(data->module_pop_string, MUIA_Disabled, !show);
    SET(data->module_active, MUIA_Disabled, !show);
    NNSET(data->module_path, MUIA_String_Contents,
        (IPTR)(show ? module->path : NULL));
    NNSET(data->module_active, MUIA_Selected, show ? module->active : FALSE);
    SET(data->remove_button, MUIA_Disabled, !show);

    return 0;
}

IPTR BootEditor__MUIM_BootEditor_UpdateModule(Class *cl, Object *self,
    Msg message)
{
    struct BootEditor_DATA *data = INST_DATA(cl, self);
    struct module_entry *module;

    DoMethod(data->module_list, MUIM_List_GetEntry,
        MUIV_List_GetEntry_Active, &module);
    if (module != NULL)
    {
        FreeVec(module->path);
        module->path =
            StrDup((APTR)XGET(data->module_path, MUIA_String_Contents));
        module->active = XGET(data->module_active, MUIA_Selected);
        DoMethod(data->module_list, MUIM_List_Redraw,
            MUIV_List_Redraw_Active);
    }
    return 0;
}

IPTR BootEditor__MUIM_BootEditor_AddModule(Class *cl, Object *self,
    Msg message)
{
    struct BootEditor_DATA *data = INST_DATA(cl, self);
    struct module_entry *module;
    BOOL success = TRUE;

    module = AllocMem(sizeof(struct module_entry), MEMF_CLEAR);
    if (module == NULL)
        success = FALSE;

    if (success)
    {
        module->path = StrDup("");
        if (module->path == NULL)
            success = FALSE;
        module->active = TRUE;
    }

    if (success)
    {
        if (DoMethod(data->module_list, MUIM_List_InsertSingle, module,
            MUIV_List_Insert_Bottom) == -1)
            success = FALSE;
        SET(data->module_list, MUIA_List_Active, MUIV_List_Active_Bottom);
    }

    return 0;
}

IPTR BootEditor__MUIM_BootEditor_RemoveModule(Class *cl, Object *self,
    Msg message)
{
    struct BootEditor_DATA *data = INST_DATA(cl, self);
    struct module_entry *module;

    /* Remove entry from list and deallocate it */

    DoMethod(data->module_list, MUIM_List_GetEntry,
        MUIV_List_GetEntry_Active, &module);
    if (module != NULL)
    {
        DoMethod(data->module_list, MUIM_List_Remove,
            MUIV_List_Remove_Active);
        FreeVec(module->path);
        FreeMem(module, sizeof(struct module_entry));
    }

    return 0;
}

static BOOL ReadBootArgs(CONST_STRPTR line, struct BootEditor_DATA *data)
{
    UWORD choice, width = 0, height = 0, depth = 0, refresh = 0, delay = 0;
    BOOL success = TRUE, best_res = FALSE, use_refresh = FALSE;
    STRPTR options, p;
    TEXT ch;

    /* Graphics */

    if (strstr(line, "nomonitors") != NULL
        && strstr(line, "vesa=") == NULL)
        choice = 3;
    else if (strstr(line, "nomonitors") != NULL)
        choice = 2;
    else if (strstr(line, "vesa=") == NULL)
        choice = 1;
    else
        choice = 0;
    SET(data->gfx_type, MUIA_Cycle_Active, choice);

    NNSET(data->gfx_composition, MUIA_Selected,
        strstr(line, "nocomposition") == NULL);

    /* VESA */

    options = strstr(line, "vesa=");
    if (options != NULL)
    {
        options += 5;

        while (*options >= '0' && *options <= '9')
            width = width * 10 + *options++ - '0';
        if (*options++ == 'x')
        {
            while (*options >= '0' && *options <= '9')
                height = height * 10 + *options++ - '0';
            if (*options++ == 'x')
            {
                while (*options >= '0' && *options <= '9')
                    depth = depth * 10 + *options++ - '0';
            }
            else
                depth = 32;
        }
        else
        {
            depth = width, width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT;
            best_res = TRUE;
        }
        NNSET(data->vesa_width, MUIA_String_Integer, width);
        NNSET(data->vesa_height, MUIA_String_Integer, height);
        SET(data->vesa_best_res, MUIA_Selected, best_res);

        /* Check for user-set refresh rate */

        if (*(options - 1) == '@')
        {
            while (*options >= '0' && *options <= '9')
                refresh = refresh * 10 + *options++ - '0';
            use_refresh = TRUE;
        }
        else
            refresh = DEFAULT_REFRESH;

        NNSET(data->vesa_refresh, MUIA_String_Integer, refresh);
        SET(data->vesa_default_refresh, MUIA_Selected, !use_refresh);

        if (depth > 16)
            choice = 0;
        else if (depth > 8)
            choice = 1;
        else
            choice = 0;
        NNSET(data->vesa_depth, MUIA_Cycle_Active, choice);
    }

    /* ATA */

    options = strstr(line, "ATA=");
    if (options != NULL)
    {
        if (strstr(options, "nopci") != NULL
            && strstr(options, "nolegacy") != NULL)
            choice = 3;
        else if (strstr(options, "nopci") != NULL)
            choice = 2;
        else if (strstr(options, "nolegacy") != NULL)
            choice = 1;
        else
            choice = 0;
        NNSET(data->ata_buses, MUIA_Cycle_Active, choice);

        if (strstr(options, "nodma") != NULL)
            NNSET(data->ata_dma, MUIA_Selected, FALSE);
        if (strstr(options, "nomulti") != NULL)
            NNSET(data->ata_multi, MUIA_Selected, FALSE);
        if (strstr(options, "32bit") == NULL)
            NNSET(data->ata_multi, MUIA_Selected, FALSE);
        if (strstr(options, "poll") != NULL)
            NNSET(data->ata_poll, MUIA_Selected, TRUE);
    }

    /* Boot Device */

    options = strstr(line, "bootdevice=");
    if (options != NULL)
    {
        options += 11;

        for (p = options; *p != ' ' && *p != '\n' && *p != '\0'; p++);
        ch = *p;
        *p = '\0';
        NNSET(data->device_name, MUIA_String_Contents, options);
        *p = ch;
    }

    options = strstr(line, "bootdelay=");
    if (options != NULL)
    {
        options += 10;

        while (*options >= '0' && *options <= '9')
            delay = delay * 10 + *options++ - '0';
    }
    NNSET(data->device_delay, MUIA_String_Integer, delay);

    /* Miscellaneous */

    NNSET(data->usb_enable, MUIA_Selected,
        strstr(line, "enableusb") != NULL);
    NNSET(data->acpi_enable, MUIA_Selected,
        strstr(line, "noacpi") == NULL);
    NNSET(data->floppy_enable, MUIA_Selected,
        strstr(line, "floppy=disabled") == NULL);

    /* Debugging */

    if (strstr(line, "debug=serial") != NULL)
        choice = 2;
    else if (strstr(line, "debug=memory") != NULL)
        choice = 1;
    else
        choice = 0;
    NNSET(data->debug_output, MUIA_Cycle_Active, choice);

    NNSET(data->debug_mungwall, MUIA_Selected,
        strstr(line, "mungwall") != NULL);
    NNSET(data->debug_usb, MUIA_Selected,
        strstr(line, "usbdebug") != NULL);

    return success;
}

static BOOL WriteBootArgs(BPTR file, struct BootEditor_DATA *data)
{
    UWORD count, choice, width, height, depth, delay;
    BOOL success = TRUE;
    CONST_STRPTR name = NULL;

    /* VESA */

    if((XGET(data->gfx_type, MUIA_Cycle_Active) & 1) == 0)
    {
        FPrintf(file, " vesa=");
        if(!XGET(data->vesa_best_res, MUIA_Selected))
        {
            width = XGET(data->vesa_width, MUIA_String_Integer);
            height = XGET(data->vesa_height, MUIA_String_Integer);
#if 0
            FPrintf(file, "%ux%ux", width, height);
#else
            FPrintf(file, "%ux", width);
            FPrintf(file, "%ux", height);
#endif
        }
        choice = XGET(data->vesa_depth, MUIA_Cycle_Active);
        if (choice == 0)
            depth = 32;
        else if (choice == 1)
            depth = 16;
        else
            depth = 8;
        FPrintf(file, "%u", depth);
        if(!XGET(data->vesa_default_refresh, MUIA_Selected))
            FPrintf(file, "@%u", XGET(data->vesa_refresh, MUIA_String_Integer));
    }

    /* Graphics */

    if(XGET(data->gfx_type, MUIA_Cycle_Active) > 1)
        FPrintf(file, " nomonitors");
    if(!XGET(data->gfx_composition, MUIA_Selected))
        FPrintf(file, " nocomposition");

    /* ATA */

    if(!XGET(data->ata_dma, MUIA_Selected) ||
        !XGET(data->ata_multi, MUIA_Selected) ||
        XGET(data->ata_32bit, MUIA_Selected) ||
        XGET(data->ata_poll, MUIA_Selected) ||
        XGET(data->ata_buses, MUIA_Cycle_Active) > 0)
    {
        count = 0;
        FPrintf(file, " ATA=");

        choice = XGET(data->ata_buses, MUIA_Cycle_Active);
        if (choice == 1)
        {
            FPrintf(file, "nolegacy");
            count++;
        }
        else if (choice == 2)
        {
            FPrintf(file, "nopci");
            count++;
        }
        else if (choice == 3)
        {
            FPrintf(file, "nopci,nolegacy");
            count += 2;
        }

        if(!XGET(data->ata_dma, MUIA_Selected))
        {
            if(count > 0)
                FPrintf(file, ",");
            FPrintf(file, "nodma");
            count++;
        }
        if(!XGET(data->ata_multi, MUIA_Selected))
        {
            if(count > 0)
                FPrintf(file, ",");
            FPrintf(file, "nomulti");
            count++;
        }
        if(XGET(data->ata_32bit, MUIA_Selected))
        {
            if(count > 0)
                FPrintf(file, ",");
            FPrintf(file, "32bit");
            count++;
        }
        if(XGET(data->ata_poll, MUIA_Selected))
        {
            if(count > 0)
                FPrintf(file, ",");
            FPrintf(file, "poll");
            count++;
        }
    }

    /* Boot device */

    GET(data->device_name, MUIA_String_Contents, &name);
    if (*name != '\0')
        FPrintf(file, " bootdevice=%s", name);
    delay = XGET(data->device_delay, MUIA_String_Integer);
    if(delay != 0)
        FPrintf(file, " bootdelay=%u", delay);

    /* Miscellaneous */

    if(XGET(data->usb_enable, MUIA_Selected))
        FPrintf(file, " enableusb");
    if(!XGET(data->acpi_enable, MUIA_Selected))
        FPrintf(file, " noacpi");
    if(!XGET(data->floppy_enable, MUIA_Selected))
        FPrintf(file, " floppy=disabled");

    /* Debugging */

    if(XGET(data->debug_output, MUIA_Cycle_Active) == 1)
        FPrintf(file, " debug=memory");
    else if(XGET(data->debug_output, MUIA_Cycle_Active) == 2)
        FPrintf(file, " debug=serial");
    if(XGET(data->debug_mungwall, MUIA_Selected))
        FPrintf(file, " mungwall");
    if(XGET(data->debug_usb, MUIA_Selected))
        FPrintf(file, " usbdebug");

    if (FPrintf(file, "\n") < 0)
        success = FALSE;

    return success;
}

static BOOL ReadModule(CONST_STRPTR line, struct BootEditor_DATA *data)
{
    BOOL success = TRUE;
    STRPTR path, comment;
    struct module_entry *entry;

    /* Parse module line */

    path = strchr(line, '/');
    comment = strchr(line, '#');
    if (path == NULL)
        success = FALSE;

    /* Create module entry */

    if (success)
    {
        entry = AllocMem(sizeof(struct module_entry), MEMF_CLEAR);
        if (entry == NULL)
            success = FALSE;
    }

    if (success)
    {
        path[strlen(path) - 1] = '\0';
        entry->path = AllocVec(strlen(path) + 4, MEMF_ANY);
        if (entry->path == NULL)
            success = FALSE;
    }

    if (success)
    {
        sprintf(entry->path, "SYS:%s", path + 1);

        /* Mark module inactive if there's a comment character before it */

        entry->active = comment == NULL || comment > path;
    }

    /* Add module to list */

    if (success)
    {
        if (DoMethod(data->module_list, MUIM_List_InsertSingle,
            (IPTR)entry, MUIV_List_Insert_Bottom) == -1)
            success = FALSE;
    }

    return success;
}

static BOOL WriteModule(Object *obj, BPTR file, struct module_entry *entry)
{
    BOOL success = TRUE;
    TEXT buffer[MAX_LINE_LENGTH], *path = buffer, *p;
    BPTR lock, old_dir;
    Object *app = NULL, *window = NULL;

    /* Convert path to canonical form */

    old_dir = CurrentDir(BNULL);
    lock = Lock(entry->path, SHARED_LOCK);
    if (lock == BNULL)
    {
        GET(obj, MUIA_ApplicationObject, &app);
        GET(obj, MUIA_Window_Window, &window);

        MUI_Request(app, window, 0, "Error", _(MSG_OK), _(MSG_BAD_MODULE),
            entry->path);
        success = FALSE;
    }

    if (success)
    {
        if (!NameFromLock(lock, buffer, MAX_LINE_LENGTH))
            success = FALSE;
        UnLock(lock);
    }

    if (success)
    {
        if ((p = strchr(buffer, ':')) != NULL)
            path = p + 1;

        FPrintf(file, "    ");
        if (!entry->active)
            FPrintf(file, "#");
        FPrintf(file, "module /%s\n", path);
    }
    CurrentDir(old_dir);

    return success;
}

AROS_UFH3S(LONG, ModuleDisplayHook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(char **, array, A2),
    AROS_UFHA(struct module_entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    if (entry != NULL)
    {
        *array++ = entry->active ? "*" : "";
        *array = entry->path;
    }
    else
    {
        *array++ = (STRPTR)_(MSG_ACTIVE);
        *array = (STRPTR)_(MSG_PATH);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

ZUNE_CUSTOMCLASS_10
(
    BootEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                         struct opSet *,
    MUIM_Setup,                     Msg,
    MUIM_Cleanup,                   Msg,
    MUIM_PrefsEditor_ImportFH,      struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,      struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_Save,          Msg,
    MUIM_BootEditor_ShowModule,     Msg,
    MUIM_BootEditor_UpdateModule,   Msg,
    MUIM_BootEditor_AddModule,      Msg,
    MUIM_BootEditor_RemoveModule,   Msg
);
