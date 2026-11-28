/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _MUIMASTER_THEMEIMAGE_H
#define _MUIMASTER_THEMEIMAGE_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct Screen;

/* where the images of the theme in use are found: the image set of the
   selected size, followed by the theme's other sets */
#define THEMEIMAGE_PREFIX       "THEMEIMAGES:"
/* the directory of the theme in use holding its image sets */
#define THEMEIMAGE_SETS_DIR     "THEME:Images/"
/* where all installed themes are found */
#define THEMEIMAGE_SETS_PREFIX  "THEMES:"

#define THEMEIMAGE_MAXSIZES     8

/* Fills 'sizes' with the sizes provided beneath 'imagedir', best match
   for 'wanted' first. Returns the number of sizes found. */
ULONG zune_themeimage_sizes(CONST_STRPTR imagedir, ULONG wanted,
    ULONG *sizes, ULONG maxsizes);

/* Parses "<number>pt"; returns 0 for any other name */
ULONG zune_themeimage_parse_size(CONST_STRPTR name, ULONG len);

/* TRUE if the name ends in an extension used for external images */
BOOL zune_themeimage_is_image(CONST_STRPTR name);

/* Finds the file an external image name refers to. Returns a path to
   release with FreeVec(), or NULL if there is no such image. */
STRPTR zune_themeimage_locate(CONST_STRPTR name, struct Screen *scr);

#endif /* _MUIMASTER_THEMEIMAGE_H */
