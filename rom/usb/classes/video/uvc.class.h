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

IPTR uvc_method(struct NepUVCBase *nh, ULONG method, IPTR *args);

STRPTR uvc_vhi_bind(struct NepClassUVC *nch, struct Library *ps);
void uvc_vhi_config(struct NepUVCBase *nh);
void uvc_vhi_release(struct NepClassUVC *nch);
BOOL uvc_vhi_expunge(struct NepUVCBase *nh);

#endif /* UVC_CLASS_H */
