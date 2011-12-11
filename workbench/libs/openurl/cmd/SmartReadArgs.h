#ifndef SMARTREADARGS_H
#define SMARTREADARGS_H
/*
 * SmartReadArgs.h -- CLI/Workbench transparent ReadArgs()
 *
 * $VER: SmartReadArgs.h 1.3 (2.9.98)
 *
 * Copyright 1998 by Thomas Aglassinger <agi@sbox.tu-graz.ac.at>
 *
 * Based on ExtReadArgs Copyright 1994,1995 by Stefan Ruppert
 */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef DOS_RDARGS_H
#include <dos/rdargs.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef WORKBENCH_STARTUP_H
#include <workbench/startup.h>
#endif

#include <SDI/SDI_compiler.h>

struct SmartArgs
{
   /* Readargs template */
   CONST_STRPTR sa_Template;

   /* Pointer to the parameter array */
   IPTR *sa_Parameter;

   /* Specifies which parameter should contain the files passed with the
    * WBStartup message; use -1 for none */
   LONG sa_FileParameter;

   /* Window description to open, if this is started from workbench or NULL
    * for no window. If a WINDOW tooltype is specifies in the icon, it is
    * used instead. */
   STRPTR sa_Window;

   /* Use this RDArgs structure instead of allocating a new. This can be
    * used to specify extended help. */
   struct RDArgs *sa_RDArgs;

   /* Some flags, see below for possible values */
   ULONG sa_Flags;

   /* Pointer to the RDArgs structure returned from the ReadArgs() call */
   struct RDArgs *sa_FreeArgs;

   /* Pointer to a buffer to use for the WBStartup. If this is NULL, a
    * buffer is allocated automatically. */
   STRPTR sa_Buffer;

   /* Size of the above buffer. If buffer == NULL, this size is used to
    * allocate the buffer. */
   ULONG sa_BufferSize;

   /* The fields below are for internal use by SmartReadArgs() only */

   STRPTR sa_ActualPtr;       /* Current write location in sa_Buffer */
   STRPTR sa_EndPtr;          /* Pointer to the end of sa_Buffer */
   BPTR sa_WindowFH;          /* Window filehandle, MUST BE NULL */
   BPTR sa_OldOutput;         /* Old output filehandle MUST BE NULL */
   BPTR sa_OldInput;          /* Old input filehandle MUST BE NULL */
   struct WBArg *sa_WBArg;    /* wbargs for erda_FileParameter */
   LONG sa_NumArgs;           /* number of wbargs */
};

#define SA_MINIMUM_BUFFER_SIZE        1024

/* Flags to be used with SmartArgs.sa_Flags */

/* Indicate, that the program was started from Workbench */
#define SAF_WORKBENCH    (1<<0)

/* SmartArgs.sa_RDArgs is allocated by AllocDosObject() */
#define SAF_ALLOCRDARGS  (1<<1)

/* SmartArgs.sa_Buffer is allocated by SmartReadArgs() */
#define SAF_ALLOCBUFFER  (1<<2)

/* SmartArgs.sa_Window is allocated by SmartReadArgs() */
#define SAF_ALLOCWINDOW  (1<<3)

/* ------------------------------ prototypes ------------------------------ */

LONG SmartReadArgs(struct WBStartup *wb_startup, struct SmartArgs * smart_args);
void SmartFreeArgs(struct SmartArgs *smart_args);

#endif /* !SMARTREADARGS_H */
