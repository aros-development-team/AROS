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

#include <stdarg.h>
#include "flexcat.h"
#include "readprefs.h"
#include "globals.h"
#include "utils.h"

/// ShowError

/* This shows an error message and quits. */

void ShowError(const char *msg, ...)
{
  va_list args;
  char header[256];
  char message[256];

  snprintf(header, sizeof(header), MSG_ERR_ERROR, ScanFile, ScanLine);
  va_start(args, msg);
  vsnprintf(message, sizeof(message), msg, args);
  va_end(args);
  fprintf(stderr, "%s %s\n", header, message);

#ifdef AMIGA
  NumberOfWarnings++;
#endif

  MyExit(10);
}

///
/// ShowErrorQuick

/* Same as ShowError but in this case we omit any line number. */

void ShowErrorQuick(const char *msg, ...)
{
  va_list args;
  char header[256];
  char message[256];

  snprintf(header, sizeof(header), MSG_ERR_ERROR_QUICK, ScanFile);
  va_start(args, msg);
  vsnprintf(message, sizeof(message), msg, args);
  va_end(args);
  fprintf(stderr, "%s %s\n", header, message);

#ifdef AMIGA
  NumberOfWarnings++;
#endif

  MyExit ( 10 );
}

///
/// MemError

/* This shows the 'Memory error' message. */

void MemError(void)
{
  ShowError(MSG_ERR_NOMEMORY);
}

///
/// ShowWarn

/* This shows a warning. */

void ShowWarn(const char *msg, ...)
{
  if(!Quiet)
  {
    va_list args;
    char header[256];
    char message[256];

    snprintf(header, sizeof(header), MSG_ERR_WARNING, ScanFile, ScanLine);
    va_start(args, msg);
    vsnprintf(message, sizeof(message), msg, args);
    va_end(args);
    fprintf(stderr, "%s %s\n", header, message);
  }

  NumberOfWarnings++;
  GlobalReturnCode = 5;
}

///
/// ShowWarnQuick

/* This shows a warning without line number. */

void ShowWarnQuick(const char *msg, ...)
{
  if(!Quiet)
  {
    va_list args;
    char header[256];
    char message[256];

    snprintf(header, sizeof(header), MSG_ERR_WARNING_QUICK, ScanFile);
    va_start(args, msg);
    vsnprintf(message, sizeof(message), msg, args);
    va_end(args);
    fprintf(stderr, "%s %s\n", header, message);
  }

  NumberOfWarnings++;
  GlobalReturnCode = 5;
}

///
