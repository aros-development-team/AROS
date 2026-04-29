/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/label.h
*/

#ifndef IMAGES_LABEL_H
#define IMAGES_LABEL_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif
#ifndef INTUITION_IMAGECLASS_H
#include <intuition/imageclass.h>
#endif

#define LABEL_CLASSNAME     "label.image"
#define LABEL_VERSION       44

/* Justification modes for LABEL_Justification */
#define LJ_LEFT     0
#define LJ_CENTRE   1
#define LJ_RIGHT    2
#define LJ_CENTER   LJ_CENTRE   /* Alternate spelling */

/* Obsolete defines - do not use */
#define LABEL_LEFT      LJ_LEFT
#define LABEL_CENTRE    LJ_CENTRE
#define LABEL_CENTER    LJ_CENTRE
#define LABEL_RIGHT     LJ_RIGHT

#define LABEL_Dummy         (REACTION_Dummy + 0x0006000)

#define LABEL_DrawInfo          SYSIA_DrawInfo       /* DrawInfo for rendering */
#define LABEL_Text              (LABEL_Dummy+1)      /* (STRPTR) Label text */
#define LABEL_Image             (LABEL_Dummy+2)      /* (struct Image *) Label image */
#define LABEL_Mapping           (LABEL_Dummy+3)      /* (UWORD *) Pen mapping for next image */
#define LABEL_Justification     (LABEL_Dummy+4)      /* (UWORD) Justification mode */
#define LABEL_Key               (LABEL_Dummy+5)      /* (UWORD) (OM_GET) Underscore key */
#define LABEL_Underscore        (LABEL_Dummy+6)      /* (UBYTE) Underscore char, default '_' */
#define LABEL_DisposeImage      (LABEL_Dummy+7)      /* (BOOL) Free image on dispose */
#define LABEL_SoftStyle         (LABEL_Dummy+8)      /* (UBYTE) Text soft style */
#define LABEL_VerticalSpacing   (LABEL_Dummy+9)      /* (UWORD) Vertical spacing, default 0 */

#ifndef LabelObject
#define LabelObject     NewObject(NULL, LABEL_CLASSNAME
#endif
#ifndef LabelEnd
#define LabelEnd        TAG_END)
#endif

#endif /* IMAGES_LABEL_H */
