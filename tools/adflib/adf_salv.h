#ifndef _ADF_SALV_H
#define _ADF_SALV_H 1

/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_salv.h
 *
 */

#include "adf_str.h"

RETCODE adfReadGenBlock(struct Volume *vol, SECTNUM nSect, struct GenBlock *block);
PREFIX RETCODE adfCheckEntry(struct Volume* vol, SECTNUM nSect, int level);
PREFIX RETCODE adfUndelEntry(struct Volume* vol, SECTNUM parent, SECTNUM nSect);
PREFIX struct List* adfGetDelEnt(struct Volume *vol);
PREFIX void adfFreeDelList(struct List* list);


/*##########################################################################*/
#endif /* _ADF_SALV_H */

