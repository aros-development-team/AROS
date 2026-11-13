/****************************************************************************
**  File:       pipedebug.h
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.1
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
*/

#define   DEBUGOPEN_MAXNAMELEN   108

#define   DEBUG_CON_NAME         "CON:10/30/300/100/pipe-handler DEBUG"



#define   OS(s)   OutStr  ((s),  DebugFH)
#define   NL      OutStr  ("\n", DebugFH)
#define   OL(n)   OutLONG ((n),  DebugFH)



extern BPTR  DebugFH;



extern int   InitDebugIO    ( BYTE NodePri );
extern void  CleanupDebugIO ( void );
extern BPTR  DebugOpen      ( char  *name, int mode );
extern void  DebugClose     ( BPTR fh );
extern int   DebugWrite     ( BPTR fh, BYTE *buf, ULONG len );
extern void  OutStr         ( char *str, BPTR fh );
extern void  OutLONG        ( ULONG n, BPTR fh );
