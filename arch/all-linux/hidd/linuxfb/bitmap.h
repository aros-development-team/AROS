#ifndef LXFB_BITMAP_H
#define LXFB_BITMAP_H

struct BitmapData
{
    UBYTE               *VideoData;
    UBYTE               *RealVideoData;
    HIDDT_PixelFormat   *pixfmt;
    UBYTE               bytesperpix;
    ULONG               bytesperline;
    ULONG               realbytesperline;
    ULONG               width;
    ULONG               height;
    ULONG               display_width;
    ULONG               display_height;
    int                 fbdev;
    BOOL                visible;
};

#endif
