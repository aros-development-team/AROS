#ifndef GADGETS_GRADIENTSLIDER_H
#define GADGETS_GRADIENTSLIDER_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: MethodIDs and AttrIDs for the gradientslider class.
    Lang: english
*/

#ifndef UTILITY_TAGITEM_H
#    include <utility/tagitem.h>
#endif

#define GRAD_Dummy	(TAG_USER + 0x05000000)
#define GRAD_MaxVal	(GRAD_Dummy + 1)     /* slider's max value	   		*/
#define GRAD_CurVal	(GRAD_Dummy + 2)     /* slider's current value	  		*/
#define GRAD_SkipVal	(GRAD_Dummy + 3)     /* move amount of "body click" move amount */
#define GRAD_KnobPixels (GRAD_Dummy + 4)     /* knob size	   			*/
#define GRAD_PenArray	(GRAD_Dummy + 5)     /* pen colors		   		*/


#endif /* GADGETS_GRADIENTSLIDER_H */
