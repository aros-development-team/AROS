#ifndef INPUTHANDLER_ACTIONS_H
#define INPUTHANDLER_ACTIONS_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

struct IIHData;

#define LOCK_REFRESH(x)     LockLayerInfo(&(x)->LayerInfo)
#define UNLOCK_REFRESH(x)   UnlockLayerInfo(&(x)->LayerInfo)

struct IntuiActionMsg
{
    struct MinNode    node;
    struct Task      *task;
    void    	    (*handler)(struct IntuiActionMsg *, struct IntuitionBase *);
    LONG    	      done;
};

void HandleIntuiActions(struct IIHData *iihdata, struct IntuitionBase *IntuitionBase);

void DoSyncAction(void (*)(struct IntuiActionMsg *, struct IntuitionBase *),
                  struct IntuiActionMsg *,
                  struct IntuitionBase *IntuitionBase);
BOOL DoASyncAction(void (*)(struct IntuiActionMsg *, struct IntuitionBase *),
                   struct IntuiActionMsg *, ULONG size,
                   struct IntuitionBase *IntuitionBase);

void CheckLayers(struct Screen *screen, struct IntuitionBase *IntuitionBase);

void DoMoveSizeWindow(struct Window *targetwindow, LONG NewLeftEdge, LONG NewTopEdge,
                      LONG NewWidth, LONG NewHeight, BOOL send_newsize,
                      struct IntuitionBase *IntuitionBase);

void WindowSizeHasChanged(struct Window *targetwindow, WORD dx, WORD dy,
                                 BOOL is_sizewindow, struct IntuitionBase *IntuitionBase);
void WindowSizeWillChange(struct Window *targetwindow, WORD dx, WORD dy,
                                 struct IntuitionBase *IntuitionBase);


#endif /* INPUTHANDLER_ACTIONS_H */
