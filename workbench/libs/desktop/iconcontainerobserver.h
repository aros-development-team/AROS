/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#ifndef ICONCONTAINER_OBSERVER_H
#    define ICONCONTAINER_OBSERVER_H

#    include "observer.h"

#    define ICOA_Directory            TAG_USER+1
#    define ICOA_InTree               TAG_USER+2

#    define ICOM_AddIcons             TAG_USER+12

struct IconContainerObserverClassData
{
    UBYTE          *directory;

// this lock is ONLY valid before the object is added
// to the application tree - that is, before OA_InTree
// is set to TRUE, and before a scan request message is
// sent to the handler. The lock will be unlocked when
// the handler deems that scanning is complete.
    BPTR            dirLock;
    Object         *desktop;
};

struct icoAddIcon
{
    Msg             methodID;
    ULONG           wsr_Results;
    struct SingleResult *wsr_ResultsArray;
};

#endif
