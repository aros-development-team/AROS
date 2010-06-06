/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: aros_visual_functions.h 31758 2009-09-02 17:49:52Z deadwood $
*/

#if !defined(AROS_VISUAL_FUNCTIONS_H)
#define AROS_VISUAL_FUNCTIONS_H

#include <GL/gl.h>
#include "arosmesa_internal.h"

AROSMesaVisual aros_new_visual(struct TagItem *tagList);
void _aros_destroy_visual(AROSMesaVisual aros_vis);

#endif /* AROS_VISUAL_FUNCTIONS_H */
