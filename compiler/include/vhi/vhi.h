#ifndef _VHI_H
#define _VHI_H 1

/* Methods */
#define VHI_METHOD_OPEN                 1L
#define VHI_METHOD_CLOSE                2L
#define VHI_METHOD_SET                  3L
#define VHI_METHOD_GET                  4L
#define VHI_METHOD_PERFORM              5L

/* Get & Set - Methods */
#define VHI_CARD_NAME                   1L
#define VHI_CARD_MANUFACTURER           2L
#define VHI_CARD_REVISION               3L
#define VHI_CARD_VERSION                4L
#define VHI_CARD_DRIVERAUTHOR           5L
#define VHI_SUPPORTED_MODES             6L
#define VHI_NUMBER_OF_INPUTS            7L
#define VHI_NAME_OF_INPUT               8L
#define VHI_SUPPORTED_OPTIONS           9L
#define VHI_SUPPORTED_VIDEOFORMATS      10L
#define VHI_MAXIMUM_SIZE                11L
#define VHI_OPTIONS                     12L
#define VHI_VIDEOFORMAT                 13L
#define VHI_COLOR_MODE                  14L
#define VHI_TRUSTME_MODE                15L
#define VHI_TRUSTME_SIZE                16L
#define VHI_CARD_DRIVERVERSION          17L
#define VHI_CARD_DRIVERREVISION         18L

/* Options */
#define VHI_OPT_LUMINANCE               1L
#define VHI_OPT_CHROMINANCE             2L
#define VHI_OPT_NOISE                   4L
#define VHI_OPT_VIDEOREC                8L
#define VHI_OPT_NTSCPAL                 16L
#define VHI_OPT_INTERLACE               32L
#define VHI_OPT_INPUT                   64L

/* Video Formats - IDs (bitwise combination) */
#define VHI_FORMAT_PAL                  1L
#define VHI_FORMAT_NTSC                 2L
#define VHI_FORMAT_SECAM                4L
#define VHI_FORMAT_PAL60                8L

/* Performable methods */
#define VHI_CHECK_DIGITIZE_SIZE         1L
#define VHI_DIGITIZE_PICTURE            2L

/* Modes to get digitized results */
#define VHI_MODE_COLOR                  1L
#define VHI_MODE_GRAYSCALE              2L

/* Types of vhi_images */
#define VHI_GRAYSCALE_8                 1L
#define VHI_RGB_24                      2L
#define VHI_YUV_411                     3L
#define VHI_YUV_422                     4L
#define VHI_YUV_444                     5L

/* Error-Messages */
#define VHI_ERR_UNKNOWN_METHOD          1L              // unknown/unhandled method.
#define VHI_ERR_COULD_NOT_INIT          2L              // VHI device initializion failed.
#define VHI_ERR_NO_HARDWARE             3L              // VHI device not present
#define VHI_ERR_NO_INPUT_DATA           4L
#define VHI_ERR_ERR_WHILE_DIG           5L              // Error buffering/digitizing from device.
#define VHI_ERR_OUT_OF_MEMORY           6L
#define VHI_ERR_INTERNAL_ERROR          7L              // Internal driver error

/* Structures */
struct vhi_setoptions
{
    ULONG                       option;                 // The ID of the option
    ULONG                       value;                  // The new value for this option
};

struct vhi_dimensions
{
    ULONG                       x1;                     // The rectangle to grab from the
    ULONG                       y1;                     // full picture

    ULONG                       x2;
    ULONG                       y2;

    ULONG                       dst_width;              // Target dimensions, to scale data to.
    ULONG                       dst_height;
};

struct vhi_size
{
    ULONG                       max_width;              // Maximum Width
    ULONG                       max_height;             // Maximum Height
    BOOL                        fixed;                  // Can`t be changed = TRUE (no cropping)
    BOOL                        scalable;               // Driver/Board supports internal scaling
};

struct vhi_image
{
    APTR                        chunky;                 // For grayscale or RGB
    APTR                        y;                      // The Y
    APTR                        u;                      // The U
    APTR                        v;                      // The V-data
    ULONG                       width;                  // Width of the image
    ULONG                       height;                 // Height of the image
    ULONG                       type;                   // e.g. VHI_YUV_422
    BOOL                        scaled;                 // Image has been scaled internally
};

struct vhi_digitize
{
    struct vhi_dimensions       dim;
    BOOL                        custom_memhandling;
    APTR                        (* CstAllocVec) (ULONG size, ULONG flags); // Provide function-pointers to use
    void                        (* CstFreeVec)  (APTR mem);                // in case this is important (e.g. for
                                                        // correct alignment ..)
};

#if !defined(__AROS__)
/* Needed to get the correct adress for Y, U, V-data */
#define LONGWORDALIGN(addr) (((((ULONG) (addr))+8) >> 2L) << 2L)
#endif

#endif
