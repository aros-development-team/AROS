/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Varargs forms of the reaction class library functions
*/

#define NO_INLINE_STDARG
#include <exec/types.h>
#include <utility/tagitem.h>
#include <proto/chooser.h>
#include <proto/clicktab.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/radiobutton.h>
#include <proto/speedbar.h>

extern struct Library *ChooserBase, *ClickTabBase, *LayoutBase, *ListBrowserBase,
                      *RadioButtonBase, *SpeedBarBase;

#define TAGS_RET(rtype, call) \
    AROS_SLOWSTACKTAGS_PRE_AS(tag1, rtype) \
    retval = call; \
    AROS_SLOWSTACKTAGS_POST

#define TAGS_VOID(call) \
    AROS_NR_SLOWSTACKTAGS_PRE(tag1) \
    call; \
    AROS_NR_SLOWSTACKTAGS_POST

struct Node *AllocChooserNode(Tag tag1, ...)
{ TAGS_RET(struct Node *, AllocChooserNodeA(AROS_SLOWSTACKTAGS_ARG(tag1))) }
void SetChooserNodeAttrs(struct Node *node, Tag tag1, ...)
{ TAGS_VOID(SetChooserNodeAttrsA(node, AROS_SLOWSTACKTAGS_ARG(tag1))) }
void GetChooserNodeAttrs(struct Node *node, Tag tag1, ...)
{ TAGS_VOID(GetChooserNodeAttrsA(node, AROS_SLOWSTACKTAGS_ARG(tag1))) }

struct Node *AllocClickTabNode(Tag tag1, ...)
{ TAGS_RET(struct Node *, AllocClickTabNodeA(AROS_SLOWSTACKTAGS_ARG(tag1))) }
void SetClickTabNodeAttrs(struct Node *node, Tag tag1, ...)
{ TAGS_VOID(SetClickTabNodeAttrsA(node, AROS_SLOWSTACKTAGS_ARG(tag1))) }
void GetClickTabNodeAttrs(struct Node *node, Tag tag1, ...)
{ TAGS_VOID(GetClickTabNodeAttrsA(node, AROS_SLOWSTACKTAGS_ARG(tag1))) }

ULONG SetPageGadgetAttrs(struct Gadget *gadget, Object *object, struct Window *window,
                         struct Requester *requester, Tag tag1, ...)
{ TAGS_RET(ULONG, SetPageGadgetAttrsA(gadget, object, window, requester, AROS_SLOWSTACKTAGS_ARG(tag1))) }

struct Node *AllocListBrowserNode(UWORD columns, Tag tag1, ...)
{ TAGS_RET(struct Node *, AllocListBrowserNodeA(columns, AROS_SLOWSTACKTAGS_ARG(tag1))) }
void SetListBrowserNodeAttrs(struct Node *node, Tag tag1, ...)
{ TAGS_VOID(SetListBrowserNodeAttrsA(node, AROS_SLOWSTACKTAGS_ARG(tag1))) }
void GetListBrowserNodeAttrs(struct Node *node, Tag tag1, ...)
{ TAGS_VOID(GetListBrowserNodeAttrsA(node, AROS_SLOWSTACKTAGS_ARG(tag1))) }
struct ColumnInfo *AllocLBColumnInfo(UWORD columns, Tag tag1, ...)
{ TAGS_RET(struct ColumnInfo *, AllocLBColumnInfoA(columns, AROS_SLOWSTACKTAGS_ARG(tag1))) }
LONG SetLBColumnInfoAttrs(struct ColumnInfo *colinfo, Tag tag1, ...)
{ TAGS_RET(LONG, SetLBColumnInfoAttrsA(colinfo, AROS_SLOWSTACKTAGS_ARG(tag1))) }
LONG GetLBColumnInfoAttrs(struct ColumnInfo *colinfo, Tag tag1, ...)
{ TAGS_RET(LONG, GetLBColumnInfoAttrsA(colinfo, AROS_SLOWSTACKTAGS_ARG(tag1))) }

struct Node *AllocRadioButtonNode(UWORD columns, Tag tag1, ...)
{ TAGS_RET(struct Node *, AllocRadioButtonNodeA(columns, AROS_SLOWSTACKTAGS_ARG(tag1))) }
void SetRadioButtonNodeAttrs(struct Node *node, Tag tag1, ...)
{ TAGS_VOID(SetRadioButtonNodeAttrsA(node, AROS_SLOWSTACKTAGS_ARG(tag1))) }
void GetRadioButtonNodeAttrs(struct Node *node, Tag tag1, ...)
{ TAGS_VOID(GetRadioButtonNodeAttrsA(node, AROS_SLOWSTACKTAGS_ARG(tag1))) }

struct Node *AllocSpeedButtonNode(UWORD number, Tag tag1, ...)
{ TAGS_RET(struct Node *, AllocSpeedButtonNodeA(number, AROS_SLOWSTACKTAGS_ARG(tag1))) }
void SetSpeedButtonNodeAttrs(struct Node *node, Tag tag1, ...)
{ TAGS_VOID(SetSpeedButtonNodeAttrsA(node, AROS_SLOWSTACKTAGS_ARG(tag1))) }
void GetSpeedButtonNodeAttrs(struct Node *node, Tag tag1, ...)
{ TAGS_VOID(GetSpeedButtonNodeAttrsA(node, AROS_SLOWSTACKTAGS_ARG(tag1))) }
