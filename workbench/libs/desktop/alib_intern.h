#ifndef _ALIB_INTERN_H
#    define _ALIB_INTERN_H

/*
   Copyright © 1995-2001, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#    ifndef EXEC_TYPES_H
#        include <exec/types.h>
#    endif
#    ifndef UTILITY_HOOKS_H
#        include <utility/hooks.h>
#    endif
#    ifndef AROS_SYSTEM_H
#        include <aros/system.h>
#    endif
#    ifndef AROS_ASMCALL_H
#        include <aros/asmcall.h>
#    endif

IPTR            CallHookPkt(struct Hook *hook, APTR object, APTR paramPacket);

#    ifndef AROS_SLOWCALLHOOKPKT
#        define CallHookPkt(h,o,p)                                   \
	    AROS_UFC3(IPTR, (((struct Hook *)(h))->h_Entry),    \
		AROS_UFHA(struct Hook *, h, A0),                \
		AROS_UFHA(APTR,          o, A2),                \
		AROS_UFHA(APTR,          p, A1)                 \
	    )
#    endif

#endif /* _ALIB_INTERN_H */
