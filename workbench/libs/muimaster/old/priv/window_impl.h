/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __WINDOW_IMPL_H__
#define __WINDOW_IMPL_H__

/*
 * low level stuff for window handling
 */

struct MUI_RenderInfo;
struct MUI_WindowData;


BOOL
_zune_renderinfo_setup (struct MUI_RenderInfo *mri);

void
_zune_renderinfo_cleanup (struct MUI_RenderInfo *mri);

void
_zune_renderinfo_show (struct MUI_RenderInfo *mri);

void
_zune_renderinfo_hide (struct MUI_RenderInfo *mri);

/* low level open */
BOOL
_zune_window_open (struct MUI_WindowData *data);

/* low level open */
void
_zune_window_close (struct MUI_WindowData *data);

/* low level resize */
void
_zune_window_resize (struct MUI_WindowData *data);

/* get default event mask */
ULONG
_zune_window_get_default_events (void);

/* change event mask */
void
_zune_window_change_events (struct MUI_WindowData *data);

#ifndef __AROS__

/* process an event got in this window */
gint
_zune_window_event(Object *win, GdkEvent *event);

#else

/* process a message got in this window */
void
_zune_window_message(struct IntuiMessage *msg);

#endif

void
_zune_focus_new (Object *obj);

void
_zune_focus_destroy (Object *obj);


#endif
