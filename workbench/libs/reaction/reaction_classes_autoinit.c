/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Auto-open hooks for the reaction class libraries. Linked into
          libreaction.a so that any program built against -lreaction
          gets ButtonBase, CheckBoxBase, ... opened/closed automatically
          (mirroring the behaviour of ClassAct's classact.lib on AmigaOS).

    Each AROS_LIBSET registers a global library base and an entry in the
    LIBS symbolset; the C startup walks the set and calls OpenLibrary()
    for every entry, falling back to a graceful exit if the open fails.
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/symbolsets.h>

/* Window/Requester classes */
AROS_LIBSET("window.class",         struct Library *, WindowBase)
AROS_LIBSET("requester.class",      struct Library *, RequesterBase)

/* Layout / page / space */
AROS_LIBSET("gadgets/layout.gadget", struct Library *, LayoutBase)
AROS_LIBSET("gadgets/page.gadget",   struct Library *, PageBase)
AROS_LIBSET("gadgets/space.gadget",  struct Library *, SpaceBase)

/* Interactive gadgets */
AROS_LIBSET("gadgets/button.gadget",        struct Library *, ButtonBase)
AROS_LIBSET("gadgets/checkbox.gadget",      struct Library *, CheckBoxBase)
AROS_LIBSET("gadgets/chooser.gadget",       struct Library *, ChooserBase)
AROS_LIBSET("gadgets/clicktab.gadget",      struct Library *, ClickTabBase)
AROS_LIBSET("gadgets/fuelgauge.gadget",     struct Library *, FuelGaugeBase)
AROS_LIBSET("gadgets/getfile.gadget",       struct Library *, GetFileBase)
AROS_LIBSET("gadgets/getfont.gadget",       struct Library *, GetFontBase)
AROS_LIBSET("gadgets/getscreenmode.gadget", struct Library *, GetScreenModeBase)
AROS_LIBSET("gadgets/integer.gadget",       struct Library *, IntegerBase)
AROS_LIBSET("gadgets/listbrowser.gadget",   struct Library *, ListBrowserBase)
AROS_LIBSET("gadgets/palette.gadget",       struct Library *, PaletteBase)
AROS_LIBSET("gadgets/radiobutton.gadget",   struct Library *, RadioButtonBase)
AROS_LIBSET("gadgets/scroller.gadget",      struct Library *, ScrollerBase)
AROS_LIBSET("gadgets/slider.gadget",        struct Library *, SliderBase)
AROS_LIBSET("gadgets/speedbar.gadget",      struct Library *, SpeedBarBase)
AROS_LIBSET("gadgets/string.gadget",        struct Library *, StringBase)
AROS_LIBSET("gadgets/textfield.gadget",     struct Library *, TextFieldBase)

/* Image classes */
AROS_LIBSET("images/bevel.image",    struct Library *, BevelBase)
AROS_LIBSET("images/bitmap.image",   struct Library *, BitMapBase)
AROS_LIBSET("images/drawlist.image", struct Library *, DrawListBase)
AROS_LIBSET("images/glyph.image",    struct Library *, GlyphBase)
AROS_LIBSET("images/label.image",    struct Library *, LabelBase)
AROS_LIBSET("images/led.image",      struct Library *, LedBase)
AROS_LIBSET("images/penmap.image",   struct Library *, PenMapBase)
