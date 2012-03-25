/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: keymap.library function MapRawKey()
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
        Converts IECLASS_RAWKEY events to ANSI bytes.
        The event list (event->ie_NextEvent) is not traversed!
        
    INPUTS
        event - InputEvent that should be converted.
                ie_NextEvent is ignored!
          
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
        MapAnsi()

    INTERNALS

    HISTORY
        27-11-96    digulla automatically created from
                            keymap_lib.fd and clib/keymap_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct BufInfo bufinfo;
    struct KeyInfo ki;
    UWORD          code, qual;
        
    bufinfo.Buffer       = buffer;
    bufinfo.BufLength    = length;
    bufinfo.CharsWritten = 0L;

    if (!keyMap)
        keyMap = KMBase(KeymapBase)->DefaultKeymap;
    
        
    /* Don't handle non-rawkey events */
    if (event->ie_Class != IECLASS_RAWKEY)
        goto done;

    code = event->ie_Code;
    qual = event->ie_Qualifier;

    /* Get info on keypress */
    if (!GetKeyInfo(&ki, code, qual, keyMap))
        goto done; /* Invalid key mapping (like CTRL-ALT-A if a doesn't support CTRL-ALT */

    /* Handle decoding of the the different keytypes (normal, KCF_STRING, KCF_DEAD and KCF_NOP) */

    switch (ki.Key_MapType & (KC_NOQUAL|KCF_STRING|KCF_DEAD|KCF_NOP))
    {
        case KC_NOQUAL:
        {
            BYTE idx;
            UBYTE c;

            D(bug("mrk: KC_NOQUAL\n"));

            D(bug("mrk: getting idx at [%d][%d]\n", ki.Key_MapType & KC_VANILLA, ki.KCFQual));
            idx = keymaptype_table[ki.Key_MapType & KC_VANILLA][ki.KCFQual];
            
            if (idx != -1)
            {
                D(bug("mrk: valid qual, idx=%d, key mapping=%04x\n", idx, ki.Key_Mapping));
                if (idx == -2)
                {
                    D(bug("mrk: Ctrl-C mode\n"));
                    /* Special-case where bit 5 & 6 should be cleared */
                    idx = 3;
                    c = GetMapChar(ki.Key_Mapping, idx);

                    /* clear bit 5 and 6 */
                    c &= ~((1 << 5)|(1 << 6));
                }
                else
                {
                     c = GetMapChar(ki.Key_Mapping, idx);
                }

                D(bug("mrk: Putting %c (%d 0x%x) into buffer\n", c, c, c));

                if (c != 0) /* If we get a 0 from the keymap, it means the char converts to "" */
                {
                    if (!WriteToBuffer(&bufinfo, &c, 1))
                        goto overflow;
                }

            } /* if (idx != -1) */
            break;
        }

        case KCF_STRING:
        {
            BYTE idx;

            D(bug("mrk: KCF_STRING\n"));
            
            D(bug("mrk: getting idx at [%d][%d]\n", ki.Key_MapType & KC_VANILLA, ki.KCFQual));
            idx = keymapstr_table[ki.Key_MapType & KC_VANILLA][ki.KCFQual];
            
            if (idx != -1)
            {
                UBYTE *str_descrs = (UBYTE *)ki.Key_Mapping;
                UBYTE len, offset;

                /* Since each string descriptor uses two bytes we multiply by 2 */
                idx *= 2;

                /* Get string info from string descriptor table */

                len    = str_descrs[idx];
                offset = str_descrs[idx + 1];

                D(bug("mrk: len=%d, offset=%d\n", len, offset));

                /* Write string to buffer */
                if (!WriteToBuffer(&bufinfo, &(str_descrs[offset]), len))
                    goto overflow;

            } /* if (idx != -1) */

            break;
        }

        case KCF_DEAD:
        {
            BYTE idx;

            /* Get the index to the right dead key descrptor */
            D(bug("mrk: KCF_DEAD\n"));
            
            D(bug("mrk: getting idx at [%d][%d]\n", ki.Key_MapType & KC_VANILLA, ki.KCFQual));
            idx = keymapstr_table[ki.Key_MapType & KC_VANILLA][ki.KCFQual];
            
            if (idx != -1)
            {

                UBYTE *dead_descr = (UBYTE *)ki.Key_Mapping;
                UBYTE dead_type;
                UBYTE dead_val;

                /* Each dead descripto is 2 bytes */
                idx *= 2;

                dead_type = dead_descr[idx];
                dead_val  = dead_descr[idx + 1];


                if (dead_type == 0)
                {
                    /* Val is the key to output */
                    if (!WriteToBuffer(&bufinfo, &dead_val, 1))
                        goto overflow;
                }
                else if (dead_type == DPF_DEAD)
                {
                    /* Do absolutely nothing. DPF_DEAD keys
                    ** are not possible to output, and are not
                    ** interesting by themselves.
                    ** However, if a DPF_MOD key follows..
                    */

                }
                else if (dead_type == DPF_MOD)
                {
                    /* Now, lets have a look at the last two previous
                    ** keypresses.
                    **
                    ** dk_idx defaults to 0, which is the index to use if it turns out that
                    ** the deadable key had no valid deadkeys before it
                    */
                    WORD dk_idx = 0; /* Defaults to 0, which is the index to be used into the transition table */
                    WORD dki_1;

                    /* Get deadkey index for Prev1 */   
                    dki_1 = GetDeadKeyIndex(event->ie_Prev1DownCode, event->ie_Prev1DownQual, keyMap);
                    if (dki_1 != -1) /* Was it a dead key ? */
                    {
                        dk_idx = dki_1;
                        
                        /* Is this a double deadkey (higher nibble set ?) */
                        if (dki_1 >> DP_2DFACSHIFT)
                        {
                            WORD dki_2;

                            dki_2 = GetDeadKeyIndex(event->ie_Prev2DownCode, event->ie_Prev2DownQual, keyMap);
                            if (dki_2 != -1) /* Was it a dead key ? */
                            {
                                /* Compute deadkey index - explained in RKM:L p. 826 */
                                dk_idx = (dki_1 & DP_2DINDEXMASK) * (dki_1 >> DP_2DFACSHIFT) + (dki_2 & DP_2DINDEXMASK);
                            }
                        }
                    }

                    dead_val = dead_descr[dead_val + dk_idx];

                    if (!WriteToBuffer(&bufinfo, &dead_val, 1))
                        goto overflow;

                }
                else
                {
                    D(bug("Keymap contains illegal deadkey type for code %04x, event->ie_Code\n"));
                }


            } /* if (idx != -1) */

            break;
        }

        case KCF_NOP:
            break;

        default:
            D(bug("Error in keymap, more than one decode action specified for code %04x\n", event->ie_Code));
            break;

    } /* switch (ki.Key_MapType & (KC_NOQUAL|KCF_STRING|KCF_DEAD|KCF_NOP)) */

done:                   
    ReturnInt ("MapRawKey", WORD, bufinfo.CharsWritten);
    
overflow:
    ReturnInt ("MapRawKey", WORD, -1);

    AROS_LIBFUNC_EXIT
} /* MapRawKey */
