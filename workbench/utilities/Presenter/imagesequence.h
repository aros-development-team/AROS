#ifndef IMAGESEQUENCE_H
#define IMAGESEQUENCE_H

#include <utility/tagitem.h>

/*** Variables **************************************************************/
extern struct MUI_CustomClass *ImageSequence_CLASS;

/*** Macros *****************************************************************/
#define ImageSequenceObject BOOPSIOBJMACRO_START(ImageSequence_CLASS->mcc_Class)

/*** Functions **************************************************************/
void ImageSequence_Create();
void ImageSequence_Destroy();

/*** Attributes *************************************************************/
#define MUIA_ImageSequence_BASE        (TAG_USER+0x42000)              

/** BOOL [ISG] */
#define MUIA_ImageSequence_Loop        (MUIA_ImageSequence_BASE+0)

/** BOOL [ISG] */
#define MUIA_ImageSequence_Auto        (MUIA_ImageSequence_BASE+1)

/* ULONG [ISG] (microseconds? seconds?) */
#define MUIA_ImageSequence_Auto_Delay  (MUIA_ImageSequence_BASE+2) 

/* ???   [I--] (list of files) */
#define MUIA_ImageSequence_List        (MUIA_ImageSequence_BASE+3) 

#endif /* IMAGESEQUENCE_H */
