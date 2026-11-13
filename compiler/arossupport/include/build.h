#ifndef AROS_BUILD_H
#define AROS_BUILD_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define AROS_BUILD_TYPE_PERSONAL  (0)
#define AROS_BUILD_TYPE_NIGHTLY   (1)
#define AROS_BUILD_TYPE_SNAPSHOT  (2)
#define AROS_BUILD_TYPE_MILESTONE (3)
#define AROS_BUILD_TYPE_RELEASE   (4)

#ifndef AROS_BUILD_TYPE
#   define AROS_BUILD_TYPE AROS_BUILD_TYPE_PERSONAL
#endif 

#endif /* AROS_BUILD_H */
