/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _COMPILER_H
#define _COMPILER_H

#ifdef __MAXON__
#define __asm
#define __inline
#define __saveds
#define const
#endif /* __MAXON__ */

#ifndef __AROS__
#ifndef _AROS_TYPES_DEFINED
typedef unsigned long IPTR;
typedef long STACKLONG;
typedef unsigned long STACKULONG;
#define _AROS_TYPES_DEFINED
#endif /* _AROS_TYPES_DEFINED */
#endif /* __AROS__ */

#endif /* _COMPILER_H */
