/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdarg.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/muimaster.h>
extern struct Library * MUIMasterBase;

	Object * MUI_MakeObject (

/*  SYNOPSIS */
	LONG type, 
	...)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    va_list args;
    IPTR    param[4];
    WORD    i, numparams = 0;
        
    switch(type)
    {
	case 2: /* MUIO_Button */
	case 3: /* MUIO_CheckMark */
	case 8: /* MUIO_PopButton */
	case 9: /* MUIO_HSpace */
	case 10: /* MUIO_VSpace */
	case 11: /* MUIO_HBar */
	case 12: /* MUIO_VBar */
	case 15: /* MUIO_BarTitle */
    	    numparams = 1;
	    break;
	        
    	case 1: /* MUIO_Label */
	case 4: /* MUIO_Cycle */
	case 5: /* MUIO_Radio */
	case 7: /* MUIO_String */
	case 13: /* MUIO_MenustripNM */
	case 112: /* MUIO_ImageButton */
	    numparams = 2;
	    break;
	    
	case 6: /* MUIO_Slider */
	case 111: /* MUIO_CoolButton */
	    numparams = 3;
	    break;

	case 14: /* MUIO_MenuItem */
	case 16: /* MUIO_NumericButton */
    	    numparams = 4;
	    break;	    	    	
    }
    
    if (numparams == 0) return NULL;
    
    va_start(args, type);
    
    for(i = 0; i < numparams; i++)
    {
    	param[i] = va_arg(args, IPTR);
    }
    
    va_end (args);
    
    return MUI_MakeObjectA(type, param);
    
} /* MUI_MakeObject */
