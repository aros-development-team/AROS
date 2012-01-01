/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android keyboard hidd class.
    Lang: English.
*/

#include <aros/debug.h>
#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>
#include <hidd/keyboard.h>
#include <oop/oop.h>

#include <proto/utility.h>
#include <proto/oop.h>

#include <android/keycodes.h>

#include "agfx.h"
#include "agfx_keyboard.h"
#include "server.h"

/****************************************************************************************/

OOP_Object *AKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("AKbd::New()\n"));

    if (XSD(cl)->kbdhidd)
    	return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	struct kbd_data *data = OOP_INST_DATA(cl, o);
	struct TagItem *tag, *tstate = msg->attrList;

	while ((tag = NextTagItem(&tstate)))
    	{
	    ULONG idx;
	    
	    if (IS_HIDDKBD_ATTR(tag->ti_Tag, idx))
	    {
	    	switch (idx)
	    	{
	    	case aoHidd_Kbd_IrqHandler:
		    data->callback = (APTR)tag->ti_Data;
		    D(bug("Got callback %p\n", tag->ti_Data));
		    break;
			
		case aoHidd_Kbd_IrqHandlerData:
		    data->callbackdata = (APTR)tag->ti_Data;
		    D(bug("Got data %p\n", tag->ti_Data));
		    break;
		}
	    }
	}
	XSD(cl)->kbdhidd = data;
    }

    ReturnPtr("AKbd::New", OOP_Object *, o);
}

/****************************************************************************************/

VOID AKbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    EnterFunc(bug("[AKbd] Dispose()\n"));

    XSD(cl)->kbdhidd = NULL;
    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

/*
 * Raw key codes conversion table (Android->AROS). Currently tested only with
 * Sony Ericsson XPeria (running Android 2.1).
 * Please feel free to modify and improve.
 *
 * Note 1: Sony Ericsson XPeria have only Alt and Sym keys on their keyboard.
 *	   Sym is used by Android to pop up extra virtual keyboard. However, under AROS it
 *	   does not produce any input events, but serves instead as additional modifier.
 *	   Currently i don't know how to make it generating events. Wound be nice to use it
 *	   as RAmiga to make hotkeys working.
 * Note 2: Alt (Blue square) key on XPeria keyboard is meant to produce additional characters,
 *	   which are marked in blue. An appropriate keymap on AROS side is needed for this. The
 *	   key itself doesn't produce an input event, instead it triggers a mode when keys generate
 *	   a sequence of two codes: ALT then key itself.
 * Note 3: XPeria keyboard doesn't handle autorepeats. Every physical keypress generates two codes:
 *	   press then release.
 */
static const UWORD KeyTable[] =
{
    -1,			/* AKEYCODE_UNKNOWN		*/
    RAWKEY_LEFT,	/* AKEYCODE_SOFT_LEFT   	*/
    RAWKEY_RIGHT,	/* AKEYCODE_SOFT_RIGHT  	*/
    RAWKEY_HOME,	/* AKEYCODE_HOME		*/
    -1,			/* AKEYCODE_BACK		- Emulates LAmiga+M */
    -1,			/* AKEYCODE_CALL		- Green button (Ok on a requester? Shift+Enter? What?) */
    RAWKEY_ESCAPE,	/* AKEYCODE_ENDCALL		- Red button */
    RAWKEY_0,		/* AKEYCODE_0			*/
    RAWKEY_1,		/* AKEYCODE_1			*/
    RAWKEY_2,		/* AKEYCODE_2			*/
    RAWKEY_3,		/* AKEYCODE_3			*/
    RAWKEY_4,		/* AKEYCODE_4			*/
    RAWKEY_5,		/* AKEYCODE_5			*/
    RAWKEY_6,		/* AKEYCODE_6			*/
    RAWKEY_7,		/* AKEYCODE_7			*/
    RAWKEY_8,		/* AKEYCODE_8			*/
    RAWKEY_9,		/* AKEYCODE_9			*/
    RAWKEY_KP_MULTIPLY,	/* AKEYCODE_STAR        	*/
    RAWKEY_KP_DIVIDE,	/* AKEYCODE_POUND       	- perhaps not good */
    RAWKEY_UP,		/* AKEYCODE_DPAD_UP     	*/
    RAWKEY_DOWN,	/* AKEYCODE_DPAD_DOWN		*/
    RAWKEY_LEFT,	/* AKEYCODE_DPAD_LEFT		*/
    RAWKEY_RIGHT,	/* AKEYCODE_DPAD_RIGHT		*/
    RAWKEY_KP_ENTER,	/* AKEYCODE_DPAD_CENTER 	*/
    -1,			/* AKEYCODE_VOLUME_UP   	- There is no adequate Amiga equivalent */
    -1,			/* AKEYCODE_VOLUME_DOWN 						*/
    -1,			/* AKEYCODE_POWER       						*/
    -1,			/* AKEYCODE_CAMERA      						*/
    RAWKEY_DELETE,	/* AKEYCODE_CLEAR       	*/
    RAWKEY_A,		/* AKEYCODE_A			*/
    RAWKEY_B,		/* AKEYCODE_B			*/
    RAWKEY_C,		/* AKEYCODE_C			*/
    RAWKEY_D,		/* AKEYCODE_D			*/
    RAWKEY_E,		/* AKEYCODE_E			*/
    RAWKEY_F,		/* AKEYCODE_F			*/
    RAWKEY_G,		/* AKEYCODE_G			*/
    RAWKEY_H,		/* AKEYCODE_H			*/
    RAWKEY_I,		/* AKEYCODE_I			*/
    RAWKEY_J,		/* AKEYCODE_J			*/
    RAWKEY_K,		/* AKEYCODE_K			*/
    RAWKEY_L,		/* AKEYCODE_L			*/
    RAWKEY_M,		/* AKEYCODE_M			*/
    RAWKEY_N,		/* AKEYCODE_N			*/
    RAWKEY_O,		/* AKEYCODE_O			*/
    RAWKEY_P,		/* AKEYCODE_P			*/
    RAWKEY_Q,		/* AKEYCODE_Q			*/
    RAWKEY_R,		/* AKEYCODE_R			*/
    RAWKEY_S,		/* AKEYCODE_S			*/
    RAWKEY_T,		/* AKEYCODE_T			*/
    RAWKEY_U,		/* AKEYCODE_U			*/
    RAWKEY_V,		/* AKEYCODE_V			*/
    RAWKEY_W,		/* AKEYCODE_W			*/
    RAWKEY_X,		/* AKEYCODE_X			*/
    RAWKEY_Y,		/* AKEYCODE_Y			*/
    RAWKEY_Z,		/* AKEYCODE_Z			*/
    RAWKEY_COMMA,	/* AKEYCODE_COMMA       	*/
    RAWKEY_PERIOD,	/* AKEYCODE_PERIOD      	*/
    RAWKEY_LALT,	/* AKEYCODE_ALT_LEFT    	*/
    RAWKEY_RALT,	/* AKEYCODE_ALT_RIGHT   	*/
    RAWKEY_LSHIFT,	/* AKEYCODE_SHIFT_LEFT  	*/
    RAWKEY_RSHIFT,	/* AKEYCODE_SHIFT_RIGHT 	*/
    RAWKEY_TAB,		/* AKEYCODE_TAB         	*/
    RAWKEY_SPACE,	/* AKEYCODE_SPACE       	*/
    RAWKEY_RAMIGA,	/* AKEYCODE_SYM         	- See note 1 */
    -1,			/* AKEYCODE_EXPLORER    	- There's neither adequate Amiga equivalent,     */
    -1,			/* AKEYCODE_ENVELOPE    	  nor adequate reason for these keys to exist :) */
    RAWKEY_RETURN,	/* AKEYCODE_ENTER       	*/
    RAWKEY_BACKSPACE,	/* AKEYCODE_DEL         	- This key is actually marked as Backspace key */
    RAWKEY_TILDE,	/* AKEYCODE_GRAVE       	*/
    RAWKEY_MINUS,	/* AKEYCODE_MINUS       	*/
    RAWKEY_EQUAL,	/* AKEYCODE_EQUALS		*/
    RAWKEY_LBRACKET,	/* AKEYCODE_LEFT_BRACKET	*/
    RAWKEY_RBRACKET,	/* AKEYCODE_RIGHT_BRACKET	*/
    RAWKEY_BACKSLASH,	/* AKEYCODE_BACKSLASH   	*/
    RAWKEY_SEMICOLON,	/* AKEYCODE_SEMICOLON   	*/
    RAWKEY_QUOTE,	/* AKEYCODE_APOSTROPHE  	*/
    RAWKEY_SLASH,	/* AKEYCODE_SLASH       	*/
    -1,			/* AKEYCODE_AT    		*/
    RAWKEY_NUMLOCK,	/* AKEYCODE_NUM   		*/
    -1,			/* AKEYCODE_HEADSETHOOK 	*/
    -1,			/* AKEYCODE_FOCUS 		*/
    -1,			/* AKEYCODE_PLUS  		- Should be Shift + '=', however keymap-dependent. Not good... */
    -1,			/* AKEYCODE_MENU  		- Used for RMB emulation */
    -1,			/* AKEYCODE_NOTIFICATION	*/
    RAWKEY_HELP,	/* AKEYCODE_SEARCH		*/
    RAWKEY_MEDIA2,	/* AKEYCODE_MEDIA_PLAY_PAUSE	*/
    RAWKEY_MEDIA1,	/* AKEYCODE_MEDIA_STOP		*/
    RAWKEY_MEDIA4,	/* AKEYCODE_MEDIA_NEXT      	*/
    RAWKEY_MEDIA3,	/* AKEYCODE_MEDIA_PREVIOUS	*/
    RAWKEY_MEDIA5,	/* AKEYCODE_MEDIA_REWIND	*/
    RAWKEY_MEDIA4,	/* AKEYCODE_MEDIA_FAST_FORWARD	*/
    -1,			/* AKEYCODE_MUTE  		*/
    RAWKEY_PAGEUP,	/* AKEYCODE_PAGE_UP         	*/
    RAWKEY_PAGEDOWN,	/* AKEYCODE_PAGE_DOWN       	*/
    -1,			/* AKEYCODE_PICTSYMBOLS		*/
    RAWKEY_CAPSLOCK,	/* AKEYCODE_SWITCH_CHARSET	*/
    RAWKEY_F1,		/* AKEYCODE_BUTTON_A		*/
    RAWKEY_F2,		/* AKEYCODE_BUTTON_B		*/
    RAWKEY_F3,		/* AKEYCODE_BUTTON_C		*/
    RAWKEY_F4,		/* AKEYCODE_BUTTON_X		*/
    RAWKEY_F5,		/* AKEYCODE_BUTTON_Y        	*/
    RAWKEY_F6,		/* AKEYCODE_BUTTON_Z        	*/
    RAWKEY_LAMIGA,	/* AKEYCODE_BUTTON_L1       	- These four perhaps not good. Need a device with keyboard. */
    RAWKEY_RAMIGA,	/* AKEYCODE_BUTTON_R1       								    */
    RAWKEY_CONTROL,	/* AKEYCODE_BUTTON_L2									    */
    RAWKEY_CONTROL,	/* AKEYCODE_BUTTON_R2									    */
    -1,			/* AKEYCODE_BUTTON_THUMBL	- simply don't know. May be F7-F11? */
    -1,			/* AKEYCODE_BUTTON_THUMBR					    */
    -1,			/* AKEYCODE_BUTTON_START					    */
    -1,			/* AKEYCODE_BUTTON_SELECT					    */
    -1			/* AKEYCODE_BUTTON_MODE						    */
};

static inline void PostRawKey(struct kbd_data *data, UWORD code)
{
    data->callback(data->callbackdata, code);
}

void AKbd_ReportKey(struct kbd_data *data, struct KeyEvent *e)
{
    UWORD code;

    /* Skip unknown codes */
    if (e->code > AKEYCODE_BUTTON_MODE)
    	return;

    switch (e->code)
    {
    case AKEYCODE_BACK:
    	/* LAmiga + M (cycle screens) emulation */
    	if (e->flags & IECODE_UP_PREFIX)
    	{
    	    /* Release in reverse order */
    	    PostRawKey(data, RAWKEY_M | IECODE_UP_PREFIX);
    	    PostRawKey(data, RAWKEY_LAMIGA | IECODE_UP_PREFIX);
    	}
    	else
    	{
    	    PostRawKey(data, RAWKEY_LAMIGA);
    	    PostRawKey(data, RAWKEY_M);
    	}
    	break;

    default:
    	/* 1:1 mapping to Amiga raw key */
    	code = KeyTable[e->code];

    	/* Ignore the key if there's no assignment */
    	if (code != -1)
    	    PostRawKey(data, code | e->flags);
    }
}
