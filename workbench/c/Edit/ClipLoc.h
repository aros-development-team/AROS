/**************************************************************
**** Clip.h : datatypes for highlighting text with mouse   ****
**** Free software under GNU license, started on 31.3.2000 ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/


#ifndef CLIPLOC_H
#define CLIPLOC_H

struct cutcopypast
{
	ULONG startsel;			/* Extreme position of selection */
	ULONG endsel;
	ULONG xp,yp;				/* Position of first char selected */
	void  *line;				/* Line starting the selection */
	ULONG xc,yc;				/* Current selection point */
	void  *cline;				/* Last line selected */
	UBYTE select;				/* See below */
};

/** type of selection **/
#define	STREAM_TYPE			1		/* Standard line selection */
#define	COLUMN_TYPE			2		/* Columnar-type of selection */
#define	LINE_TYPE			3		/* Whole line selection */
#define	WORD_TYPE			4		/* Selection by words */

/** To access global error message table **/
#define ErrMsg(num)		Errors[ num-ERR_BADOS ]
extern  UBYTE *Errors[];

/** Prototypes **/
#ifdef	MEMORY_H
struct IOClipReq	*CBOpen		( ULONG );
void					CBClose		(struct IOClipReq *);
int					CBWriteFTXT	(struct IOClipReq *, LINE *, struct cutcopypast *);
int					CBQueryFTXT	(struct IOClipReq *);
BOOL					CBReadCHRS	(struct IOClipReq *, void *, LINE *, ULONG, LONG *);
void					CBReadDone	(struct IOClipReq *);
#endif

void jano_local(void);
void free_locale(void);

#endif
