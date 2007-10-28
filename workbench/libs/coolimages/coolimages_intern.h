/*
    Copyright © 2002-2007, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef COOLIMAGES_INTERN_H
#define COOLIMAGES_INTERN_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_MEMORY_H
#   include <exec/memory.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef CYBERGRAPHX_CYBERGRAPHICS_H
#   include <cybergraphx/cybergraphics.h>
#endif
#ifndef LINKLIBS_COOLIMAGES_H
#   include <linklibs/coolimages.h>
#endif
#ifndef LIBRARIES_COOLIMAGES_H
#   include <libraries/coolimages.h>
#endif
#ifndef UTILITY_UTILITY_H
#   include <utility/utility.h>
#endif



/****************************************************************************************/

struct CoolImagesBase_intern
{
    struct Library		library;
};

/****************************************************************************************/

#undef CIB
#define CIB(b)	((struct CoolImagesBase_intern *)(b))

/****************************************************************************************/

#endif /* COOLIMAGES_INTERN_H */
