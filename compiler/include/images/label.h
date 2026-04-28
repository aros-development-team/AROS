/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/label.h
*/

#ifndef IMAGES_LABEL_H
#define IMAGES_LABEL_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define LABEL_CLASSNAME     "images/label.image"
#define LABEL_VERSION       44

#define LABEL_Dummy         (TAG_USER + 0x190000)

#define LABEL_Text              (LABEL_Dummy + 0x0001)
#define LABEL_Image             (LABEL_Dummy + 0x0002)
#define LABEL_Justification     (LABEL_Dummy + 0x0003)
#define LABEL_SoftStyle         (LABEL_Dummy + 0x0004)
#define LABEL_DisposeImage      (LABEL_Dummy + 0x0005)
#define LABEL_Mapping           (LABEL_Dummy + 0x0006)
#define LABEL_DrawInfo          (LABEL_Dummy + 0x0007)
#define LABEL_MenuMode          (LABEL_Dummy + 0x0008)
#define LABEL_Underscore        (LABEL_Dummy + 0x0009)
#define LABEL_KeyStroke         (LABEL_Dummy + 0x000A)
#define LABEL_TextPen           (LABEL_Dummy + 0x000B)

/* Label justification */
#define LJ_LEFT     0
#define LJ_CENTER   1
#define LJ_RIGHT    2

#define LabelObject     NewObject(NULL, LABEL_CLASSNAME
#define LabelEnd        TAG_END)

#endif /* IMAGES_LABEL_H */
