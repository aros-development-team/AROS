#ifndef INTUITION_PREFERENCES_H
#   include <intuition/preferences.h>
#endif

/*
** The intuition default preferences structure
*/

struct Preferences IntuitionDefaultPreferences =
{
    0,      // FontHeight
    0,      // PrinterPort
    0,      // BaudRate

    {
        0,40000
    }
    ,   // timeval KeyRptSpeed

    {
        0,500000
    }
    ,   // timeval KeyRptDelay

    {
        0,500000
    }
    ,   // timeval DoubleClick

    {
        0,12,
        0xff00, 0xfff0,
        0xfe00, 0xffe0,
        0xfc00, 0xffc0,
        0xf800, 0xff80,
        0xf000, 0xff00,
        0xe000, 0xfe00,
        0xc000, 0xfc00,
        0x8000, 0xf800,
        0x0000, 0xf000,
        0x0000, 0xe000,
        0x0000, 0xc000,
        0x0000, 0x8000,
        0x0000, 0x0000,
        0x0000, 0x0000,
        0x0000, 0x0000,
        0x0000, 0x0000,
        0,0
    }
    ,   // PointerMatrix (36 entries)
    0,      // XOffset
    0,      // YOffset
    0x0b00, // color17
    0x0d00, // color18
    0x0f00, // color19
    0,      // PointerTicks

    0x0999, // color0-4
    0x0000,
    0x0eee,
    0x068b,

    0,      // ViewXOffset
    0,      // ViewYOffset
    0,      // ViewInitX
    0,      // ViewINitY

    0,      // EnableCLI

    0,      // PointerType
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    }
    ,// PrinterFilename (30 chars)

    0,      // PrintPitch
    0,      // PrintQuality
    0,      // PrintSpacing
    0,      // PrintLeftMargin
    0,      // PrintRightMargin
    0,      // PrintImage
    0,      // PrintAspect
    0,      // PrintShade
    0,      // PrintTreshold

    0,      // PaperSize
    0,      // PaperLength
    0,      // PaperType

    0,      // SerRWBits
    0,      // SerStopBuf
    0,      // SerParShk

    0,      // LaceWb

    {
        0,0,0,0,0,0,0,0,0,0,0,0
    }
    ,       // pad (12 bytes)
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    }
    ,   // PrtDevName (16 chars)

    0,      // DefaultPrtUnit
    0,      // DefaultSerUnit

    0,      // RowSizeChange
    0,      // ColumnSizeChange

    0,      // PrintFlags
    0,      // PrintMaxWidth
    0,      // PrintMaxHeight
    0,      // PrintDensity
    0,      // PrintXOffset

    0,      // wb_Width
    0,      // wb_Height
    0,      // wb_Depth

    0       // ext size
};
