#ifndef _IDENTIFY_H_
#define _IDENTIFY_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

AROS_UFP3
(
    struct DiskObject *, FindDefaultIcon,
    AROS_UFPA(struct Hook *,            hook,     A0),
    AROS_UFPA(APTR,                     reserved, A2),
    AROS_UFPA(struct IconIdentifyMsg *, iim,      A1) 
);

#endif /* _IDENTIFY_H_ */
