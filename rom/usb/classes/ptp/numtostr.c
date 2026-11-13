
#include "numtostr.h"

#include <proto/poseidon.h>

#define PsdBase nch->nch_Base

/* /// "Operation Codes" */
const struct PTPIDMap opcodemap[] =
{
    { 0x1000, "Undefined" },
    { 0x1001, "GetDeviceInfo" },
    { 0x1002, "OpenSession" },
    { 0x1003, "CloseSession" },
    { 0x1004, "GetStorageIDs" },
    { 0x1005, "GetStorageInfo" },
    { 0x1006, "GetNumObjects" },
    { 0x1007, "GetObjectHandles" },
    { 0x1008, "GetObjectInfo" },
    { 0x1009, "GetObject" },
    { 0x100A, "GetThumb" },
    { 0x100B, "DeleteObject" },
    { 0x100C, "SendObjectInfo" },
    { 0x100D, "SendObject" },
    { 0x100E, "InitiateCapture" },
    { 0x100F, "FormatStore" },
    { 0x1010, "ResetDevice" },
    { 0x1011, "SelfTest" },
    { 0x1012, "SetObjectProtection" },
    { 0x1013, "PowerDown" },
    { 0x1014, "GetDevicePropDesc" },
    { 0x1015, "GetDevicePropValue" },
    { 0x1016, "SetDevicePropValue" },
    { 0x1017, "ResetDevicePropValue" },
    { 0x1018, "TerminateOpenCapture" },
    { 0x1019, "MoveObject" },
    { 0x101A, "CopyObject" },
    { 0x101B, "GetPartialObject" },
    { 0x101C, "InitiateOpenCapture" },
    { 0x0000, NULL }
};
/* \\\ */

/* /// "Response Codes" */
const struct PTPIDMap rescodemap[] =
{
    { 0x2000, "Undefined" },
    { 0x2001, "OK" },
    { 0x2002, "General Error" },
    { 0x2003, "Session Not Open" },
    { 0x2004, "Invalid TransactionID" },
    { 0x2005, "Operation Not Supported" },
    { 0x2006, "Parameter Not Supported" },
    { 0x2007, "Incomplete Transfer" },
    { 0x2008, "Invalid StorageID" },
    { 0x2009, "Invalid ObjectHandle" },
    { 0x200A, "DeviceProp Not Supported" },
    { 0x200B, "Invalid ObjectFormatCode" },
    { 0x200C, "Store Full" },
    { 0x200D, "Object WriteProtected" },
    { 0x200E, "Store Read-Only" },
    { 0x200F, "Access Denied" },
    { 0x2010, "No Thumbnail Present" },
    { 0x2011, "SelfTest Failed" },
    { 0x2012, "Partial Deletion" },
    { 0x2013, "Store Not Available" },
    { 0x2014, "Specification By Format Unsupported" },
    { 0x2015, "No Valid ObjectInfo" },
    { 0x2016, "Invalid Code Format" },
    { 0x2017, "Unknown Vendor Code" },
    { 0x2018, "Capture Already Terminated" },
    { 0x2019, "Device Busy" },
    { 0x201A, "Invalid ParentObject" },
    { 0x201B, "Invalid DeviceProp Format" },
    { 0x201C, "Invalid DeviceProp Value" },
    { 0x201D, "Invalid Parameter" },
    { 0x201E, "Session Already Open" },
    { 0x201F, "Transaction Cancelled" },
    { 0x2020, "Specification of Destination Unsupported" },
    { 0x0000, NULL }
};
/* \\\ */

#if 0
/* /// "Object Format Codes" */
const struct PTPIDMap objectfmtcodemap[] =
{
    { 0x3000, "Undefined" },
    { 0x3001, "Association" },
    { 0x3002, "Script" },
    { 0x3003, "Executable" },
    { 0x3004, "Text" },
    { 0x3005, "HTML" },
    { 0x3006, "DPOF" },
    { 0x3007, "AIFF" },
    { 0x3008, "WAV" },
    { 0x3009, "MP3" },
    { 0x300A, "AVI" },
    { 0x300B, "MPEG" },
    { 0x300C, "ASF" },
    { 0x3800, "Undefined2" },
    { 0x3801, "EXIF/JPEG" },
    { 0x3802, "TIFF/EP" },
    { 0x3803, "FlashPix" },
    { 0x3804, "BMP" },
    { 0x3805, "CIFF" },
    { 0x3806, "Undefined3" },
    { 0x3807, "GIF" },
    { 0x3808, "JFIF" },
    { 0x3809, "PCD" },
    { 0x380A, "PICT" },
    { 0x380B, "PNG" },
    { 0x380C, "Undefined4" },
    { 0x380D, "TIFF" },
    { 0x380E, "TIFF/IT" },
    { 0x380F, "JP2" },
    { 0x3810, "JPX" },
    { 0x0000, NULL }
};
/* \\\ */

/* /// "Event Codes" */
const struct PTPIDMap eventcodemap[] =
{
    { 0x4000, "Undefined" },
    { 0x4001, "CancelTransaction" },
    { 0x4002, "ObjectAdded" },
    { 0x4003, "ObjectRemoved" },
    { 0x4004, "StoreAdded" },
    { 0x4005, "StoreRemoved" },
    { 0x4006, "DevicePropChanged" },
    { 0x4007, "ObjectInfoChanged" },
    { 0x4008, "DeviceInfoChanged" },
    { 0x4009, "RequestObjectTransfer" },
    { 0x400A, "StoreFull" },
    { 0x400B, "DeviceReset" },
    { 0x400C, "StorageInfoChanged" },
    { 0x400D, "CaptureComplete" },
    { 0x400E, "UnreportedStatus" },
    { 0x0000, NULL }
};
/* \\\ */

/* /// "Device Property Codes" */
const struct PTPIDMap devicepropcodemap[] =
{
    { 0x5000, "Undefined" },
    { 0x5001, "BatteryLevel" },
    { 0x5002, "FunctionalMode" },
    { 0x5003, "ImageSize" },
    { 0x5004, "CompressionSetting" },
    { 0x5005, "WhiteBalance" },
    { 0x5006, "RGB Gain" },
    { 0x5007, "F-Number" },
    { 0x5008, "FocalLength" },
    { 0x5009, "FocusDistance" },
    { 0x500A, "FocusMode" },
    { 0x500B, "ExposureMeteringMode" },
    { 0x500C, "FlashMode" },
    { 0x500D, "ExposureTime" },
    { 0x500E, "ExposureProgramMode" },
    { 0x500F, "ExposureIndex" },
    { 0x5010, "ExposureBiasCompensation" },
    { 0x5011, "DateTime" },
    { 0x5012, "CaptureDelay" },
    { 0x5013, "StillCaptureMode" },
    { 0x5014, "Contrast" },
    { 0x5015, "Sharpness" },
    { 0x5016, "DigitalZoom" },
    { 0x5017, "EffectMode" },
    { 0x5018, "BurstNumber" },
    { 0x5019, "BurstInterval" },
    { 0x501A, "TimelapseNumber" },
    { 0x501B, "TimelapseInterval" },
    { 0x501C, "FocusMeteringMode" },
    { 0x501D, "UploadURL" },
    { 0x501E, "Artist" },
    { 0x501F, "CopyrightInfo" },
    { 0x0000, NULL }
};
/* \\\ */
#endif

/* /// "nNumToStr()" */
STRPTR nNumToStr(struct NepClassPTP *nch, UWORD type, ULONG id, STRPTR defstr)
{
    const struct PTPIDMap *pim = NULL;

    switch(type)
    {
        case NTS_OPCODE:
            pim = opcodemap;
            break;

        case NTS_RESPCODE:
            pim = rescodemap;
            break;

        //case NTS_OBJECTFMTCODE:
        //    pim = objectfmtcodemap;
        //    break;

        //case NTS_EVENTCODE:
        //    pim = eventcodemap;
        //    break;

        //case NTS_DEVICEPROPCODE:
        //    pim = devicepropcodemap;
        //    break;
    }
    if(pim)
    {
        while(pim->pim_String)
        {
            if(pim->pim_ID == id)
            {
                return(pim->pim_String);
            }
            pim++;
        }
    }
    return(defstr);
}
/* \\\ */
