/*
 * $Id$
 *
 * Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
 * Copyright (C) 2002-2010 by the FlexCat Open Source Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef  FLEXCAT_READPREFS_H
#define  FLEXCAT_READPREFS_H

/* Functions */
char            ReadPrefs ( void );

/* Variables */
extern char    *prefs_sddir;
extern int      WarnCTGaps;
extern int      NoOptim;
extern int      Fill;
extern int      DoExpunge;
extern int      NoBeep;
extern int      Quiet;
extern int      LANGToLower;
extern int      NoBufferedIO;
extern int      Modified;
extern char     Msg_New[MAX_NEW_STR_LEN];
extern int      CopyNEWs;
extern char     Old_Msg_New[MAX_NEW_STR_LEN];
extern char     DestCodeset[MAX_NEW_STR_LEN];

#endif  /* FLEXCAT_READPREFS_H */
