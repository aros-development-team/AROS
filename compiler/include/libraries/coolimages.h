#ifndef LIBRARIES_COOLIMAGES_H
#define LIBRARIES_COOLIMAGES_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define SHARED_COOLIMAGES_LIBRARY 1

#ifndef LINKLIBS_COOLIMAGES_H
#include <linklibs/coolimages.h>
#endif

#define COOL_SAVEIMAGE_ID       0
#define COOL_LOADIMAGE_ID       1
#define COOL_USEIMAGE_ID    	2
#define COOL_CANCELIMAGE_ID  	3
#define COOL_DOTIMAGE_ID    	4
#define COOL_DOTIMAGE2_ID   	5
#define COOL_WARNIMAGE_ID   	6
#define COOL_DISKIMAGE_ID   	7
#define COOL_SWITCHIMAGE_ID 	8
#define COOL_MONITORIMAGE_ID	9
#define COOL_MOUSEIMAGE_ID  	10
#define COOL_INFOIMAGE_ID   	11
#define COOL_ASKIMAGE_ID    	12
#define COOL_KEYIMAGE_ID    	13
#define COOL_CLOCKIMAGE_ID  	14
#define COOL_FLAGIMAGE_ID   	15
#define COOL_HEADIMAGE_ID   	16
#define COOL_WINDOWIMAGE_ID 	17
#define COOL_KBDIMAGE_ID    	18

#define DEF_COOLIMAGEHEIGHT 16

#define COOLIMAGECLASS      "coolimageclass"
#define COOLBUTTONGCLASS    "coolbuttongclass"

#endif /* LIBRARIES_COOLIMAGES_H */
