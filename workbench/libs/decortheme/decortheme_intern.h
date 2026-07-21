/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: decortheme.library internal header
*/

#ifndef DECORTHEME_INTERN_H
#define DECORTHEME_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/decortheme.h>

struct DecorThemeBase
{
    struct Library          dtb_Lib;
};

/* The library holds no theme state of its own - all state lives in
   the DecorTheme instances handed to clients, so any number of themes
   can be loaded concurrently by any number of clients. */

/* themeconfig.c */
struct DecorConfig *LoadConfig(STRPTR path);
void FreeConfig(struct DecorConfig *dc);

/* themeimages.c */
struct DecorImages *NewImages(void);
struct DecorImages *LoadImages(struct DecorConfig *dc);
void FreeImages(struct DecorImages *di);

/* themeelements.c */
void InitThemeElements(struct DecoratorElement *elements, struct DecorConfig *dc, struct DecorImages *di);

/* themescreen.c */
struct DecorThemeScreen *ObtainScreenTheme(struct DecorTheme *theme, struct Screen *scr, BOOL truecolor);
void ReleaseScreenTheme(struct DecorThemeScreen *dts);

#endif /* DECORTHEME_INTERN_H */
