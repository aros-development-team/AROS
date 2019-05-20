/************************************************************
* MultiUser - MultiUser Task/File Support System				*
* ---------------------------------------------------------	*
* Locale Routines															*
* ---------------------------------------------------------	*
* © Copyright 1993-1994 Geert Uytterhoeven						*
* All Rights Reserved.													*
************************************************************/

/*
*       Message Numbers
*/

#define CATCOMP_NUMBERS
#include "strings.h"

struct SecurityBase;

struct LocaleInfo	{
	APTR li_LocaleBase;
	APTR li_Catalog;
};

/*
*       Function Prototypes
*/

extern void OpenLoc(struct SecurityBase *secBase, struct LocaleInfo *li);
extern void CloseLoc(struct LocaleInfo *li);
extern STRPTR GetString(struct SecurityBase *secBase, struct LocaleInfo *li, LONG id);
#define GetLocS(base,li,id) GetString(base,li,id)
extern STRPTR GetLocStr(struct SecurityBase *secBase, LONG id);
#define GetLogStr(base,id) GetLocS(base, &base->LogInfo, id)
