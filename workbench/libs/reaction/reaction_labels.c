/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Label list and listbrowser helpers of the reaction link library
*/

#include <exec/memory.h>
#include <exec/execbase.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/chooser.h>
#include <proto/clicktab.h>
#include <proto/radiobutton.h>
#include <proto/listbrowser.h>
#include <gadgets/chooser.h>
#include <gadgets/clicktab.h>
#include <gadgets/radiobutton.h>
#include <gadgets/listbrowser.h>
#include <clib/reaction_lib_protos.h>

#include <stdarg.h>

/* the program's bases: libreaction.a links into it */
extern struct ExecBase *SysBase;
extern struct IntuitionBase *IntuitionBase;
extern struct Library *ChooserBase, *ClickTabBase, *RadioButtonBase, *ListBrowserBase;

enum { LBL_CHOOSER, LBL_CLICKTAB, LBL_RADIO, LBL_BROWSER };

static struct Node *alloc_label(int kind, STRPTR text, WORD num)
{
    switch (kind)
    {
    case LBL_CHOOSER:
        return AllocChooserNode(CNA_Text, (IPTR)text, TAG_DONE);
    case LBL_CLICKTAB:
        return AllocClickTabNode(TNA_Text, (IPTR)text, TNA_Number, num, TAG_DONE);
    case LBL_RADIO:
        return AllocRadioButtonNode(0, RBNA_Labels, (IPTR)text, TAG_DONE);
    default:
        return AllocListBrowserNode(1, LBNCA_Text, (IPTR)text, TAG_DONE);
    }
}

static void free_labels(int kind, struct List *list)
{
    struct Node *n;

    if (!list)
        return;
    while ((n = RemHead(list)))
    {
        switch (kind)
        {
        case LBL_CHOOSER:  FreeChooserNode(n); break;
        case LBL_CLICKTAB: FreeClickTabNode(n); break;
        case LBL_RADIO:    FreeRadioButtonNode(n); break;
        default:           FreeListBrowserNode(n); break;
        }
    }
    FreeVec(list);
}

static struct List *make_labels(int kind, STRPTR *labels)
{
    struct List *list = AllocVec(sizeof(struct List), MEMF_ANY);
    WORD i;

    if (!list)
        return NULL;
    NewList(list);
    for (i = 0; labels && labels[i]; i++)
    {
        struct Node *n = alloc_label(kind, labels[i], i);
        if (!n)
        {
            free_labels(kind, list);
            return NULL;
        }
        AddTail(list, n);
    }
    return list;
}

/* the varargs forms collect their NULL-terminated strings into an array */
static struct List *make_labels_va(int kind, STRPTR first, va_list ap)
{
    STRPTR buf[64];
    int n = 0;
    STRPTR s = first;

    while (s && n < 63)
    {
        buf[n++] = s;
        s = va_arg(ap, STRPTR);
    }
    buf[n] = NULL;
    return make_labels(kind, buf);
}

#define LABEL_FUNCS(Name, FreeName, kind) \
    struct List *Name##A(STRPTR *labels) { return make_labels(kind, labels); } \
    struct List *Name(STRPTR label1, ...) \
    { \
        struct List *l; \
        va_list ap; \
        va_start(ap, label1); \
        l = make_labels_va(kind, label1, ap); \
        va_end(ap); \
        return l; \
    } \
    void FreeName(struct List *list) { free_labels(kind, list); }

LABEL_FUNCS(ChooserLabels, FreeChooserLabels, LBL_CHOOSER)
LABEL_FUNCS(ClickTabs, FreeClickTabs, LBL_CLICKTAB)
LABEL_FUNCS(RadioButtons, FreeRadioButtons, LBL_RADIO)
LABEL_FUNCS(BrowserNodes, FreeBrowserNodes, LBL_BROWSER)

struct Node *LBAddNodeA(struct Gadget *lb, struct Window *w, struct Requester *r,
                        struct Node *n, struct TagItem *tags)
{
    return (struct Node *)DoGadgetMethod(lb, w, r, LBM_ADDNODE, NULL, (IPTR)n, (IPTR)tags);
}

struct Node *LBAddNode(struct Gadget *lb, struct Window *w, struct Requester *r,
                       struct Node *n, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE_AS(tag1, struct Node *)
    retval = LBAddNodeA(lb, w, r, n, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
}

ULONG LBEditNodeA(struct Gadget *lb, struct Window *w, struct Requester *r,
                  struct Node *n, struct TagItem *tags)
{
    return DoGadgetMethod(lb, w, r, LBM_EDITNODE, NULL, (IPTR)n, (IPTR)tags);
}

ULONG LBEditNode(struct Gadget *lb, struct Window *w, struct Requester *r,
                 struct Node *n, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE_AS(tag1, ULONG)
    retval = LBEditNodeA(lb, w, r, n, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
}

ULONG LBRemNode(struct Gadget *lb, struct Window *w, struct Requester *r, struct Node *n)
{
    return DoGadgetMethod(lb, w, r, LBM_REMNODE, NULL, (IPTR)n);
}
