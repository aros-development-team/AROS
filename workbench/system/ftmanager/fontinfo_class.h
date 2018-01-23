#ifndef FONTINFO_CLASS_H
#define FONTINFO_CLASS_H

#include <diskfont/diskfonttag.h>

#define FONTINFO_BASE			TAG_USER
#define MUIA_FontInfo_Filename		(FONTINFO_BASE + 1)
#define MUIA_FontInfo_Face		(FONTINFO_BASE + 2)

#define FONTINFO_MBASE			TAG_USER
#define MUIM_FontInfo_UpdatePreview	(FONTINFO_MBASE + 1)
#define MUIM_FontInfo_SetOTags		(FONTINFO_MBASE + 2)
#define MUIM_FontInfo_WriteFiles	(FONTINFO_MBASE + 3)

struct MUI_CustomClass *FontInfoClass;

#define FontInfoObject	NewObject(FontInfoClass->mcc_Class, NULL //)

void CleanupFontInfoClass(void);
int InitFontInfoClass(void);


struct FontInfoData
{
    STRPTR Filename;
    FT_Face Face;
    Object *AttachedFile;
    Object *Name;
    Object *YSizeFactorHigh;
    Object *YSizeFactorLow;
    Object *StemWeight;
    Object *SlantStyle;
    Object *HorizStyle;
    Object *Family;
    Object *Fixed;
    Object *Serif;
    //Object *AlgoStyle;
    Object *FaceNum;
    Object *Metric;
    Object *BBoxYMin;
    Object *BBoxYMax;
    Object *SpaceWidth;
    Object *Preview;
    Object *PreviewGroup;
    Object *Gray;
    Object *TestSize;
    Object *TestString;
    struct TagItem OTags[26];
    UWORD AvailSizes[OT_MAXAVAILSIZES];
};

typedef struct FontInfoData FontInfoData;

ULONG fiWriteFiles(FontInfoData *dat, STRPTR base, STRPTR target_dir, ULONG size);

#endif
