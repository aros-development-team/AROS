/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible reaction/reaction_macros.h
          Convenience macros for ReAction/ClassAct programming
*/

#ifndef REACTION_REACTION_MACROS_H
#define REACTION_REACTION_MACROS_H

#ifndef INTUITION_CLASSUSR_H
#include <intuition/classusr.h>
#endif

/*
 * Convenience object creation macros matching
 * the ClassAct/ReAction programming style.
 */

/* Generic end */
#ifndef End
#define End     TAG_END)
#endif

#ifndef EndObject
#define EndObject   TAG_END)
#endif

/* Shortcut macros for common gadget creation patterns */

/* Button macros */
#define Button(text, id) \
    ButtonObject, \
        GA_ID, id, \
        GA_RelVerify, TRUE, \
        GA_Text, text

#define PushButton(text, id) \
    ButtonObject, \
        GA_ID, id, \
        GA_RelVerify, TRUE, \
        GA_Text, text

/* Toggle button */
#define ToggleObject \
    ButtonObject, \
        BUTTON_Pushed, FALSE

/* Label */
#define Label(text) \
    CHILD_Label, LabelObject, \
        LABEL_Text, text, \
    LabelEnd

/* String gadget with label */
#define String(text, id, maxchars) \
    StringObject, \
        GA_ID, id, \
        GA_RelVerify, TRUE, \
        GA_TabCycle, TRUE, \
        STRINGA_MaxChars, maxchars, \
        STRINGA_TextVal, text

#define LabelString(text, id, maxchars) \
    LAYOUT_AddChild, String(text, id, maxchars), \
    StringEnd, \
    Label(text)

/* Text line (read-only string) */
#define TextLine(text) \
    StringObject, \
        GA_ReadOnly, TRUE, \
        STRINGA_TextVal, text

#define LabelTextLine(label, text) \
    LAYOUT_AddChild, TextLine(text), \
    StringEnd, \
    Label(label)

/* PopString (getfile with string) */
#define PopString(text, id) \
    GetFileObject, \
        GA_ID, id, \
        GA_RelVerify, TRUE, \
        GETFILE_FullFile, text

/* Slider macro */
#define Slider(min, max, level) \
    SliderObject, \
        SLIDER_Min, min, \
        SLIDER_Max, max, \
        SLIDER_Level, level, \
        SLIDER_Orientation, SLIDER_HORIZONTAL, \
        SLIDER_LevelFormat, "%ld", \
        SLIDER_LevelPlace, PLACETEXT_IN

/* Cycle (Chooser) macro */
#define Cycle(labellist, id) \
    ChooserObject, \
        GA_ID, id, \
        CHOOSER_LabelArray, labellist, \
        CHOOSER_Selected, 0

/* Convenience group start macros */
#define VGroup  VLayoutObject, LAYOUT_SpaceOuter, TRUE, LAYOUT_SpaceInner, TRUE
#define HGroup  HLayoutObject, LAYOUT_SpaceOuter, TRUE, LAYOUT_SpaceInner, TRUE

/* Start/End named groups */
#define StartVGroup VLayoutObject
#define StartHGroup HLayoutObject

#endif /* REACTION_REACTION_MACROS_H */
