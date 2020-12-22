#ifndef IA_PACKAGES_H
#define IA_PACKAGES_H
/*
    Copyright © 2018-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

void PACKAGES_InitSupport(struct List *);
void PACKAGES_AddCoreSkipPaths(struct List *);
void PACKAGES_DoInstall(Class * CLASS, Object * self);

#endif /* IA_PACKAGES_H */