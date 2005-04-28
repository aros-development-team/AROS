/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id: C_h.sd,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

/****************************************************************
   This file was created automatically by `FlexCat V1.3'
****************************************************************/

#ifndef BetterString_mcp_LOCALE_H
#define BetterString_mcp_LOCALE_H
#include <exec/types.h>

#define MSG_Space ((APTR)1)

extern char *GetStr(APTR);
extern char *GetStripStr(APTR);
extern void OpenCat(void);
extern void CloseCat(void);

extern const APTR _MSG_Label_Active;
#define MSG_Label_Active ((APTR) &_MSG_Label_Active)
extern const APTR _MSG_Label_Background;
#define MSG_Label_Background ((APTR) &_MSG_Label_Background)
extern const APTR _MSG_Label_Cursor;
#define MSG_Label_Cursor ((APTR) &_MSG_Label_Cursor)
extern const APTR _MSG_Label_Fonts;
#define MSG_Label_Fonts ((APTR) &_MSG_Label_Fonts)
extern const APTR _MSG_Label_Frame;
#define MSG_Label_Frame ((APTR) &_MSG_Label_Frame)
extern const APTR _MSG_Label_Inactive;
#define MSG_Label_Inactive ((APTR) &_MSG_Label_Inactive)
extern const APTR _MSG_Label_Marked;
#define MSG_Label_Marked ((APTR) &_MSG_Label_Marked)
extern const APTR _MSG_Label_Text;
#define MSG_Label_Text ((APTR) &_MSG_Label_Text)
extern const APTR _MSG_String_TestString;
#define MSG_String_TestString ((APTR) &_MSG_String_TestString)

#endif /* BetterString_mcp_LOCALE_H */
