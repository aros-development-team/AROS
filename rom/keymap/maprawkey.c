/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/arossupport.h>
#include <devices/inputevent.h>
#include <devices/keymap.h>
#include "keymap_intern.h"

#include <aros/debug.h>



/*****************************************************************************

    NAME */
#include <clib/keymap_protos.h>

	AROS_LH4(WORD, MapRawKey,

/*  SYNOPSIS */
	AROS_LHA(struct InputEvent *, event, A0),
	AROS_LHA(STRPTR             , buffer, A1),
	AROS_LHA(LONG               , length, D1),
	AROS_LHA(struct KeyMap     *, keyMap, A2),

/*  LOCATION */
	struct Library *, KeymapBase, 7, Keymap)

/*  FUNCTION
	Converts possibly linked IECLASS_RAWKEY events to
	ANSI bytes.
    INPUTS
        event - InputEvent that should be converted.
          	ie_NextEvent can point ot other events.
          
        buffer - buffer into which the mapped ANSI bytes will be put.
        
        length - length of buffer.
        
        keymap - keymap to use for mapping. If NULL, then the default
           	 keymap will be used.
    

    RESULT
    	Actual number of chars written to the buffer. A return value of
    	-1 means buffer owerflow.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    keymap_lib.fd and clib/keymap_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,KeymapBase)
    
    UWORD code, qual;
    struct BufInfo bufinfo;
    
    bufinfo.Buffer	 = buffer;
    bufinfo.BufLength	 = length;
    bufinfo.CharsWritten = 0L;

    D(bug("MapRawKey(ie=%p, buf=%p, len=%d, keymap=%p)\n",
    	event, buffer, length, keyMap));
    	
    if (!keyMap)
    	keyMap = KMBase(KeymapBase)->DefaultKeymap;
    
    for ( ;event; event = event->ie_NextEvent )
    {
    	D(bug("Handlig event %p\n", event));
    	
        /* Don't handle non-rawkey events */
    	if (event->ie_Class != IECLASS_RAWKEY)
    	    continue;


    	code = event->ie_Code & 0x00FF; /* Make sure upper byte is cleared */
    	qual = event->ie_Qualifier;

    	D(bug("event is IECLASS_RAWKEY, code=%04x, qual=%04x\n",
    		code, qual));
    	    
    	if (!(code & IECODE_UP_PREFIX)) /* Key pressed ? */
    	{
    	    UBYTE kcf_qual = KC_NOQUAL; /* KCF_NOQUAL == 0 */
    	    IPTR key_mapping;
    	    UBYTE key_maptype;
    	    BYTE capsable;
    	    BYTE repeatable;
    	    
    	    D(bug("Keypress\n"));
    	    
    	    code &= ~IECODE_UP_PREFIX;
    	    
    	    /* Convert from IEQUAL_xxx into KCF_xx */
    	    if (qual & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))
    	    	kcf_qual |= KCF_SHIFT;
    	    	
    	    if (qual & (IEQUALIFIER_LALT|IEQUALIFIER_RALT))
    	    	kcf_qual |= KCF_ALT;
    	    	
    	    if (qual & IEQUALIFIER_CONTROL)
    	    	kcf_qual |= KCF_CONTROL;
    	    	
    	    D(bug("KCF qualifiers: %d\n", kcf_qual));
    	    	
    	    /* Get the type of the key */
    	    if (code <= 0x3F)
    	    {
    	    	/* Get key info from low keymap */
    	    	key_maptype = keyMap->km_LoKeyMapTypes[code];
    	    	key_mapping = keyMap->km_LoKeyMap[code];
    	    	capsable    = GetBitProperty(keyMap->km_LoCapsable,   code);
    	    	repeatable  = GetBitProperty(keyMap->km_LoRepeatable, code);
    	    	
    	    	D(bug("Lowkey\n"));
    	    } 
    	    else
    	    {
    	    	/* Get key info from high keymap */
    	    	key_maptype = keyMap->km_HiKeyMapTypes[code];
    	    	key_mapping = keyMap->km_HiKeyMap[code];
    	    	capsable    = GetBitProperty(keyMap->km_LoCapsable,   code);
    	    	repeatable  = GetBitProperty(keyMap->km_LoRepeatable, code);
    	    	D(bug("Hikey\n"));
    	    }
    	    
    	    if ((qual & IEQUALIFIER_CAPSLOCK) && (!capsable))
    	    	continue; /* Capslock not supported for key, skip keypress */
    	    else
    	    	kcf_qual |= KCF_SHIFT;
    	    	
    	    D(bug("caps test passed\n"));
    	    
    	    if ((qual & IEQUALIFIER_REPEAT) && (!repeatable))
    	    	continue; /* Repeating not supported for key, skip keypress */
    	    	
    	    D(bug("repeat test passed\n"));
    	    	
    	    /* Handle decoding of the the different keytypes (normal, KCF_STRING, KCF_DEAD and KCF_NOP) */
    	    
    	    switch (key_maptype & (KC_NOQUAL|KCF_STRING|KCF_DEAD|KCF_NOP))
    	    {
    	    case KC_NOQUAL: {
    	        BYTE idx;
    	        UBYTE c;
    	        
    	        D(bug("KC_NOQUAL keymap type\n"));
    	            
    	        idx = keymaptype_table[key_maptype & KC_VANILLA][kcf_qual];
    	        if (idx != -1)
    	        {
    	            D(bug("Valid key qualifiers\n"));
    	            if (idx == -2)
    	            {
    	            	/* Special-case where bit 5 & 6 should be cleared */
    	            	idx = 3;

    	                c = (key_mapping >> (idx * 8)) & 0x000000FF;
    	                
			/* clear bit 5 and 5 */
    	                c &= ~((1 << 5)|(1 << 6));
    	            }
    	            else
    	            {
    	                c = (key_mapping >> (idx * 8)) & 0x000000FF;
    	            }
    	            
    	            D(bug("Writing to buffer\n"));
    	        
    	            if (!WriteToBuffer(&bufinfo, &c, 1))
    	            	goto overflow;
    	            	
    	        }
    	    } break;
    	        
    	    case KCF_STRING:
    	        break;
    	        
    	    case KCF_DEAD:
    	        break;
    	        
    	    case KCF_NOP:
    	        continue;
    	        
    	    default:
    	    	kprintf("Error in keymap, more than one decode action specified !\n");
    	    	break;
    	        
    	    }
    	    
    	}
    	else /* Key released */
    	{
    	}
    	
    }
    
    
    ReturnInt ("MapRawKey", WORD, bufinfo.CharsWritten);
    
overflow:
    ReturnInt ("MapRawKey", WORD, -1);

    AROS_LIBFUNC_EXIT
} /* MapRawKey */
