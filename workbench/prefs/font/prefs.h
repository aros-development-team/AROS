#ifndef PREFS_H
#define PREFS_H

#include <exec/types.h>
#include <prefs/font.h>
#include <dos/bptr.h>

#define FP_COUNT (3)  /* Number of entries in fped_FontPrefs array */

/* Data is stored on disk in this format */
struct FileFontPrefs
{
    UBYTE   fp_Reserved[4 * 3];
    UBYTE   fp_Reserved2[2];
    UBYTE   fp_Type[2];
    UBYTE   fp_FrontPen;
    UBYTE   fp_BackPen;
    UBYTE   fp_Drawmode;
    UBYTE   fp_pad;
    UBYTE   fp_TextAttr_ta_Name[4];
    UBYTE   fp_TextAttr_ta_YSize[2];
    UBYTE   fp_TextAttr_ta_Style;
    UBYTE   fp_TextAttr_ta_Flags;
    BYTE    fp_Name[FONTNAMESIZE];
};

/*** Prototypes *************************************************************/
BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save);
BOOL Prefs_Default(struct FontPrefs fp[]);
BOOL Prefs_ImportFH(BPTR fh, struct FontPrefs fp[]);
BOOL Prefs_ExportFH(BPTR fh, struct FontPrefs fp[]);

#endif
