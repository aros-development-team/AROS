#ifndef DEFINES_DISKFONT_H
#define DEFINES_DISKFONT_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define AvailFonts(buffer, bufBytes, flags) \
    AROS_LC3(LONG, AvailFonts, \
    AROS_LCA(STRPTR, buffer, A0), \
    AROS_LCA(LONG  , bufBytes, D0), \
    AROS_LCA(LONG  , flags, D1), \
    struct Library *, DiskfontBase, 6, Diskfont)

#define DisposeFontContents(fontContentsHeader) \
    AROS_LC1(void, DisposeFontContents, \
    AROS_LCA(struct FontContentsHeader *, fontContentsHeader, A1), \
    struct Library *, DiskfontBase, 8, Diskfont)

#define NewFontContents(fontsLock, fontName) \
    AROS_LC2(struct FontContentsHeader *, NewFontContents, \
    AROS_LCA(BPTR  , fontsLock, A0), \
    AROS_LCA(STRPTR, fontName, A1), \
    struct Library *, DiskfontBase, 7, Diskfont)

#define NewScaledDiskFont(sourceFont, destTextAttr) \
    AROS_LC2(struct DiskFont *, NewScaledDiskFont, \
    AROS_LCA(struct TextFont *, sourceFont, A0), \
    AROS_LCA(struct TextAttr *, destTextAttr, A1), \
    struct Library *, DiskfontBase, 9, Diskfont)

#define OpenDiskFont(textAttr) \
    AROS_LC1(struct TextFont *, OpenDiskFont, \
    AROS_LCA(struct TextAttr *, textAttr, A0), \
    struct Library *, DiskfontBase, 5, Diskfont)


#endif /* DEFINES_DISKFONT_H */
