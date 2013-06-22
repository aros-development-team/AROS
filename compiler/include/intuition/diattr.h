#ifndef INTUITION_DIATTR_H
#define INTUITION_DIATTR_H

/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DrawInfo attributes
    Lang: english
*/

#include <intuition/screens.h>

#define GDIA_Color       0x00100000
#define GDIA_Pen         0x00200000
#define GDIA_Version     0x00300000
#define GDIA_DirectColor 0x00400000
#define GDIA_NumPens     0x00500000
#define GDIA_Font        0x00600000
#define GDIA_Depth       0x00700000
#define GDIA_ResolutionX 0x00800000
#define GDIA_ResolutionY 0x00900000
#define GDIA_CheckMark   0x00A00000
#define GDIA_MenuKey     0x00B00000

#define DRIPEN_DETAIL        DETAILPEN
#define DRIPEN_BLOCK         BLOCKPEN
#define DRIPEN_TEXT          TEXTPEN
#define DRIPEN_SHINE         SHINEPEN
#define DRIPEN_SHADOW        SHADOWPEN
#define DRIPEN_FILL          FILLPEN
#define DRIPEN_FILLTEXT      FILLTEXTPEN
#define DRIPEN_BACKGROUND    BACKGROUNDPEN
#define DRIPEN_HIGHLIGHTTEXT HIGHLIGHTTEXTPEN
#define DRIPEN_BARDETAIL     BARDETAILPEN
#define DRIPEN_BARBLOCK      BARBLOCKPEN
#define DRIPEN_BARTRIM       BARTRIMPEN

/*
 * The following pens are not implemented in AROS and defined here
 * only for complete source code compatibility.
 * GetDrawInfoAttr() returns error upon attempt to query these pens.
 */
#define DRIPEN_HALFSHINE     12
#define DRIPEN_HALFSHADOW    13
#define DRIPEN_NUMDRIPENS    14

#endif
