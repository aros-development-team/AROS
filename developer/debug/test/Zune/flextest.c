/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Test program for Flexgroup.mui.

    The upper group is a Flexgroup driven by the controls above it.  The
    lower one holds equivalent children in a plain HGroup, for comparison.
*/

#include <exec/types.h>
#include <stdio.h>

#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#include <libraries/mui.h>

#define NUM_CHILDREN 4

static const char *dir_entries[] = { "row", "column", NULL };
static const char *justify_entries[] =
{
    "start", "end", "center", "space-between", "space-around",
    "space-evenly", NULL
};
static const char *align_entries[] =
{
    "stretch", "start", "end", "center", NULL
};
static const char *weight_entries[] =
{
    "0 - all fixed", "100 - all equal", "0,1,2,0 - graded", NULL
};

/* Weight along the main axis is flex-grow/flex-shrink.  With every child
   at 0 there is surplus for justify-content to distribute; as soon as a
   child grows it eats the surplus and justify-content goes quiet. */
static const WORD weight_sets[3][NUM_CHILDREN] =
{
    { 0, 0, 0, 0 },
    { 100, 100, 100, 100 },
    { 0, 100, 200, 0 },
};

static Object *flexobj, *dir_cycle, *weight_cycle;
static Object *children[NUM_CHILDREN];
static struct Hook weighthook;

static void WeightFunc(struct Hook *hook, Object *obj, APTR msg)
{
    LONG sel = XGET(weight_cycle, MUIA_Cycle_Active);
    BOOL row = (XGET(dir_cycle, MUIA_Cycle_Active)
        == MUIV_Flexgroup_Direction_Row);
    LONG i;

    DoMethod(flexobj, MUIM_Group_InitChange);
    for (i = 0; i < NUM_CHILDREN; i++)
    {
        set(children[i], row ? MUIA_HorizWeight : MUIA_VertWeight,
            weight_sets[sel][i]);
    }
    DoMethod(flexobj, MUIM_Group_ExitChange);
}

static Object *MakeChild(const char *label, LONG fixheight)
{
    return TextObject,
        ButtonFrame,
        MUIA_Background, MUII_ButtonBack,
        MUIA_Text_Contents, (IPTR)label,
        MUIA_Text_PreParse, (IPTR)"\33c",
        MUIA_Text_SetVMax, FALSE,
        MUIA_HorizWeight, 0,
        MUIA_VertWeight, 0,
        fixheight ? MUIA_FixHeight : TAG_IGNORE, fixheight,
    End;
}

int main(void)
{
    Object *application, *window;
    Object *justify_cycle, *align_cycle, *gap_slider;
    LONG i;

    children[0] = MakeChild("One", 0);
    children[1] = MakeChild("Two", 0);
    children[2] = MakeChild("Three is wider", 0);
    children[3] = MakeChild("Tall", 60);

    application = ApplicationObject,
        SubWindow, window = WindowObject,
            MUIA_Window_Title, (IPTR)"Flexgroup.mui",
            MUIA_Window_Activate, TRUE,

            WindowContents, (IPTR)VGroup,

                Child, (IPTR)ColGroup(2),
                    GroupFrameT("Container"),
                    Child, (IPTR)Label("flex-direction:"),
                    Child, (IPTR)(dir_cycle = CycleObject,
                        MUIA_Cycle_Entries, (IPTR)dir_entries,
                    End),
                    Child, (IPTR)Label("justify-content:"),
                    Child, (IPTR)(justify_cycle = CycleObject,
                        MUIA_Cycle_Entries, (IPTR)justify_entries,
                    End),
                    Child, (IPTR)Label("align-items:"),
                    Child, (IPTR)(align_cycle = CycleObject,
                        MUIA_Cycle_Entries, (IPTR)align_entries,
                    End),
                    Child, (IPTR)Label("gap:"),
                    Child, (IPTR)(gap_slider = SliderObject,
                        MUIA_Numeric_Min, 0,
                        MUIA_Numeric_Max, 32,
                        MUIA_Numeric_Value, 4,
                    End),
                    Child, (IPTR)Label("child weight (= flex-grow):"),
                    Child, (IPTR)(weight_cycle = CycleObject,
                        MUIA_Cycle_Entries, (IPTR)weight_entries,
                    End),
                End,

                Child, (IPTR)(flexobj = MUI_NewObject(MUIC_Flexgroup,
                    GroupFrameT("Flexgroup"),
                    MUIA_Flexgroup_Gap, 4,
                    MUIA_Group_Child, (IPTR)children[0],
                    MUIA_Group_Child, (IPTR)children[1],
                    MUIA_Group_Child, (IPTR)children[2],
                    MUIA_Group_Child, (IPTR)children[3],
                    TAG_DONE)),

                Child, (IPTR)HGroup,
                    GroupFrameT("Plain HGroup, same children"),
                    Child, (IPTR)MakeChild("One", 0),
                    Child, (IPTR)MakeChild("Two", 0),
                    Child, (IPTR)MakeChild("Three is wider", 0),
                    Child, (IPTR)MakeChild("Tall", 60),
                End,

            End,
        End,
    End;

    if (application == NULL)
    {
        printf("Could not create application\n");
        return RETURN_FAIL;
    }

    weighthook.h_Entry = (HOOKFUNC)HookEntry;
    weighthook.h_SubEntry = (HOOKFUNC)WeightFunc;

    DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR)application, 2, MUIM_Application_ReturnID,
        MUIV_Application_ReturnID_Quit);

    DoMethod(dir_cycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
        (IPTR)flexobj, 3, MUIM_Set, MUIA_Flexgroup_Direction,
        MUIV_TriggerValue);
    DoMethod(justify_cycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
        (IPTR)flexobj, 3, MUIM_Set, MUIA_Flexgroup_Justify,
        MUIV_TriggerValue);
    DoMethod(align_cycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
        (IPTR)flexobj, 3, MUIM_Set, MUIA_Flexgroup_Align,
        MUIV_TriggerValue);
    DoMethod(gap_slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        (IPTR)flexobj, 3, MUIM_Set, MUIA_Flexgroup_Gap, MUIV_TriggerValue);
    DoMethod(weight_cycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
        (IPTR)application, 2, MUIM_CallHook, (IPTR)&weighthook);

    set(window, MUIA_Window_Open, TRUE);

    {
        ULONG sigs = 0;

        while (DoMethod(application, MUIM_Application_NewInput, (IPTR)&sigs)
            != MUIV_Application_ReturnID_Quit)
        {
            if (sigs)
            {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
                if (sigs & SIGBREAKF_CTRL_C)
                    break;
                if (sigs & SIGBREAKF_CTRL_D)
                    break;
            }
        }
    }

    MUI_DisposeObject(application);

    return RETURN_OK;
}
