#ifndef CLIB_REACTION_LIB_PROTOS_H
#define CLIB_REACTION_LIB_PROTOS_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: reaction link library (libreaction.a) helper functions
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>

__BEGIN_DECLS

struct List *ChooserLabelsA(STRPTR *labels);
struct List *ChooserLabels(STRPTR label1, ...);
void FreeChooserLabels(struct List *list);

struct List *ClickTabsA(STRPTR *labels);
struct List *ClickTabs(STRPTR label1, ...);
void FreeClickTabs(struct List *list);

struct List *RadioButtonsA(STRPTR *labels);
struct List *RadioButtons(STRPTR label1, ...);
void FreeRadioButtons(struct List *list);

struct List *BrowserNodesA(STRPTR *labels);
struct List *BrowserNodes(STRPTR label1, ...);
void FreeBrowserNodes(struct List *list);

struct Node *LBAddNodeA(struct Gadget *lb, struct Window *w, struct Requester *r,
                        struct Node *n, struct TagItem *tags);
struct Node *LBAddNode(struct Gadget *lb, struct Window *w, struct Requester *r,
                       struct Node *n, Tag tag1, ...);
ULONG LBEditNodeA(struct Gadget *lb, struct Window *w, struct Requester *r,
                  struct Node *n, struct TagItem *tags);
ULONG LBEditNode(struct Gadget *lb, struct Window *w, struct Requester *r,
                 struct Node *n, Tag tag1, ...);
ULONG LBRemNode(struct Gadget *lb, struct Window *w, struct Requester *r, struct Node *n);

__END_DECLS

#endif /* CLIB_REACTION_LIB_PROTOS_H */
