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
