#ifndef GADGETS_TAPEDECK_H
#define GADGETS_TAPEDECK_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: MethodIDs and AttrIDs for the tapedeck class.
    Lang: english
*/

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define TDECK_Dummy  	    (TAG_USER    | 0x5000000)
#define TDECK_Mode   	    (TDECK_Dummy + 0x1      )
#define TDECK_Paused 	    (TDECK_Dummy + 0x2      )
#define TDECK_Tape   	    (TDECK_Dummy + 0x3      )
#define TDECK_Frames 	    (TDECK_Dummy + 0xB      )
#define TDECK_CurrentFrame  (TDECK_Dummy + 0xC	    )

#define BUT_REWIND  	    0
#define BUT_PLAY    	    1
#define BUT_FORWARD 	    2
#define BUT_STOP    	    3
#define BUT_PAUSE   	    4
#define BUT_BEGIN   	    5
#define BUT_FRAME   	    6
#define BUT_END     	    7

#endif /* GADGETS_TAPEDECK_H */
