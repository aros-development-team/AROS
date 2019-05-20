/************************************************************
* MultiUser - MultiUser Task/File Support System				*
* ---------------------------------------------------------	*
* Group Information Management										*
* ---------------------------------------------------------	*
* © Copyright 1993-1994 Geert Uytterhoeven						*
* All Rights Reserved.													*
************************************************************/


#include <libraries/mufs.h>

/*
 *		Private Group Information Structure
 *
 *		This is a sub class of the Public Group Information Structure
 */

struct secPrivGroupInfo {
	struct secGroupInfo Pub;					/* The public part */
	STRPTR Pattern;                     /* Pattern matching temp */
	ULONG Count;								/* last info */
};
