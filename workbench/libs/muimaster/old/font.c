#include <exec/types.h>

#ifdef _AROS
#include <proto/graphics.h>
#include <proto/diskfont.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <zunepriv.h>
#include <prefs.h>
#include <font.h>

#ifdef _AROS
const struct TextAttr defaultFont =
    { "topaz.font", 8, FS_NORMAL, 0 };
#endif

GdkFont *
zune_font_get (LONG preset)
{
/*  g_print("zune_font_get %ld\n", preset); */
    if ((preset <= MUIV_Font_Inherit) && (preset >= MUIV_Font_NegCount))
    {
	g_return_val_if_fail(preset <= 0, NULL);
	return __zprefs.fonts[-preset];
    }
    return (GdkFont *)preset;
}

/*
 * On the Amiga, 'fontname' is "font/size"
 */
void
zune_font_replace (GdkFont **font, STRPTR fontname)
{
#ifdef _AROS
    struct TextAttr ta;
    char name[256], *s;
#endif

/*  g_print("replacing preset %ld with font %s\n", preset, fontname); */

    if (*font)
#ifdef _AROS
	CloseFont(*font);
#else
	gdk_font_unref(*font);
#endif

    if (fontname != NULL && strlen(fontname) > 0)
    {
#ifdef _AROS
	strcpy(name, fontname);
	ta.ta_Name = name;

	if ((s = strchr(name, '/')) != NULL)
	{
	    *s++ = 0;
	    ta.ta_YSize = atoi(s);
	}
	else
	    ta.ta_YSize = 11; /* FIXME: */

	strcat(name, ".font");
	ta.ta_Style = FS_NORMAL;
	ta.ta_Flags = 0;

	*font = OpenDiskFont(&ta);
#else
	*font = gdk_font_load(fontname);
#endif
	if (*font)
	{
/*  	    g_print("font loaded\n"); */
	    return;
	}
/*  	g_print("cannot load font\n"); */
    }
    if (__zprefs.fonts[-MUIV_Font_Normal])
    {
	if (fontname != NULL && strlen(fontname) > 0)
	    g_warning("Cannot load font '%s', fallback to window font\n",
		      fontname);
#ifdef _AROS
	*font = __zprefs.fonts[-MUIV_Font_Normal];
	(*font)->tf_Accessors ++; /* FIXME: HACK */
#else
	*font = gdk_font_ref(__zprefs.fonts[-MUIV_Font_Normal]);
#endif
    }
    else
    {
	g_warning("Cannot load font '%s', trying 'fixed'\n", fontname);
#ifdef _AROS
	*font = OpenFont(&defaultFont);
#else
	*font = gdk_font_load("fixed");
#endif
    }
}

