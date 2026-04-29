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

#define LABEL_CLASSNAME     "label.image"
#define LABEL_VERSION       44

#define LABEL_Dummy         (REACTION_Dummy + 0x0006000)

#define LABEL_Text              (LABEL_Dummy + 0x0001) /* Label string */
#define LABEL_Image             (LABEL_Dummy + 0x0002) /* Inline image */
#define LABEL_Justification     (LABEL_Dummy + 0x0003) /* Text alignment */
#define LABEL_SoftStyle         (LABEL_Dummy + 0x0004) /* Text style flags */
#define LABEL_DisposeImage      (LABEL_Dummy + 0x0005) /* Free image on dispose */
#define LABEL_Mapping           (LABEL_Dummy + 0x0006) /* Pen mapping array */
#define LABEL_DrawInfo          (LABEL_Dummy + 0x0007) /* DrawInfo for rendering */
#define LABEL_MenuMode          (LABEL_Dummy + 0x0008) /* Menu-style rendering */
#define LABEL_Underscore        (LABEL_Dummy + 0x0009) /* Shortcut underline char */
#define LABEL_KeyStroke         (LABEL_Dummy + 0x000A) /* Keyboard shortcut */
#define LABEL_TextPen           (LABEL_Dummy + 0x000B) /* Text pen */

/* Justification */
#define LJ_LEFT     0
#define LJ_CENTER   1
#define LJ_RIGHT    2

#ifndef LabelObject
#define LabelObject     NewObject(NULL, LABEL_CLASSNAME
#endif
#ifndef LabelEnd
#define LabelEnd        TAG_END)
#endif

#endif /* IMAGES_LABEL_H */
