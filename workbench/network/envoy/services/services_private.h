#ifndef SERVICES_PRIVATE_H
#define SERVICES_PRIVATE_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: services.library - the service list shared between the library,
          the Services Manager and the configuration editor. These are the
          private vectors of the original library (re/spec/services-accounts.md
          §3), with an interface of our own: the original's private vectors
          are not a compatibility target.
*/

#include <exec/nodes.h>
#include <exec/types.h>
#include <envoy/services.h>

#define SVCNODE_PATHSIZE        256
#define SVCNODE_NAMESIZE        64

struct ServiceNode
{
    struct Node sn_Node;
    char        sn_Path[SVCNODE_PATHSIZE];  /* the .service library, as in the prefs file   */
    char        sn_Name[SVCNODE_NAMESIZE];  /* the service name clients ask for             */
    ULONG       sn_Flags;                   /* SNF_#?                                       */
};

#define SNB_ACTIVE              0           /* may be started                               */
#define SNB_TEMP                1           /* removed by the next UnlockServiceList()      */
#define SNF_ACTIVE              (1 << SNB_ACTIVE)
#define SNF_TEMP                (1 << SNB_TEMP)

/* Tags for GetServiceAttrsA()/SetServiceAttrsA()/AddService() of
 * services.library. Get: ti_Data points to the IPTR that receives the value
 * (strings as pointers into the node, valid while the list is locked).
 * Set/Add: ti_Data is the value. */
#define SVCL_Flags              (SVCAttrs_Dummy + 1)    /* ULONG : sn_Flags                 */
#define SVCL_Name               (SVCAttrs_Dummy + 2)    /* STRPTR (Get only)                */
#define SVCL_Path               (SVCAttrs_Dummy + 3)    /* STRPTR (Get only)                */
#define SVCL_Active             (SVCAttrs_Dummy + 4)    /* BOOL                             */
#define SVCL_Temp               (SVCAttrs_Dummy + 5)    /* BOOL                             */

/* Where the Services Manager reads the configuration */
#define SERVICES_PREFS_ENV      "ENV:Envoy/services.prefs"
#define SERVICES_PREFS_ENVARC   "ENVARC:Envoy/services.prefs"

/* The public entity of the Services Manager */
#define SERVICES_MANAGER_ENTITY "Services Manager"

#endif /* SERVICES_PRIVATE_H */
