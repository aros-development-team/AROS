/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __ZUNE_H__
#define __ZUNE_H__

#ifdef _ZUNE_CORE
#error Zune core must not include this file
#endif

#include <zune/boopsi.h>
#include <zune/preprotos.h>
#ifndef __AROS__
#include <gdk/gdktypes.h>
#endif
#include <zune/premui.h>
#include <stdio.h>

#include <zune/zune_common.h>

/*** API added in Zune ***/

/* Dataspace new ASCII API */

#define MUIM_Dataspace_ReadASCII  0x80423367
#define MUIM_Dataspace_WriteASCII 0x80423368
#define MUIM_Dataspace_AddString  0x80423369
#define MUIM_Dataspace_AddInt     0x8042336b
#define MUIM_Dataspace_AddFloat   0x8042336c
#define MUIM_Dataspace_FindString 0x8042336a
#define MUIM_Dataspace_FindInt    0x8042336d
#define MUIM_Dataspace_FindFloat  0x8042336e

struct  MUIP_Dataspace_ReadASCII  { ULONG MethodID; FILE *infile; /* ... */ };
struct  MUIP_Dataspace_WriteASCII { ULONG MethodID; FILE *outfile; /* ... */ };
struct  MUIP_Dataspace_AddString  { ULONG MethodID; STRPTR objID; STRPTR key; STRPTR value; };
struct  MUIP_Dataspace_AddInt     { ULONG MethodID; STRPTR objID; STRPTR key; LONG   value; };
struct  MUIP_Dataspace_AddFloat   { ULONG MethodID; STRPTR objID; STRPTR key; FLOAT  value; };
struct  MUIP_Dataspace_FindString { ULONG MethodID; STRPTR objID; STRPTR key; };
struct  MUIP_Dataspace_FindInt    { ULONG MethodID; STRPTR objID; STRPTR key; };
struct  MUIP_Dataspace_FindFloat  { ULONG MethodID; STRPTR objID; STRPTR key; FLOAT *value;};


/* Configdata - to load/save GUI prefs */

#define MUIM_Configdata_AddPens          0x8042336f
#define MUIM_Configdata_AddWindows       0x80423370
#define MUIM_Configdata_AddGroups        0x80423371
#define MUIM_Configdata_AddButtons       0x80423372
#define MUIM_Configdata_AddCycles        0x80423373
#define MUIM_Configdata_AddSliders       0x80423374
#define MUIM_Configdata_AddScrollbars    0x80423375
#define MUIM_Configdata_AddListviews     0x80423376
#define MUIM_Configdata_AddStrings       0x80423377
#define MUIM_Configdata_AddNavigation    0x80423378
#define MUIM_Configdata_AddSpecial       0x80423379
#define MUIM_Configdata_FindPens          0x8042337a
#define MUIM_Configdata_FindWindows       0x8042337b
#define MUIM_Configdata_FindGroups        0x8042337c
#define MUIM_Configdata_FindButtons       0x8042337d
#define MUIM_Configdata_FindCycles        0x8042337e
#define MUIM_Configdata_FindSliders       0x8042337f
#define MUIM_Configdata_FindScrollbars    0x80423380
#define MUIM_Configdata_FindListviews     0x80423381
#define MUIM_Configdata_FindStrings       0x80423382
#define MUIM_Configdata_FindNavigation    0x80423383
#define MUIM_Configdata_FindSpecial       0x80423384

struct ZunePrefs;

struct MUIP_Configdata_AddPens        { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_AddWindows     { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_AddGroups      { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_AddButtons     { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_AddCycles      { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_AddSliders     { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_AddScrollbars  { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_AddListviews   { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_AddStrings     { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_AddNavigation  { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_AddSpecial     { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_FindPens       { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_FindWindows    { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_FindGroups     { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_FindButtons    { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_FindCycles     { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_FindSliders    { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_FindScrollbars { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_FindListviews  { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_FindStrings    { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_FindNavigation { ULONG MethodID; struct ZunePrefs *prefs; };
struct MUIP_Configdata_FindSpecial    { ULONG MethodID; struct ZunePrefs *prefs; };


#endif /* __ZUNE_H__ */
