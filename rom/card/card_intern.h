/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for card.resource
    Lang: english
*/

#ifndef CARD_INTERN_H
#define CARD_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef EXEC_INTERRUPTS_H
#include <exec/interrupts.h>
#endif
#ifndef RESOURCES_CARD_H
#include <resources/card.h>
#endif
#ifndef PROTO_CARDRES_H
#include <proto/cardres.h>
#endif

struct CardResource
{
    struct Library crb_LibNode;
    struct CardMemoryMap cmm;
};

#endif //CARD_INTERN_H
