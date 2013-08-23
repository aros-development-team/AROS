/***************************************************************************

 NFloattext.mcc - New Floattext MUI Custom Class
 Registered MUI class, Serial Number: 1d51 (0x9d5100a1 to 0x9d5100aF)

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2013 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <proto/exec.h>

/******************************************************************************/
/*                                                                            */
/* MCC/MCP name and version                                                   */
/*                                                                            */
/* ATTENTION:  The FIRST LETTER of NAME MUST be UPPERCASE                     */
/*                                                                            */
/******************************************************************************/

#include "private.h"
#include "version.h"

/******************************************************************************/
/* include the minimal startup code to be able to start the class from a      */
/* shell without crashing the system                                          */
/******************************************************************************/
#include "shellstart.c"

#define	VERSION             LIB_VERSION
#define	REVISION            LIB_REVISION

#define CLASS               MUIC_NFloattext
#define SUPERCLASS          MUIC_NList

#define	INSTDATA            NFTData

#define USERLIBID           CLASS " " LIB_REV_STRING " [" SYSTEMSHORT "/" CPU "] (" LIB_DATE ") " LIB_COPYRIGHT
#define MASTERVERSION       19

#define USEDCLASSES used_mccs
static const char *used_mccs[] = { "NList.mcc", "NListview.mcc", NULL };

#define USEDCLASSESP used_mcps
static const char *used_mcps[] = { "NListviews.mcp", NULL };

#define MIN_STACKSIZE       8192

/******************************************************************************/
/* define the functions used by the startup code ahead of including mccinit.c */
/******************************************************************************/

/******************************************************************************/
/* include the lib startup code for the mcc/mcp  (and muimaster inlines)      */
/******************************************************************************/
#include "mccinit.c"

/******************************************************************************/
/* define all implementations of our user functions                           */
/******************************************************************************/
