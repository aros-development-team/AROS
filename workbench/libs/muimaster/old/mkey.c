/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#ifdef _AROS
#include <proto/intuition.h>
#endif

#include <zunepriv.h>
#include <key.h>
#include <prefs.h>
#include <muikey.h>
#include <Window.h>

/* translate a key event to a muikey value, depending on user settings.
 * Events may be eaten here, and the function returns FALSE.
 * Else the muikey is set and can be send to the event handlers.
 */
#warning FIXME: zune_key_translate
#if 0
BOOL
zune_key_translate (Object *win, GdkEventKey *event, ULONG *muikey)
{
    int i;
/* ignore CapsLock, ScrollLock, NumLock */
    static guint ign_modifiers = ~(GDK_MOD2_MASK | GDK_LOCK_MASK
				   | GDK_MOD4_MASK | GDK_MOD5_MASK);

    g_return_val_if_fail(event != NULL, FALSE);
    g_return_val_if_fail(muikey != NULL, FALSE);
    
    *muikey = MUIKEY_NONE;
    
/*  g_print("state %d\n", event->state); */

    for (i = 0; i < MUIKEY_COUNT; i++)
    {
	if ((((event->state & ign_modifiers) == __zprefs.muikeys[i].modifiers))
	    && (event->keyval == __zprefs.muikeys[i].key))
	{
	    if (i < MUIKEY_GADGET_NEXT)
	    {
		*muikey = i;
		return TRUE;
	    }
	    switch (i)
	    {
		case MUIKEY_GADGET_NEXT:
/*  g_print("gadget next\n"); */
		    set(win, MUIA_Window_ActiveObject,
			MUIV_Window_ActiveObject_Next);
	            return FALSE;
		case MUIKEY_GADGET_PREV:
		    set(win, MUIA_Window_ActiveObject,
			MUIV_Window_ActiveObject_Prev);
	            return FALSE;
		case MUIKEY_GADGET_OFF:
		    set(win, MUIA_Window_ActiveObject,
			MUIV_Window_ActiveObject_None);
	            return FALSE;
		case MUIKEY_WINDOW_CLOSE:
		    set(win, MUIA_Window_CloseRequest, TRUE);
	            return FALSE;
		case MUIKEY_WINDOW_NEXT:
		    g_print("MUIKEY_WINDOW_NEXT unimplemented yet.\n");
	            return FALSE;
		case MUIKEY_WINDOW_PREV:
		    g_print("MUIKEY_WINDOW_PREV unimplemented yet.\n");
	            return FALSE;
		case MUIKEY_HELP:
		    g_print("MUIKEY_HELP unimplemented yet.\n");
	            return FALSE;
		case MUIKEY_POPUP:
		    g_print("MUIKEY_POPUP unimplemented yet.\n");
	            return FALSE;

		default:
	    }
	}
    }

/*      g_print("keysym = |%s|\n",  */
/*  	    gdk_keyval_name(event->keyval)); */

    return TRUE;
}
#endif

BOOL
zune_keyspec_parse_modifier (ZuneKeySpec *spec, const char *str)
{
#warning FIXME: zune_keyspec_parse_modifier
#if 0
    if (!strcmp(str, "Shift"))
    {
	spec->modifiers |= GDK_SHIFT_MASK;
	return TRUE;
    }
    else if (!strcmp(str, "Control"))
    {
	spec->modifiers |= GDK_CONTROL_MASK;
	return TRUE;
    }
    else if (!strcmp(str, "Meta"))
    {
	spec->modifiers |= GDK_MOD1_MASK;
	return TRUE;
    }
    else if (!strcmp(str, "Alt"))
    {
	spec->modifiers |= GDK_MOD1_MASK;
	return TRUE;
    }
    else if (!strcmp(str, "Mod1"))
    {
	spec->modifiers |= GDK_MOD1_MASK;
	return TRUE;
    }
    else if (!strcmp(str, "Mod3"))
    {
	spec->modifiers |= GDK_MOD3_MASK;
	return TRUE;
    }
#endif
    return FALSE;
}


/* readable_hotkey is already set, init the other fields */
void
zune_keyspec_parse (ZuneKeySpec *spec)
{
#warning FIXME: zune_keyspec_parse
#if 0
    gchar **vec;
    int i;

    vec = g_strsplit(spec->readable_hotkey, " ", 10);
    for (i = 0; vec[i] != NULL; i++)
    {
	if (!zune_keyspec_parse_modifier(spec, vec[i]))
	{
/*  g_print("key = %s\n", vec[i]); */
	    spec->key = gdk_keyval_from_name(vec[i]);
	}
    }
    g_strfreev(vec);

    /* shift-tab bug ? */
    if (spec->modifiers & GDK_SHIFT_MASK
	&& spec->key == GDK_Tab)
	spec->key = GDK_ISO_Left_Tab;
#endif
}
