#ifndef LXFB_BITMAP_H
#define LXFB_BITMAP_H

struct BitmapData
{
    UBYTE *VideoData;
    UBYTE *RealVideoData;
    UBYTE bytesperpix;
    ULONG bytesperline;
    ULONG realbytesperline;
    ULONG width;
    ULONG height;
    BOOL  VideoDataAllocated;
};

#endif
