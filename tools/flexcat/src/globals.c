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

#include "flexcat.h"
#include "SDI_compiler.h"

/// Globals
char           *BaseName = NULL;        /* Basename of catalog description */
const char     *Language = "english";   /* Language of catalog description */
int             CatVersion = 0;         /* Version of catalog to be opened */
int             CatRevision = 0;        /* Revision of catalog to be opened */
int             NumStrings = 0;         /* Number of catalog strings */
char           *ScanFile;               /* File currently scanned */
int             ScanLine;               /* Line currently scanned */
int             GlobalReturnCode = 0;   /* Will be 5 if warnings appear */
int             NumberOfWarnings = 0;   /* We count warnings to be smart
                                           and avoid Beep bombing, but
                                           call DisplayBeep() only once */
int             buffer_size = 2048;     /* Size of the I/O buffer */

const char USED_VAR versionCookie[] = VERSTAG;

///
