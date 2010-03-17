#ifndef _UAE_INTEGRATION_H_
#define _UAE_INTEGRATION_H_

/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <utility/tagitem.h>

struct JUAE_Launch_Message 
{
    struct Message  ExecMessage;
    STRPTR          ln_Name;
    struct TagItem *tags;
    void           *mempool; 
};

#define J_UAE_PORT "J-UAE Execute"

/*** Prototypes *************************************************************/

BOOL is_68k        (STRPTR filename);
void forward_to_uae(struct TagItem *argsTagList, char *name);
BOOL j_uae_running (void);

#endif /* _UAE_INTEGRATION_H_ */
