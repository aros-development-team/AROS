#ifndef _PALEDITOR_H_
#define _PALEDITOR_H_

/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/* support functions and definitions ... */
typedef struct penarray {struct ColorMap *cm; ULONG pen[8]; } penarray_t;
extern BOOL allocPens(struct ColorMap *, penarray_t *);
extern void releasePens(penarray_t *);

/*** Identifier base ********************************************************/
#define MUIB_PalEditor                  (TAG_USER | 0x10000000)

#define MUIA_PalEditor_Pens             (MUIB_PalEditor + 1)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *PalEditor_CLASS;

/*** Macros *****************************************************************/
#define PalEditorObject BOOPSIOBJMACRO_START(PalEditor_CLASS->mcc_Class)

#endif /* _PALEDITOR_H_ */
