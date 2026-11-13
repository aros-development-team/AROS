/**********************************************************
**                                                       **
**      $VER: ClipLoc.h 1.2 (10 dec 1999)                **
**      Datatypes for highlighting text with mouse.      **
**                                                       **
**      © T.Pierron, C.Guilaume. Free software under     **
**      terms of GNU public license.                     **
**                                                       **
**********************************************************/

#ifndef CLIPLOC_H
#define CLIPLOC_H

struct cutcopypast
{
	ULONG startsel;         /* Extreme position of selection */
	ULONG endsel;
	ULONG xp,yp;            /* Position of first char selected */
	void  *line;            /* Line starting the selection */
	ULONG xc,yc;            /* Current selection point */
	void  *cline;           /* Last line selected */
	UBYTE select;           /* See below */
};

/** Clipboard unit for copy/paste operations **/
#define	STD_CLIP_UNIT     PRIMARY_CLIP

/** Type of selection **/
#define	STREAM_TYPE       1     /* Standard line selection */
#define	COLUMN_TYPE       2     /* Columnar-type of selection */
#define	LINE_TYPE         3     /* Whole line selection */
#define	WORD_TYPE         4     /* Selection by words */

/** To access global messages table **/
#define ErrMsg(num)        JanoMessages[ num-ERR_BADOS ]
extern  STRPTR JanoMessages[];

/** Prototypes **/
#ifdef	MEMORY_H
void CBClose     ( void );
BOOL CBWriteFTXT ( LINE *, struct cutcopypast * );
BOOL CBReadCHRS  ( void *, LINE *, ULONG, LONG * );
#endif

void InitLocale    ( void );
void CleanupLocale ( void );

#endif
