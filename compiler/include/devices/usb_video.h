#ifndef DEVICES_USB_VIDEO_H
#define DEVICES_USB_VIDEO_H
/*
**	$VER: usb_video.h 2.0 (15.12.07)
**
**	usb definitions include file
**
**	(C) Copyright 2008 Chris Hodges
**	    All Rights Reserved
*/

#include <exec/types.h>

#if defined(__GNUC__)
# pragma pack(1)
#endif

/* Usb Video Requests */
#define UVUDR_SET_CUR             0x01
#define UVUDR_GET_CUR             0x81
#define UVUDR_SET_MIN             0x02
#define UVUDR_GET_MIN             0x82
#define UVUDR_SET_MAX             0x03
#define UVUDR_GET_MAX             0x83
#define UVUDR_SET_RES             0x04
#define UVUDR_GET_RES             0x84
#define UVUDR_GET_LEN             0x85
#define UVUDR_GET_INFO            0x86
#define UVUDR_GET_DEF             0x87

/* Video Ctrl class specific interface descriptor subtypes */
#define UDST_VIDEO_CTRL_HEADER          0x01
#define UDST_VIDEO_CTRL_INPUT_TERMINAL  0x02
#define UDST_VIDEO_CTRL_OUTPUT_TERMINAL 0x03
#define UDST_VIDEO_CTRL_SELECTOR_UNIT   0x04
#define UDST_VIDEO_CTRL_PROCESSING_UNIT 0x05
#define UDST_VIDEO_CTRL_EXTENSION_UNIT  0x06

/* Video Streaming class specific interface descriptor subtypes */
#define UDST_VIDEO_STREAM_INPUT_HEADER        0x01
#define UDST_VIDEO_STREAM_OUTPUT_HEADER       0x02
#define UDST_VIDEO_STREAM_STILL_IMAGE_FRAME   0x03
#define UDST_VIDEO_STREAM_FORMAT_UNCOMPRESSED 0x04
#define UDST_VIDEO_STREAM_FRAME_UNCOMPRESSED  0x05
#define UDST_VIDEO_STREAM_FORMAT_MJPEG        0x06
#define UDST_VIDEO_STREAM_FRAME_MJPEG         0x07
#define UDST_VIDEO_STREAM_FORMAT_MPEG2TS      0x0a
#define UDST_VIDEO_STREAM_FORMAT_DV           0x0c
#define UDST_VIDEO_STREAM_COLORFORMAT         0x0d
#define UDST_VIDEO_STREAM_FORMAT_FRAME_BASED  0x10
#define UDST_VIDEO_STREAM_FRAME_FRAME_BASED   0x11
#define UDST_VIDEO_STREAM_FORMAT_STREAM_BASED 0x12

/* Videoclass specific endpoint descriptors subtypes */
#define UDST_VIDEO_EP_GENERAL    0x01
#define UDST_VIDEO_EP_ENDPOINT   0x02
#define UDST_VIDEO_EP_INTERRUPT  0x03

/* Video classes */
#define VIDEO_NO_SUBCLASS     0x00
#define VIDEO_CTRL_SUBCLASS   0x01
#define VIDEO_STREAM_SUBCLASS 0x02
#define VIDEO_IFCOLL_SUBCLASS 0x03

/* USB Video specific stuff */

/* USB Video USB Terminal types */
#define UVUTT_VENDOR           0x0100 /* USB vendor specific */
#define UVUTT_STREAMING        0x0101 /* USB streaming */

/* USB Video Input Terminal types */
#define UVITT_VENDOR           0x0200 /* Input Vendor specific */
#define UVITT_CAMERA           0x0201 /* Camera sensor */
#define UVITT_MEDIA_TRANSPORT  0x0202 /* Sequential media */

/* USB Video Output Terminal types */
#define UVOTT_VENDOR           0x0300 /* Output Vendor specific */
#define UVOTT_DISPLAY          0x0301 /* Generic display */
#define UVOTT_MEDIA_TRANSPORT  0x0302 /* Sequential media */

/* USB Video External Terminal types */
#define UVETT_VENDOR           0x0400 /* External Vendor specific */
#define UVETT_COMPOSITE_CONNECTOR 0x0401 /* Composite video connector */
#define UVETT_SVIDEO_CONNECTOR 0x0402 /* S-video connector */
#define UVETT_COMPONENT_CONNECTOR 0x0403 /* Component video connector */

/* VideoControl Interface Control Selectors */
#define UVVCCS_VIDEO_POWER_MODE_CONTROL   0x01
#define UVVCCS_REQUEST_ERROR_CODE_CONTROL 0x02

/* Selector Unit Control Selectors */
#define UVSUCS_INPUT_SELECT_CONTROL       0x01

/* Camera Terminal Control Selectors */
#define UVCTCS_SCANNING_MODE_CONTROL      0x01
#define UVCTCS_AE_MODE_CONTROL            0x02
#define UVCTCS_AE_PRIORITY_CONTROL        0x03
#define UVCTCS_EXPOSURE_TIME_ABS_CONTROL  0x04
#define UVCTCS_EXPOSURE_TIME_REL_CONTROL  0x05
#define UVCTCS_FOCUS_ABS_CONTROL          0x06
#define UVCTCS_FOCUS_REL_CONTROL          0x07
#define UVCTCS_FOCUS_AUTO_CONTROL         0x08
#define UVCTCS_IRIS_ABS_CONTROL           0x09
#define UVCTCS_IRIS_REL_CONTROL           0x0a
#define UVCTCS_ZOOM_ABS_CONTROL           0x0b
#define UVCTCS_ZOOM_REL_CONTROL           0x0c
#define UVCTCS_PANTILT_ABS_CONTROL        0x0d
#define UVCTCS_PANTILT_REL_CONTROL        0x0e
#define UVCTCS_ROLL_ABS_CONTROL           0x0f
#define UVCTCS_ROLL_REL_CONTROL           0x10
#define UVCTCS_PRIVACY_CONTROL            0x11

/* Processing Unit Control Selectors */
#define UVPUCS_BACKLIGHT_COMP_CONTROL     0x01
#define UVPUCS_BRIGHTNESS_CONTROL         0x02
#define UVPUCS_CONTRAST_CONTROL           0x03
#define UVPUCS_GAIN_CONTROL               0x04
#define UVPUCS_POWER_LINE_FREQ_CONTROL    0x05
#define UVPUCS_HUE_CONTROL                0x06
#define UVPUCS_SATURATION_CONTROL         0x07
#define UVPUCS_SHARPNESS_CONTROL          0x08
#define UVPUCS_GAMMA_CONTROL              0x09
#define UVPUCS_WB_TEMP_CONTROL            0x0a
#define UVPUCS_WB_TEMP_AUTO_CONTROL       0x0b
#define UVPUCS_WB_COMPONENT_CONTROL       0x0c
#define UVPUCS_WB_COMPONENT_AUTO_CONTROL  0x0d
#define UVPUCS_DIGITAL_MULT_CONTROL       0x0e
#define UVPUCS_DIGITAL_MULT_LIMIT_CONTROL 0x0f
#define UVPUCS_HUE_AUTO_CONTROL           0x10
#define UVPUCS_ANALOG_VIDEO_STD_CONTROL   0x11
#define UVPUCS_ANALOG_LOCK_STATUS_CONTROL 0x12

/* VideoStreaming Interface Control Selectors */
#define UVVSCS_PROBE_CONTROL              0x01
#define UVVSCS_COMMIT_CONTROL             0x02
#define UVVSCS_STILL_PROBE_CONTROL        0x03
#define UVVSCS_STILL_COMMIT_CONTROL       0x04
#define UVVSCS_IMAGE_TRIGGER_CONTROL      0x05
#define UVVSCS_STREAM_ERROR_COE_CONTROL   0x06
#define UVVSCS_GENERATE_KEY_FRAME_CONTROL 0x07
#define UVVSCS_UPDATE_FRAME_SEG_CONTROL   0x08
#define UVVSCS_SYNCH_DELAY_CONTROL        0x09


/* USB Video Spec 1.1 stuff */
struct UsbVideoHeaderDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x01) */
    UBYTE bcdUVC0;             /* Low byte of spec version (0x00) */
    UBYTE bcdUVC1;             /* High byte of spec version (0x01) */
    UBYTE wTotalLength0;       /* Total length of all descriptors (low byte) */
    UBYTE wTotalLength1;       /* Total length of all descriptors (high byte) */
    UBYTE dwClockFreq[4];      /* Clock frequency (deprecated) */
    UBYTE bInCollection;       /* The number of Video/MidiStreaming */
    UBYTE baInterfaceNr[1];    /* Interface number of the first VideoStreaming interface */
};

struct UsbVideoInputTermDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x02) */
    UBYTE bTerminalID;         /* unique ID */
    UWORD wTerminalType;       /* Terminal Type */
    UBYTE bAssocTerminal;      /* Associated Output Terminal ID */
    UBYTE iTerminal;           /* String descriptor */
    UBYTE extra[0];            /* depending on the type, extra bytes may follow */
};

struct UsbVideoOutputTermDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x03) */
    UBYTE bTerminalID;         /* unique ID */
    UWORD wTerminalType;       /* Terminal Type */
    UBYTE bAssocTerminal;      /* Associated Output Terminal ID */
    UBYTE bSourceID;           /* ID of the Unit or Terminal to which this Terminal is connected. */
    UBYTE iTerminal;           /* String descriptor */
    UBYTE extra[0];            /* depending on the type, extra bytes may follow */
};

struct UsbVideoCameraTermDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x04) */
    UBYTE bTerminalID;         /* unique ID */
    UWORD wTerminalType;       /* Terminal Type */
    UBYTE bAssocTerminal;      /* Associated Output Terminal ID */
    UBYTE bSourceID;           /* ID of the Unit or Terminal to which this Terminal is connected. */
    UBYTE iTerminal;           /* String descriptor */
    UBYTE wObjFocalLengthMin0; /* Focal Length minimum low byte */
    UBYTE wObjFocalLengthMin1; /* Focal Length minimum high byte */
    UBYTE wObjFocalLengthMax0; /* Focal Length maximum low byte */
    UBYTE wObjFocalLengthMax1; /* Focal Length maximum high byte */
    UBYTE wOcFocalLength0;     /* Ocular Focal Length low byte */
    UBYTE wOcFocalLength1;     /* Ocular Focal Length high byte */
    UBYTE bControlSize;        /* Size in bytes of an element of the control array */
    UBYTE bmControls[0];       /* Ckontrols bitmap */
};

/* this data structure is of no use, as it's highly variable in size */
struct UsbVideoSelectorUnitDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x05) */
    UBYTE bUnitID;             /* unique ID */
    UBYTE bNrInPins;           /* Number of Input Pins */
    UBYTE baSourceID[0];       /* ID of the Unit or Terminal to which the nth Input Pin of this Selector Unit is connected. */
    UBYTE iSelector;           /* String descriptor */
};

/* this data structure is of no use, as it's highly variable in size */
struct UsbVideoProcessingUnitDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x07) */
    UBYTE bUnitID;             /* unique ID */
    UBYTE bSourceID;           /* ID of the Unit or Terminal to which this Unit is connected */
    UWORD wMaxMultiplier;      /* maximum digital magnification */
    UBYTE bControlSize;        /* Size in bytes of an element of the control array */
    UBYTE bmControls[0];       /* Controls bitmap */
    UBYTE iProcessing;         /* String descriptor */
    UBYTE bmVideoStandards;    /* A Bitmap of all analog video standards supported */
};

/* this data structure is of no use, as it's highly variable in size */
struct UsbVideoExtensionUnitDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x08) */
    UBYTE bUnitID;             /* unique ID */
    ULONG guidExtensionCode;   /* vendor specific */
    UBYTE bNumControls;        /* Number of controls in this extension unit */
    UBYTE bNrInPins;           /* Number of Input Pins */
    UBYTE baSourceID[0];       /* ID of the Unit or Terminal to which the nth Input Pin of this Extension Unit is connected. */
    UBYTE bControlSize;        /* Size in bytes of an element of the control array */
    UBYTE bmControls[0];       /* Controls bitmap */
    UBYTE iExtension;          /* String descriptor */
};
 
/* generic video stuff */
struct UsbVideoInputHeaderIFDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x01) */
    UBYTE bNumFormats;         /* Number of video payload format descriptors following */
    UWORD wTotalLength;        /* Length of descriptor */
    UBYTE bEndpointAddress;    /* Endpoint for bulk or iso video data */
    UBYTE bmInfo;              /* Capabilities bitmap */
    UBYTE bTerminalLink;       /* Terminal linked to this interface */
    UBYTE bStillCaptureMethod; /* Method of still image capture supported */
    UBYTE bTriggerSupport;     /* HW Triggering support */
    UBYTE bTriggerUsage;       /* Trigger behaviour */
    UBYTE bControlSize;        /* Size in bytes of an element of the control array */
    UBYTE bmControls[0];       /* Controls bitmap */
};

struct UsbVideoOutputHeaderIFDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x02) */
    UBYTE bNumFormats;         /* Number of video payload format descriptors following */
    UWORD wTotalLength;        /* Length of descriptor */
    UBYTE bEndpointAddress;    /* Endpoint for bulk or iso video data */
    UBYTE bTerminalLink;       /* Terminal linked to this interface */
    UBYTE bControlSize;        /* Size in bytes of an element of the control array */
    UBYTE bmControls[0];       /* Controls bitmap */
};

/* this data structure is of no use, as it's highly variable in size */
struct UsbVideoStillImageFrameIFDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x03) */
    UBYTE bEndpointAddress;    /* Endpoint for bulk or iso video data */
    UBYTE bNumImageSizePatterns; /* Number of image size patterns of this format */
    UBYTE baImageSizePatterns[0]; /* Array of width/heights */
    UBYTE bNumCompressionPatterns; /* Number of compression patterns of this format */
    UBYTE baCompression[0];    /* Array of compressions */
};

struct UsbVideoColorFormatIFDesc
{
    UBYTE bLength;             /* Size of this descriptor (6) */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x0d) */
    UBYTE bColorPrimaries;     /* Color primaries and reference white */
    UBYTE bTransferChar;       /* Transfer characteristics (gamma) */
    UBYTE bMatrixCoefficients; /* Matrix used to compute luma and chroma */
};

/* MJPEG Video Format stuff */

#define UVMJHF_FRAME_ID      0x01 /* Alternating bit for frame */
#define UVMJHF_END_OF_FRAME  0x02 /* Indicates last fragment of frame */
#define UVMJHF_HAS_PTS       0x04 /* Indicates the presence of the PTS field */
#define UVMJHF_HAS_SCR       0x08 /* Indicates the presence of the SCR field */
#define UVMJHF_STILL_IMAGE   0x20 /* This is part of a still image */
#define UVMJHF_ERROR         0x40 /* Error in the device streaming */
#define UVMJHF_END_OF_HEADER 0x80 /* End of header bytes */

struct UsbVideoMJPEGStreamHeader
{
    UBYTE bHeaderLength;       /* Size of this header */
    UBYTE bmHeader;            /* Bitfield header field */
    UBYTE dwPTS[4];            /* Presentation Time Stamp */
    UBYTE dwSCR[4];            /* Source Clock Reference */
};

struct UsbVideoMJPEGFormatDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x06) */
    UBYTE bFormatIndex;        /* Index of this Format Descriptor */
    UBYTE bNumFrameDescriptors; /* Number of Frame Descriptors following */
    UBYTE bmFlags;             /* Specifies characteristics of this format */
    UBYTE bDefaultFrameIndex;  /* Optimum frame index (used to select resolution) */
    UBYTE bAspectRatioX;       /* X dimension of the picture aspect ratio */
    UBYTE bAspectRatioY;       /* Y dimension of the picture aspect ratio */
    UBYTE bmInterlaceFlags;    /* Specifies interlace information */
    UBYTE bCopyProtect;        /* CP info */
};

struct UsbVideoMJPEGFrameDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x07) */
    UBYTE bFrameIndex;         /* Index of this Frame Descriptor */
    UBYTE bmCapabilites;       /* Capabilities bitmap */
    UBYTE wWidth0;             /* Width low byte */
    UBYTE wWidth1;             /* Width high byte */
    UBYTE wHeight0;            /* Height low byte */
    UBYTE wHeight1;            /* Height high byte */
    UBYTE dwMinBitRate[4];     /* Minimum bitrate at default compression */
    UBYTE dwMaxBitRate[4];     /* Maximum bitrate at default compression */
    UBYTE dwMaxVideoFrameBuf[4]; /* Deprecated */
    UBYTE dwDefaultFrameIval[4]; /* Default frame interval */
    UBYTE bFrameIntervalType;  /* How many frame intervals (continuous or fixed) are supported */
    UBYTE dwFrameInterval[4];  /* Array of frame intervals or min/max/step */
};

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* DEVICES_USB_VIDEO_H */
