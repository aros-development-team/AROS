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
#define VHI_DIGITAL_CAMERA              19L             // Is this a driver for a digital camera
                                                        // with extended abilities ?
                                                        // Return VHI_DC_TYPE_#? - ID

#define VHI_DC_PORT_OPTIONS             20L             // DC port options in a vhi_dc_port - structure

#define VHI_DC_BAUD_RATE_INFO           21L             // Get back a vhi_dc_baudrate_info - structure
                                                        // A vhi_dc_baurate_info-structure has to be passed
                                                        // in attr;
                                                        // The supported_rates has to be allocated via AllocVec in the driver
                                                        // and freed via FreeVec by the program using the driver

#define VHI_DC_NUMBER_OF_PICS           22L             // Get the number of pictures in memory
                                                        // of the dc as a vhi_dc_number_of_pics structure, that
                                                        // has been passed in attr

#define VHI_DC_FREESTORE                23L             // Fill in the vhi_dc_freestore-structure passed in attr

#define VHI_DC_DCTIME                   24L             // Set or get the time sett for dc as vhi_dc_time-structure
                                                        // passed in attr

#define VHI_DRIVER_SERIAL               25L             // Set the serial-number for the driver for "registration"
                                                        // Suggested: Driver should crash or block after three
                                                        //            unsuccessful attempts to "register" it.
                                                        //            Also try some delay-loops, Delay()-calls or
                                                        //            similiar for checking the serial (passed as
                                                        //            a ULONG). Otherwhise it is easy to crack your
                                                        //            driver within just some minutes!!
                                                        //
                                                        // If your driver doesn`t require a serial, it should
                                                        // return an VHI_ERR_UNKNOWN_METHOD, if the serial is
                                                        // wrong, return VHI_ERR_SERIALNO_WRONG

#define VHI_DC_STATUS                   26L             // Only gettable .. fills in the vhi_dc_status-structure passed
                                                        // as attr

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
#define VHI_FORMAT_NOVIDEO              16L /* new */

/* Digital camera types */
#define VHI_DC_TYPE_NOCAM               0L              // This driver is NOT for a DC
#define VHI_DC_TYPE_SERIAL              1L              // This driver is for a DC connected to the serial port
#define VHI_DC_TYPE_USB                 2L              // This driver is for a DC connected to the USB port
#define VHI_DC_TYPE_IRDA                3L              // This driver is for a DC connected via IrDa
#define VHI_DC_TYPE_TCPIP               4L              // This driver is for a DC connected via TCP/IP
                                                        // .. yet not really implemented in the standard ..

/* Performable methods */
#define VHI_CHECK_DIGITIZE_SIZE         1L
#define VHI_DIGITIZE_PICTURE            2L
#define VHI_DC_GET_THUMBNAIL            3L              /* Get back the thumbnail-image vhi_dc_getimage
                                                         of a picture as a vhi_dc_image
                                                         Return error VHI_ERR_DC_NO_THUMBNAIL and NULL,
                                                         if no thumbnails are available. */

#define VHI_DC_GET_FULL_PICTURE         4L              /* Get back the image vhi_dc_getimage of a picture
                                                         as a vhi_dc_image */

#define VHI_DC_DELETE_PICTURE           5L              /* Delete picture <num> (1 = first) */
#define VHI_DC_ERASE_MEMORY             6L              /* Erase all pictures in the camera */

#define VHI_DC_TAKE_PICTURE             7L              /* Take a picture and store it in the cam's memory
                                                           This method returns either TRUE for success or
                                                           FALSE and an errormessage */

/* Modes to get digitized results */
#define VHI_MODE_COLOR                  1L
#define VHI_MODE_GRAYSCALE              2L

/* Types of vhi_images */
#define VHI_GRAYSCALE_8                 1L
#define VHI_RGB_24                      2L
#define VHI_YUV_411                     3L
#define VHI_YUV_422                     4L
#define VHI_YUV_444                     5L
#define VHI_YUV_ACCUPAK                 6L              // not yet supported by VHI Studio, but defined in the VHI-standard for other apps
#define VHI_JPEG                        7L
#define VHI_EXIF_JPEG                   8L

/* Error-Messages */
#define VHI_ERR_UNKNOWN_METHOD          1L              // unknown/unhandled method.
#define VHI_ERR_COULD_NOT_INIT          2L              // VHI device initializion failed.
#define VHI_ERR_NO_HARDWARE             3L              // VHI device not present
#define VHI_ERR_NO_INPUT_DATA           4L
#define VHI_ERR_ERR_WHILE_DIG           5L              // Error buffering/digitizing from device.
#define VHI_ERR_OUT_OF_MEMORY           6L
#define VHI_ERR_INTERNAL_ERROR          7L              // Internal driver error
#define VHI_ERR_UNKNOWN_OPTION          8L              // Means: Method is known, option passed is unknown

#define VHI_ERR_DC_OFFLINE              9L              // Means: DC seems to be offline
#define VHI_ERR_DC_NO_THUMBNAIL         10L             // Means: No thumbnail available for this picture from the DC
#define VHI_ERR_DC_NO_FULLPIC           11L             // Means: No full picture available for this picture from the DC
#define VHI_ERR_SERIALNO_WRONG          12L             // Means: The submitted serialno is wrong for the driver !!
#define VHI_ERR_DC_PORT_IN_USE          13L             // Means: The port is already in use
#define VHI_ERR_DC_RATE_UNSUPP          14L             // Means: This baudrate is not supported.
#define VHI_ERR_DC_NOTIMEAVAIL          15L             // Means: There is no way to get or set the time of the connected DC

#define VHI_ERR_IO_WRITE                16L             // Means: Error while writing/sending data
#define VHI_ERR_IO_READ                 17L             // Means: Error while reading/receiving data
#define VHI_ERR_IO_EXECUTION            18L             // Means: Execution failed

/**********************************/
/* Video & DC-specific Structures */
/**********************************/

struct vhi_setoptions
{
    ULONG                       option;                 // Option ID
    ULONG                       value;                  // new option value
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
    APTR                        chunky;                 // For grayscale, RGB, JPEG or EXIF-JPEG
    APTR                        y;                      // The Y
    APTR                        u;                      // The U
    APTR                        v;                      // The V-data
    ULONG                       width;                  // Width of the image, file size for JPEG and EXIF-JPEG
    ULONG                       height;                 // Height of the image, zero for JPEG and EXIF-JPEG
    ULONG                       type;                   // e.g. VHI_YUV_422
    BOOL                        scaled;                 // Image has been scaled internally
};

struct vhi_digitize
{
    struct vhi_dimensions       dim;
    BOOL                        custom_memhandling;
    APTR                        (* CstAllocVec) (ULONG size, ULONG flags);      // Provide function-pointers to use
    void                        (* CstFreeVec)  (APTR mem);                     // in case this is important (e.g. for
                                                                                // correct alignment ..)
};


/**************************************/
/* Digital camera specific structures */
/**************************************/
struct vhi_dc_freestore
{
    ULONG                       bytes_free;             // Number of bytes still free in the cams memory
    ULONG                       bytes_total;            // Size of the cams memory in bytes
    BOOL                        avail;                  // TRUE=This information is available and valid, FALSE = this information is not available
};

struct vhi_dc_getimage
{
    LONG                        struct_size;            // has to be sizeof(struct vhi_dc_getimage) ..
                                                        // allows future extension of the structure
                                                        // without causing Enforcer Hits and without
                                                        // need of recompilation

    ULONG                       pic;                    // Picture to get. First picture is 1

    void                        (* set_progress) (char *msg, ULONG perc);// Functionpointer to progressbar
                                                        // .. msg has to be AllocVec`d, NULL terminated string
                                                        // .. perc can be ~0, if the percentage shall not be
                                                        //    changed

    BOOL                        (* check_cancel) (void);// Functionpointer to a function,
                                                        // that checks, whether the Cancel-button
                                                        // next to the progress-bar has been canceled.
                                                        // TRUE. if canceled

    BOOL                        custom_memhandling;

    APTR                        (* CstAllocVec) (ULONG size, ULONG flags); // Provide function-pointers to use

    void                        (* CstFreeVec)  (APTR mem); // in case this is important (e.g. for
                                                        // correct alignment ..)
};

struct vhi_dc_number_of_pics
{
    ULONG                       num;                    /* Number of pictures in the cam`s memory */
    BOOL                        thumbsavail;            /* Are thumbnails available ?             */
};

struct vhi_dc_port
{
    STRPTR                      device;                 // AllocVec-allocated string containg the device-name
    ULONG                       unit;                   // unit of the camera
    ULONG                       baud;                   // Baudrate (as index of supported_rates)
};

#define VHI_DC_RATE_UNKNOWN             0L

struct vhi_dc_baudrate_info
{
    ULONG                       num_of_supported_rates; // Number of entries in "supported_rates"
    ULONG                       *supported_rates;       // Rates supported by the dc. Set the last element to 0.
    ULONG                       defaultrate;            // The recommended defaultrate (as index of supported_rates)
    APTR                        usb;                    // Currently not supported. Set this to NULL
                                                        // (lateron pointer to more USB-related information).
};

#define VHI_ROT_UNKNOWN                 1L
#define VHI_ROT_NO_ROTATION             2L
#define VHI_ROT_ROTATE90                3L
#define VHI_ROT_ROTATE180               4L
#define VHI_ROT_ROTATE270               5L

#define VHI_FLASH_UNKNOWN               1L
#define VHI_FLASH_ON                    2L
#define VHI_FLASH_OFF                   3L

#define VHI_EXPOSURE_UNKNOWN            ~0

#define VHI_APERTURE_UNKNOWN            ~0

#define VHI_ZOOM_UNKNOWN                ~0

#define VHI_TIME_UNKNOWN                1000

struct vhi_dc_time
{
    ULONG                       day, month, year;       // As variable-names
    ULONG                       hour, min, sec;         // from 0h of the respective day
};

struct vhi_dc_image
{
    LONG                        struct_size;            // has to be sizeof(struct vhi_dc_image) ..
                                                        // allows future extension of the structure
                                                        // without causing Enforcer Hits and without
                                                        // need of recompilation

    struct vhi_image            image;                  // Contains the image-data .. see above
    ULONG                       file_size;              // size of the picture in bytes (for JPEG and EXIF-JPEG)
    struct vhi_dc_time          time;                   // Date of the picture
    ULONG                       degree;                 // Any of the VHI_ROT_#?
    ULONG                       flash;                  // Was flash active while taking the photo ?
    ULONG                       zoom_setting;           // Zoom-Settings * 100 .. e.g. 1500 for 15 mm .. VHI_ZOOM_UNKNOWN for unknown value
    ULONG                       exposure;               // How long was light on the pic ? Lighttime in ms * 100 .. e.g. 1500 for 15 ms
    ULONG                       aperture;               // Blendeneinstellung .. see exposure for calculation ..

    BOOL                        is_thumbnail;           // Is this a thumbnail ? TRUE if yes, FALSE if no
    ULONG                       fullsize_width;         // If it is a thumbnail, the size of the full image. 0 for unknown.
    ULONG                       fullsize_height;        // If it isn`t a thumbnail, these values are undefined.
};

struct vhi_dc_status
{
    LONG                        struct_size;            // has to be sizeof(struct vhi_dc_image) ..
                                                        // allows future extension of the structure
                                                        // without causing Enforcer Hits and without
                                                        // need of recompilation

    ULONG                       battery_status;         // Has to be filled with a VHI_BATTERY_#? - code
    ULONG                       pictures_taken;         // Number of pictures taken, VHI_STATUS_UNKNOWN, if unknown
    ULONG                       flashes_fired;          // Number of flashes fired,  VHI_STATUS_UNKNOWN, if unknown
    ULONG                       memory_total;           // Total space available in bytes for the cam (e.g. if a 8 MB card is
                                                        // plugged in, 8 MB are returned) .. VHI_STATUS_UNKNOWN, if this
                                                        // is unknown
    ULONG                       memory_used;            // Memory used by the pictures in bytes
};

#define VHI_BATTERY_UNKNOWN             ~0              // If you do not know anything about this
#define VHI_BATTERY_ACADAPTER           1               // If a AC-Adapter is in use
#define VHI_BATTERY_EMPTY               2               // If the battery is empty
#define VHI_BATTERY_LOW                 3               // If battery-power is low
#define VHI_BATTERY_MEDIUM              4               // If battery-power is medium
#define VHI_BATTERY_FULL                5               // If battery-power is at its best

#define VHI_STATUS_UNKNOWN              ~0

#if !defined(__AROS__)
/* Needed to get the correct adress for Y, U, V-data */
#define LONGWORDALIGN(addr) (((((ULONG) (addr))+8) >> 2L) << 2L)
#endif

#endif
