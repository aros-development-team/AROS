/**************************************************************
**** diskio.h : prototypes for reading / writing files     ****
**** Free software under GNU license, started on 25.2.2000 ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/

#ifndef DISKIO_H
#define DISKIO_H

#include "Memory.h"

/* This is the fields modified by load_file in Project datatype **
** But because of this function may failed, we must use another **
** datatype and re-fill original structure in case of success   */
typedef	struct
{
	STRPTR filename;			/* Just fill this field */
	STRPTR buffer;
	LINE  *lines;
	ULONG  nblines;
	char   eol;
}	LoadFileArgs;

WORD load_file     ( LoadFileArgs * );
WORD read_file     ( LoadFileArgs *, ULONG * );
BYTE save_file     ( STRPTR, LINE *, unsigned char eol );
BYTE get_full_path ( STRPTR, STRPTR * );

#ifndef	INTUITION_INTUITION_H
struct Window;
#endif

/** Arguments to use with ask_save and ask_load function **/
typedef	struct
{
	STRPTR dir;				/* Directory name */
	STRPTR file;			/* File name */
	UBYTE  modifmark;		/* True if file has modif. mark */
}	AskArgs;

/** Split a name into two pointers **/
void split_path( AskArgs *, STRPTR *, STRPTR * );

/** Prompt user for a filename **/
STRPTR ask_save(struct Window *, AskArgs *, CONST_STRPTR);

/** Like previous but with ASL_LOAD. if setfile is 1 File gadget will
*** be initially fill. Otherwise it will be empty and multi-select will
*** be enabled. Thus a (StartUpArgs *) will be returned instead (utility.h).
**/
STRPTR ask_load(struct Window *, AskArgs *, BYTE setfile, CONST_STRPTR asltitle);

void free_diskio_alloc(void);

#endif
