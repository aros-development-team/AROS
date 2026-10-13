#ifndef ENVOY_SERVICES_H
#define ENVOY_SERVICES_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy services.library and xxx.service public definitions
          (as published in the Envoy developer kit).
*/

#include <exec/types.h>
#include <utility/tagitem.h>

/* Tags for FindServiceA() */
#define FSVC_Dummy              (TAG_USER + 2048)
#define FSVC_Error              (FSVC_Dummy + 0x02)     /* ULONG * : failure code written back       */
#define FSVC_UserName           (FSVC_Dummy + 0x03)     /* STRPTR  : user asking for the service     */
#define FSVC_PassWord           (FSVC_Dummy + 0x04)     /* STRPTR  : that user's password            */

/* Tags for a service's Get/SetServiceAttrsA() */
#define SVCAttrs_Dummy          (TAG_USER + 4096)
#define SVCAttrs_Name           (SVCAttrs_Dummy + 0x01) /* STRPTR : the service's name               */
#define SVCAttrs_Reserved1      (SVCAttrs_Dummy + 0x02) /* private                                   */
#define SVCAttrs_Reserved2      (SVCAttrs_Dummy + 0x03) /* private                                   */
#define SVCAttrs_FullService    (SVCAttrs_Dummy + 0x04) /* BOOL : supports SetServiceAttrsA() and    */
                                                        /*        AttemptShutdown() (Get only)       */
#define SVCAttrs_Reserved3      (SVCAttrs_Dummy + 0x05) /* private                                   */
#define SVCAttrs_Reserved4      (SVCAttrs_Dummy + 0x06) /* private                                   */

/* Tags passed by the Services Manager to a service's StartServiceA() */
#define SSVC_Dummy              (TAG_USER + 8192)
#define SSVC_UserName           (SSVC_Dummy + 0x01)     /* STRPTR : user name from the client        */
#define SSVC_Password           (SSVC_Dummy + 0x02)     /* STRPTR : password from the client         */
#define SSVC_EntityName         (SSVC_Dummy + 0x03)     /* STRPTR : 64-byte buffer the service fills */
                                                        /*          with the entity name to use      */
#define SSVC_HostName           (SSVC_Dummy + 0x04)     /* STRPTR : the client's host name           */

#endif /* ENVOY_SERVICES_H */
