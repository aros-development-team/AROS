/*** ProtoTypes.h : Load standard Amiga prototypes declaration
**** Written by T.Pierron 27/5/2000
***/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/asl.h>
#include <proto/gadtools.h>
#include <proto/keymap.h>
#include <proto/locale.h>
#include <proto/diskfont.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	PR_EXEC			1
#define	PR_DOS			2
#define	PR_INTUITION	4
#define	PR_ASL			8

#if ((INC_PROTOS) & PR_EXEC)
#include <clib/exec_protos.h>
#endif

#if ((INC_PROTOS) & PR_DOS)
#include <clib/dos_protos.h>
#endif

#if ((INC_PROTOS) & PR_INTUITION)
#include <clib/intuition_protos.h>
#endif

#if ((INC_PROTOS) & PR_ASL)
#include <clib/asl_protos.h>
#endif
