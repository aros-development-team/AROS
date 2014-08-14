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

#ifndef  FLEXCAT_GLOBALS_H
#define  FLEXCAT_GLOBALS_H

/* Variables */
extern int      buffer_size;
extern char    *ScanFile;
extern int      ScanLine;
extern char    *BaseName;
extern int      LengthBytes;
extern int      NumStrings;
extern char    *Language;
extern int      CatVersion;
extern int      CatRevision;
extern int      GlobalReturnCode;
extern int      NumberOfWarnings;

#endif  /* FLEXCAT_GLOBALS_H */
