#ifndef ADF_LINK_H
#define ADF_LINK_H 1

/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_link.h
 *
 *  
 */

#include"prefix.h"

PREFIX RETCODE adfBlockPtr2EntryName(struct Volume *vol, SECTNUM nSect, SECTNUM lPar, 
	char **name, long *size);


#endif /* ADF_LINK_H */
/*##########################################################################*/
