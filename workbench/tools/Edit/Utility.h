/**************************************************************
**** Utility.h: Prototypes of nice and helpful functions   ****
**** Free software under GNU license, written in 9/6/2000  ****
**************************************************************/

#ifndef	UTILITY_H
#define	UTILITY_H

#ifndef	INTUITION_INTUITION_H
struct Window;
#endif

/** Very simple SPrintf-like function **/
STRPTR my_SPrintf(STRPTR fmt, APTR data);

typedef struct
{
	ULONG sa_NbArgs;			/* Nb. of WBArg */
	APTR  sa_ArgLst;			/* WBArg * */
	UBYTE sa_Free;				/* Must FreeVec()'ed sa_ArgLst */
}	StartUpArgs;

/** Converts command line arguments into WBArg **/
void ParseArgs(StartUpArgs *, int nb, char **);

#ifndef	UTILITY_C
/** List manipulation **/
void InsertAfter( void *It,void *Src );

void Destroy( void *First, void *p );
#endif

/** Get include file name **/
STRPTR GetIncludeFile( Project, LINE * );

/** Like CopyMem but copy buf from end instead of beg. **/
void MemMove(UBYTE *Src, UWORD Offset, LONG sz);

#ifndef	JANOPREF
/** Pre-computes the 256 first tabstop **/
void init_tabstop(UBYTE ts);
#endif

/** Returns increment to next tabstop **/
UBYTE tabstop(ULONG);

/** Shutdown events coming to the window and change pointer **/
void BusyWindow(struct Window *);

/** Enable receiving events and reset pointer **/
void WakeUp(struct Window *);

/** Simple strings manipulation **/
STRPTR CatPath ( STRPTR dir, STRPTR file );

/** Display an error in title bar & start a countdown **/
void ThrowError    (struct Window *, STRPTR);
void ThrowDOSError (struct Window *, STRPTR);

/** Set a permanent title **/
void SetTitle(struct Window *, STRPTR);

/** Stop countdown msg. and restore original title */
void StopError(struct Window *);

/** Check if path already exists **/
char warn_overwrite( STRPTR path );

/** Write column/line in top of window **/
void draw_info(Project p);

/** Avert user that its file has been modified **/
char warn_modif(Project p);

void show_info(Project p);

/** Simple requester to ask user for a number **/
int get_number( Project p, CONST_STRPTR title, LONG * result );

/** About requester messages **/
extern STRPTR JanoMessages[];
#define	MsgAbout     (JanoMessages + (MSG_ABOUT - ERR_BADOS))

#endif
