/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible classes/requester.h
*/

#ifndef CLASSES_REQUESTER_H
#define CLASSES_REQUESTER_H

/*****************************************************************************/

#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

/*****************************************************************************/

/* Attributes defined by the requester.gadget class */
#define REQ_Dummy               (REACTION_Dummy + 0x45000)
#define REQS_Dummy              (REQ_Dummy + 0x100)
#define REQI_Dummy              (REQ_Dummy + 0x200)
#define REQP_Dummy              (REQ_Dummy + 0x300)

#define REQ_Type                (REQ_Dummy+1)   /* ULONG — requester type (see REQTYPE_*) */

#define REQ_TitleText           (REQ_Dummy+2)   /* UBYTE * — requester window title */

#define REQ_BodyText            (REQ_Dummy+3)   /* UBYTE * — body text content */

#define REQ_GadgetText          (REQ_Dummy+4)   /* UBYTE * — button labels separated by '|' */

#define REQ_ReturnCode          (REQ_Dummy+5)   /* ULONG — result from last RM_OPENREQ (get only) */

#define REQ_TabSize             (REQ_Dummy+6)   /* ULONG — tab stop width for body text */

#define REQ_Image               (REQ_Dummy+7)   /* ULONG — predefined image type (see REQIMAGE_*) */

/**********************************
* integer request type attributes *
**********************************/

#define REQI_Minimum            (REQI_Dummy+1)  /* LONG — lower bound */

#define REQI_Maximum            (REQI_Dummy+2)  /* LONG — upper bound */

#define REQI_Invisible          (REQI_Dummy+3)  /* BOOL — obscure input with dots */

#define REQI_Number             (REQI_Dummy+4)  /* LONG — initial numeric value */

#define REQI_Arrows             (REQI_Dummy+5)  /* BOOL — show increment/decrement arrows */

#define REQI_MaxChars           (REQI_Dummy+6)  /* UWORD — maximum digit count */

/*********************************
* string request type attributes *
*********************************/

#define REQS_AllowEmpty         (REQS_Dummy+1)  /* BOOL — accept empty string as valid */

#define REQS_Invisible          REQI_Invisible  /* BOOL — obscure input with dots */

#define REQS_Buffer             (REQS_Dummy+2)  /* UBYTE * — string buffer (required) */

#define REQS_ShowDefault        (REQS_Dummy+3)  /* BOOL — pre-fill from buffer contents */

#define REQS_MaxChars           (REQS_Dummy+4)  /* ULONG — buffer capacity */

#define REQS_ChooserArray       (REQS_Dummy+5)  /* UBYTE ** — string array for chooser */

#define REQS_ChooserActive      (REQS_Dummy+6)  /* ULONG — initially selected chooser entry */

/**********************************
* progress window type attributes *
**********************************/

#define REQP_Total              (REQP_Dummy+1)  /* ULONG — maximum progress value */

#define REQP_Current            (REQP_Dummy+2)  /* ULONG — current progress value */

#define REQP_AbortText          REQ_GadgetText  /* UBYTE * — abort button label */

#define REQP_ProgressText       REQ_BodyText    /* UBYTE * — text above progress bar */

#define REQP_OpenInactive       (REQP_Dummy+3)  /* BOOL — open without activating */

#define REQP_NoText             (REQP_Dummy+4)  /* BOOL — suppress text in progress bar */

#define REQP_Dynamic            (REQP_Dummy+5)  /* BOOL — auto-resize for long text */

#define REQP_CenterWindow       (REQP_Dummy+6)  /* struct Window * — center over this window */

#define REQP_LastPosition       (REQP_Dummy+7)  /* BOOL — reopen at previous position */

#define REQP_Percent            (REQP_Dummy+8)  /* BOOL — show numeric percentage */

#define REQP_Ticks              (REQP_Dummy+9)  /* WORD — number of tick marks */

#define REQP_ShortTicks         (REQP_Dummy+10) /* BOOL — show short intermediate ticks */

/*****************************************************************************/

/* requester.class methods */
#define RM_OPENREQ      (0x650001L)

/* RM_OPENREQ message */
struct orRequest
{
    ULONG MethodID;             /* method ID */
    struct TagItem *or_Attrs;   /* additional attributes */
    struct Window *or_Window;   /* parent window (optional if screen given) */
    struct Screen *or_Screen;   /* parent screen (required if no window) */
};

/* Requester types for REQ_Type */
#define REQTYPE_INFO        0   /* informational or yes/no query */
#define REQTYPE_INTEGER     1   /* integer input */
#define REQTYPE_STRING      2   /* string input */
#define REQTYPE_PROGRESS    3   /* progress display */

/* Predefined image types for REQ_Image */
#define REQIMAGE_DEFAULT    0   /* system-default image */
#define REQIMAGE_INFO       1   /* information icon */
#define REQIMAGE_WARNING    2   /* warning icon */
#define REQIMAGE_QUESTION   3   /* question mark icon */
#define REQIMAGE_ERROR      4   /* error/stop icon */
#define REQIMAGE_INSERTDISK 5   /* insert-disk icon */

/* Useful macros */
#ifndef RequesterObject
#define OpenRequester(obj, win) DoMethod(obj, RM_OPENREQ, NULL, win, NULL, TAG_DONE)
#define RequesterObject         NewObject(REQUESTER_GetClass(), NULL
#endif

#endif /* CLASSES_REQUESTER_H */
