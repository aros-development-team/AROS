#ifndef UVC_CLASS_H
#define UVC_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                     Includes for uvc (USB Video) class
 *----------------------------------------------------------------------------
 */

#include "common.h"

#include <devices/usb_video.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "uvc.h"

/* Protos */

struct NepClassUVC * usbAttemptInterfaceBinding(struct NepUVCBase *nh, struct PsdInterface *pif);
struct NepClassUVC * usbForceInterfaceBinding(struct NepUVCBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepUVCBase *nh, struct NepClassUVC *nch);

#endif /* UVC_CLASS_H */
