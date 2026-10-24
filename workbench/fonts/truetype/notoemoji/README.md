# Noto Color Emoji fallback asset

This unmodified Google font and its SIL Open Font License were recovered from
this workspace's existing AROS font source. The font SHA-256 is:

`72a635cb3d2f3524c51620cdde406b217204e8a6a06c6a096ff8ed4b5fd6e27b`

The normal and quick TrueType font targets install both files together in
`Fonts/TrueType`. ConsoleNG's device-local fallback opens
`FONTS:TrueType/NotoColorEmoji.ttf`; it needs the FreeType color glyph-map
provider added in private ADT commit `68f3c157a9` and a supporting device.
Font bytes and naming have not been modified.

No `.font` or `.otag` wrappers are installed. This package supplies the tested
color fallback, not a selectable fixed-width primary console font. Registering
it with diskfont requires separate monochrome-preload and font-metrics tests.
Individual emoji rendering and per-console preferences have native QEMU
acceptance. Presentation selectors, grapheme/ZWJ composition and final release
image installation remain separate gates.
