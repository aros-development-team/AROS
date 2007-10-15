/*

    (C) 1995-2001 AROS - The Amiga Research OS
    (C) 2002-2005 Harry Sintonen
    (C) 2005-2007 Pavel Fedin
    $Id: Assign.c,v 1.6 2007/09/21 06:51:45 sonic_amiga Exp $
 
    Desc: Assign CLI command
    Lang: English
*/

#define AROS_ALMOST_COMPATIBLE

#ifdef __AROS__
#include <aros/asmcall.h>
#include <proto/arossupport.h>
#else
#include <clib/debug_protos.h>
#endif

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/exall.h>
#include <utility/tagitem.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>

#define	DEBUG_ASSIGN(x)

/******************************************************************************
 
 
    NAME
 
        Assign [(name):] [{(target)}] [LIST] [EXISTS] [DISMOUNT] [DEFER]
	       [PATH] [ADD] [REMOVE] [VOLS] [DIRS] [DEVICES]
 
    SYNOPSIS
 
        NAME, TARGET/M, LIST/S, EXISTS/S, DISMOUNT/S, DEFER/S, PATH/S, ADD/S,
	REMOVE/S, VOLS/S, DIRS/S, DEVICES/S
 
    LOCATION
 
        Workbench:C
 
    FUNCTION
 
        ASSIGN creates a reference to a file or directory. The reference
	is a logical device name which makes it very convenient to specify
	assigned objects using the reference instead of their paths.
 
	If the NAME and TARGET arguments are given, ASSIGN assigns the
	given logical name to the specified target. If the NAME given is
	already assigned to a file or directory the new target replaces the
	previous target. A colon must be included after the NAME argument.
 
	If only the NAME argument is given, any assigns to that NAME are
	removed. If no arguments whatsoever are given, all logical
	assigns are listed.
 
    INPUTS
 
        NAME      --  the name that should be assigned to a file or directory
	TARGET    --  one or more files or directories to assign the NAME to
	LIST      --  list all assigns made
	EXISTS    --  if NAME is already assigned, set the condition flag to
		      WARN
	DISMOUNT  --  remove the volume or device NAME from the dos list
	DEFER     --  make an ASSIGN to a path or directory that not need to
		      exist at the time of assignment. The first time the
		      NAME is referenced the NAME is bound to the object
	PATH      --  path to assign with a non-binding assign. This means
		      that the assign is re-evaluated each time a reference
		      to NAME is done. Like for DEFER, the path doesn't have
		      to exist when the ASSIGN command is executed
	ADD       --  don't replace an assign but add another object for a
                      NAME (multi-assigns)
	REMOVE    --  remove an ASSIGN
	VOLS      --  show assigned volumes if in LIST mode
	DIRS      --  show assigned directories if in LIST mode
	DEVICES   --  show assigned devices if in LIST mode
 
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
        The assign command has many switches. This together with the somewhat
	messy handling of DosList:s by dos.library makes the operation rather
	complicated.
 
	There are some fundamental building blocks that defines the semantics
	of the Assign command.
 
	Only one of the switches ADD, REMOVE, PATH and DEFER may be specified.
 
	If EXISTS is specified, only the name parameter is important.
 
	The implementation is split up in two fundamental procedures.
 
	doAssign()     --  make [a number of] assigns
	showAssigns()  --  show the available assigns
	checkAssign()  --  check if a particular assign exists
 
    HISTORY
 
******************************************************************************/

#ifndef __AROS__
#define AROS_BSTR_strlen(s) *((UBYTE *)BADDR(s))
#endif

#ifdef __MORPHOS__
#define AROS_ASMSYMNAME(s) (&s)

static const int __abox__ = 1;
static const char version[] = "\0$VER: Assign unofficial 50.8 (24.09.07) © AROS" ;
#else
static const char version[] = "\0$VER: Assign 50.8 (24.09.07) © AROS" ;
#endif

#ifdef __AROS__
int __nocommandline = 1;
#endif

struct localdata
{
/* TODO: Under MorphOS this utility is pure and can be made resident.
	 Under AROS we are not pure because startup code is used.
	 Removing this limitation requires rewrite of the mmakefile.src
	 and some changes in the code for safe startup. */
#ifndef __AROS__
	struct ExecBase     *ld_SysBase;
#endif
	struct DosLibrary   *ld_DOSBase;
	struct MinList      ld_DeferList;
};

#ifndef __AROS__
#define SysBase   ld->ld_SysBase
#endif
#define DOSBase   ld->ld_DOSBase
#define DeferList ld->ld_DeferList


/* Prototypes */

static
int checkAssign(struct localdata *ld, STRPTR name);
static
int doAssign(struct localdata *ld, STRPTR name, STRPTR *target, BOOL dismount, BOOL defer, BOOL path,
             BOOL add, BOOL remove);
static
void showAssigns(struct localdata *ld, BOOL vols, BOOL dirs, BOOL devices);
static
int removeAssign(struct localdata *ld, STRPTR name);
static
STRPTR GetFullPath(struct localdata *ld, BPTR lock);

static
void _DeferPutStr(struct localdata *ld, CONST_STRPTR str);
static
void _DeferVPrintf(struct localdata *ld, CONST_STRPTR fmt, IPTR *args);
static
void _DeferFlush(struct localdata *ld, BPTR fh);

#define DeferPutStr(str) _DeferPutStr(ld,str)
#define DeferPrintf(fmt,args...) \
  DEBUG_ASSIGN(kprintf(fmt, ## args);) \
  do { IPTR __args[] = {0 , ## args}; _DeferVPrintf(ld, fmt, __args + 1); } while (0)
#define DeferFlush(fh) _DeferFlush(ld,fh)


static
const UBYTE template[] =
"NAME,"
"TARGET/M,"
"LIST/S,"
"EXISTS/S,"
"DISMOUNT/S,"
"DEFER/S,"
"PATH/S,"
"ADD/S,"
"REMOVE/S,"
"VOLS/S,"
"DIRS/S,"
"DEVICES/S";

struct ArgList
{
    STRPTR name;
    STRPTR *target;
    IPTR list;
    IPTR exists;
    IPTR dismount;
    IPTR defer;
    IPTR path;
    IPTR add;
    IPTR remove;
    IPTR vols;
    IPTR dirs;
    IPTR devices;
};

int main(void)
{
	struct localdata _ld, *ld = &_ld;
	struct RDArgs *readarg;
	struct ArgList arglist;
	struct ArgList *MyArgList = &arglist;
	int error = RETURN_OK;

#ifndef __AROS__
	SysBase = *((struct ExecBase **) 4);
#endif

	DOSBase = (struct DosLibrary *) OpenLibrary("dos.library",37);
	if (DOSBase)
	{
		memset(&arglist, 0, sizeof(arglist));

		NEWLIST(&DeferList);

		readarg = ReadArgs(template, (IPTR *)MyArgList, NULL);
		if (readarg)
		{
			/* Verify mutually exclusive args
			 */
			if ((MyArgList->add!=0) + (MyArgList->remove!=0) + (MyArgList->path!=0) + (MyArgList->defer!=0) > 1)
			{
				PutStr("Only one of ADD, REMOVE, PATH or DEFER is allowed\n");
				FreeArgs(readarg);
				CloseLibrary((struct Library *) DOSBase);
				return RETURN_FAIL;
			}

			/* Check device name
			 */
			if (MyArgList->name)
			{
				char *pos;

				/* Correct assign name construction? The rule is that the device name
				 * should end with a colon at the same time as no other colon may be
				 * in the name.
				 */
				pos = strchr(MyArgList->name, ':');
				if (!pos || pos[1])
				{
					Printf("Invalid device name %s\n", (IPTR)MyArgList->name);
					FreeArgs(readarg);
					CloseLibrary((struct Library *) DOSBase);
					return RETURN_FAIL;
				}
			}

			/* If the EXISTS keyword is specified, we only care about NAME */
			if (MyArgList->exists)
			{
				error = checkAssign(ld, MyArgList->name);
				DEBUG_ASSIGN(Printf("checkassign error %ld\n",error));
			}
			else if (MyArgList->name)
			{
				/* If a NAME is specified, our primary task is to add or
				   remove an assign */

				error = doAssign(ld, MyArgList->name, MyArgList->target, MyArgList->dismount, MyArgList->defer,
				                 MyArgList->path, MyArgList->add, MyArgList->remove);
				DEBUG_ASSIGN(Printf("doassign error %ld\n",error));
				if (MyArgList->list)
				{
					/* With the LIST keyword, the current assigns will be
					   displayed also when (after) making an assign */

					showAssigns(ld, MyArgList->vols, MyArgList->dirs, MyArgList->devices);
				}
			}
			else
			{
				/* If no NAME was given, we just show the current assigns
				   as specified by the user (VOLS, DIRS, DEVICES) */

				showAssigns(ld, MyArgList->vols, MyArgList->dirs, MyArgList->devices);
			}

			FreeArgs(readarg);
		}

		CloseLibrary((struct Library *) DOSBase);
	}

	DEBUG_ASSIGN(Printf("error %ld\n", error));

	return error;
}


static
void showAssigns(struct localdata *ld, BOOL vols, BOOL dirs, BOOL devices)
{
	ULONG           lockBits = LDF_READ;
	struct DosList *dl;

	/* If none of the parameters are specified, everything should be
	   displayed */
	if (!(vols || dirs || devices))
	{
		vols    = TRUE;
		dirs    = TRUE;
		devices = TRUE;
	}

	lockBits |= vols    ? LDF_VOLUMES : 0;
	lockBits |= dirs    ? LDF_ASSIGNS : 0;
	lockBits |= devices ? LDF_DEVICES : 0;

	dl = LockDosList(lockBits);

#warning "FIXME: GetFullPath() breaks LockDosList()'s Forbid()!"
#warning "Note: This should be ok as long as we don't have ks 1.x compatibility."

	if (vols)
	{
		struct DosList *tdl = dl;

		DeferPutStr("Volumes:\n");

		/* Print all mounted volumes */
		while ((tdl = NextDosEntry(tdl, LDF_VOLUMES)))
		{
			DeferPrintf("%b [Mounted]\n", tdl->dol_Name);
		}
	}

	if (dirs)
	{
		struct DosList *tdl = dl;
		int             count;

		DeferPutStr("\nDirectories:\n");

		/* Print all assigned directories */
		while ((tdl = NextDosEntry(tdl, LDF_ASSIGNS)))
		{
			DeferPrintf("%b ", tdl->dol_Name);

			for (count = 14 - AROS_BSTR_strlen(tdl->dol_Name); count > 0; count--)
			{
				DeferPutStr(" ");
			}

			switch (tdl->dol_Type)
			{
			case DLT_LATE:
				DeferPrintf("<%s>\n", (IPTR)tdl->dol_misc.dol_assign.dol_AssignName);
				break;

			case DLT_NONBINDING:
				DeferPrintf("[%s]\n", (IPTR)tdl->dol_misc.dol_assign.dol_AssignName);
				break;

			default:
				{
					STRPTR             dirName;     /* For NameFromLock() */
					struct AssignList *nextAssign;  /* For multiassigns */

					dirName = GetFullPath(ld, tdl->dol_Lock);

					if (dirName)
					{
						DeferPutStr(dirName);
						FreeVec(dirName);
					}
					DeferPutStr("\n");

					nextAssign = tdl->dol_misc.dol_assign.dol_List;

					while (nextAssign)
					{
						dirName = GetFullPath(ld, nextAssign->al_Lock);

						if (dirName)
						{
							DeferPrintf("             + %s\n", (IPTR)dirName);
							FreeVec(dirName);
						}

						nextAssign = nextAssign->al_Next;
					}
				}

				break;
			}
		} /* while (NextDosEntry()) */
	}

	if (devices)
	{
		struct DosList *tdl = dl;
		int             count = 0; /* Used to make sure that as most 5 entries are printed per line */

		DeferPutStr("\nDevices:\n");

		/* Print all assigned devices */
		while ((tdl = NextDosEntry(tdl, LDF_DEVICES)))
		{
			DeferPrintf("%b%lc", tdl->dol_Name, ++count % 5 ? ' ' : '\n');
		}

		if (count % 5)
		{
			DeferPutStr("\n");
		}
	}

	UnLockDosList(lockBits);

	DeferFlush(Output());
}


static
STRPTR GetFullPath(struct localdata *ld, BPTR lock)
{
	STRPTR buf;       /* Pointer to the memory allocated for the string */
	ULONG  size;      /* Holder of the (growing) size of the string */

	for (size = 512; ; size += 512)
	{
		buf = AllocVec(size, MEMF_ANY);
		if (!buf)
		{
			break;
		}

		if (NameFromLock(lock, buf, size))
		{
			return buf;
		}

		FreeVec(buf);

		if (IoErr() != ERROR_LINE_TOO_LONG)
		{
			break;
		}
	}

	return NULL;
}


static
int doAssign(struct localdata *ld, STRPTR name, STRPTR *target, BOOL dismount, BOOL defer, BOOL path,
             BOOL add, BOOL remove)
{
	STRPTR colon;
	BPTR   lock = NULL;
	int    i;

	int  error = RETURN_OK;
	LONG ioerr = 0;
	BOOL cancel = FALSE;
        BOOL success = TRUE;

/* TODO: AROS currently doesn't support packet handlers directly
      	 and we currently don't support shutting down IOFS handlers */
#ifndef __AROS__
	if (dismount)
	{
        	struct MsgPort *dp;
        	struct Process *tp;

        	tp=(struct Process *)FindTask(NULL);
        	tp->pr_WindowPtr = (APTR)-1;
        	dp = DeviceProc(name);
        	DEBUG_ASSIGN(Printf("doassign: dp <%08X>\n",dp));
        	if (dp)
        	{
                	success = DoPkt(dp,ACTION_DIE,0,0,0,0,0);
                	DEBUG_ASSIGN(Printf("doassign: ACTION_DIE returned %ld\n",success));
		}
	}
#endif

	colon = strchr(name, ':');

	*colon = '\0';	      /* Remove trailing colon; name[] is changed! */

	DEBUG_ASSIGN(Printf("doassign: name <%s>\n", name));

	/* This is a little bit messy... We first remove the 'name' assign
	 * and later in the loop the target assigns.
	 */

	if (dismount)
        {
		struct DosList *dl;
		struct DosList *fdl;

		DEBUG_ASSIGN(PutStr("Removing device node\n"));
		dl = LockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_WRITE);

		fdl = FindDosEntry(dl, name, LDF_VOLUMES | LDF_DEVICES);

		/* Note the ! for conversion to boolean value */
		if (fdl)
			success = RemDosEntry(fdl);

		UnLockDosList(LDF_VOLUMES | LDF_DEVICES | LDF_WRITE);
        }
        else
        {
        	if (target == NULL || *target == NULL)
		{
			error = removeAssign(ld, name);
			if (error)
			{
				ioerr = IoErr();
				cancel = TRUE;
			}
		}
        }

	/* AmigaDOS doesn't use RETURN_WARN here... but it should? */
	error = success ? error : RETURN_WARN;
        DEBUG_ASSIGN(Printf("error: %d\n", error));

	// The Loop over multiple targets starts here

	if (target)
	{
		for (i = 0; target[i]; i++)
		{
			cancel = FALSE;

			DEBUG_ASSIGN(Printf("doassign: target <%s>\n", target[i]));
			if (!(path || defer || dismount))
			{
				lock = Lock(target[i], SHARED_LOCK);

				if (!lock)
				{
					Printf("Can't find %s\n", (IPTR)target[i]);
					return RETURN_FAIL;
				}
			}

			if (remove)
			{
				if (!RemAssignList(name, lock))
				{
					Printf("Can't subtract %s from %s\n", (IPTR)target[i], (IPTR)name);
					error = RETURN_FAIL;
				}
				UnLock(lock);
			}
			else if (path)
			{
				if (!AssignPath(name, target[i]))
				{
					ioerr = IoErr();
					error = RETURN_FAIL;
					DEBUG_ASSIGN(Printf("doassign AssignPath error %ld\n",error));
				}
			}
			else if (add)
			{
				if (!AssignAdd(name, lock))
				{
					struct DosList *dl;

					error = RETURN_FAIL;
					ioerr = IoErr();
					DEBUG_ASSIGN(Printf("doassign AssignAdd error %ld\n",error));

					/* Check if the assign doesn't exist at all. If so, create it.
					 * This fix bug id 145. - Piru
					 */
					dl = LockDosList(LDF_ASSIGNS | LDF_READ);
					dl = FindDosEntry(dl, name, LDF_ASSIGNS);
					UnLockDosList(LDF_ASSIGNS | LDF_READ);

					if (!dl)
					{
						if (AssignLock(name, lock))
						{
							error = RETURN_OK;
							lock = NULL;
						}
						else
						{
							ioerr = IoErr();
							DEBUG_ASSIGN(Printf("doassign AssignLock error %ld\n", error));
						}
					}

					if (lock)
						UnLock(lock);

					if (error && ioerr != ERROR_OBJECT_EXISTS)
					{
						Printf("Can't add %s to %s\n", (IPTR)target[i], (IPTR)name);
					}
				}
			}
			else if (defer)
			{
				if (!AssignLate(name, target[i]))
				{
					ioerr = IoErr();
					UnLock(lock);
					error = RETURN_FAIL;
					DEBUG_ASSIGN(Printf("doassign AssignLate error %ld\n",error));
				}
			}
			else
			{
				/* If no extra parameters are specified we just do a regular
				   assign (replacing any possible previous assign with that
				   name. The case of target being NULL is taken care of above.
				*/
				if (!AssignLock(name, lock))
				{
					ioerr = IoErr();
					cancel = TRUE;
					UnLock(lock);
					error = RETURN_FAIL;
					DEBUG_ASSIGN(Printf("doassign AssignLock error %ld\n",error));
				}
				/* If there are several targets, the next ones have to be added. */
				add = TRUE;
			}

			/* We break as soon as we get a serious error */
			if (error >= RETURN_FAIL)
			{
				break;
			}

		} /* loop through all targets */
	}

	if (error)
	{
		if (ioerr == ERROR_OBJECT_EXISTS)
		{
			Printf("Can't %s %s\n", (IPTR)(cancel ? "cancel" : "assign"), (IPTR)name);
		}
		else
		{
			PrintFault(ioerr, NULL);
		}
	}

	return error;
}


static
int removeAssign(struct localdata *ld, STRPTR name)
{
	/* In case no target is given, the 'name' assign should be removed.
	 * The AmigaDOS semantics for this is apparently that the error
	 * code is never set even if the assign didn't exist.
	 */

	if (!AssignLock(name, NULL))
	{
		return RETURN_FAIL;
	}
	return RETURN_OK;
}


static
int checkAssign(struct localdata *ld, STRPTR name)
{
	STRPTR colon;
	struct DosList *dl;
	int             error = RETURN_OK;

	if (!name)
		name = "";

	colon = strchr(name, ':');
	if (colon)
	{
		*colon = '\0';
	}

	dl = LockDosList(LDF_DEVICES | LDF_ASSIGNS | LDF_VOLUMES | LDF_READ);

#warning "Note: GetFullPath() breaks LockDosList()'s Forbid()!"
#warning "Note: This should be ok as long as we don't have ks 1.x compatibility."

	dl = FindDosEntry(dl, name, LDF_DEVICES | LDF_ASSIGNS | LDF_VOLUMES);
	if (dl)
	{
		struct DosList *tdl = dl;
		int             count;

		switch (dl->dol_Type)
		{
		case DLT_DEVICE:
			DeferPrintf("%b\n", tdl->dol_Name);
			break;

		case DLT_VOLUME:
			DeferPrintf("%b [Mounted]\n", tdl->dol_Name);
			break;

		case DLT_DIRECTORY:
		case DLT_LATE:
		case DLT_NONBINDING:

			DeferPrintf("%b ", tdl->dol_Name);

			for (count = 14 - *((UBYTE*)BADDR(tdl->dol_Name)); count > 0; count--)
			{
				DeferPutStr(" ");
			}

			switch (tdl->dol_Type)
			{
			case DLT_LATE:
				DeferPrintf("<%s>\n", (IPTR)tdl->dol_misc.dol_assign.dol_AssignName);
				break;

			case DLT_NONBINDING:
				DeferPrintf("[%s]\n", (IPTR)tdl->dol_misc.dol_assign.dol_AssignName);
				break;

			default:
				{
					STRPTR             dirName;     /* For NameFromLock() */
					struct AssignList *nextAssign;  /* For multiassigns */

					dirName = GetFullPath(ld, tdl->dol_Lock);

					if (dirName)
					{
						DeferPutStr(dirName);
						FreeVec(dirName);
					}
					DeferPutStr("\n");

					nextAssign = tdl->dol_misc.dol_assign.dol_List;

					while (nextAssign)
					{
						dirName = GetFullPath(ld, nextAssign->al_Lock);

						if (dirName)
						{
							DeferPrintf("             + %s\n", (IPTR)dirName);
							FreeVec(dirName);
						}

						nextAssign = nextAssign->al_Next;
					}
				}
			}

			break;
		}
	}
	else
	{
		DeferPrintf("%s: not assigned\n", (IPTR)name);

		error = RETURN_WARN;
	}

	UnLockDosList(LDF_DEVICES | LDF_ASSIGNS | LDF_VOLUMES | LDF_READ);

	DeferFlush(Output());

	if (colon)
		*colon = ':';

	return error;
}


/* Feferred printing routines - Piru
*/

#define MAXDEFERBUF 4096
#define MAXOUTPUT   128
struct deferbufnode
{
	struct MinList node;
	LONG           pos;
	UBYTE          buf[MAXDEFERBUF];
};

static void deferputch(UBYTE ch, struct localdata *ld)
{
	struct deferbufnode *cur;

	if (!ch)
		return;

	cur = (struct deferbufnode *) GetTail(&DeferList);

	if (!cur || cur->pos >= MAXDEFERBUF)
	{
		cur = AllocMem(sizeof(struct deferbufnode), MEMF_ANY);
		if (!cur)
			return;

		cur->pos = 0;

		ADDTAIL(&DeferList, cur);
	}

	cur->buf[cur->pos] = ch;
	cur->pos++;
}

static
void _DeferPutStr(struct localdata *ld, CONST_STRPTR str)
{
	UBYTE c;

	DEBUG_ASSIGN(kprintf(str);)
	while ((c = *str++))
	{
		deferputch(c, ld);
	}
}

#ifdef __AROS__
AROS_UFH2(static void, deferputch_gate,
	  AROS_UFHA(UBYTE, ch, D0),
	  AROS_UFHA(struct localdata *, ld, A3))
{
	AROS_USERFUNC_INIT
	deferputch(ch, ld);
	AROS_USERFUNC_EXIT
}
#endif

#ifdef __MORPHOS__
static
void deferputch_trampoline(void)
{
	UBYTE ch = (UBYTE) REG_D0;
	struct localdata *ld = (struct localdata *) REG_A3;

	deferputch(ch, ld);
}

static
const struct EmulLibEntry deferputch_gate =
{
	TRAP_LIBNR, 0, (void (*)(void)) deferputch_trampoline
};
#endif

static
void _DeferVPrintf(struct localdata *ld, CONST_STRPTR fmt, IPTR *args)
{
	RawDoFmt(fmt, args, (void (*)(void))AROS_ASMSYMNAME(deferputch_gate), ld);
}

static
void _DeferFlush(struct localdata *ld, BPTR fh)
{
	struct deferbufnode *node;
	BOOL broken = FALSE;

	Flush(fh);

	while ((node = REMHEAD(&DeferList)))
	{
		LONG offs = 0;
		LONG left = node->pos;

		while (!broken && left)
		{
			LONG len;

			if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
			{
				broken = TRUE;
				break;
			}

			len = left > MAXOUTPUT ? MAXOUTPUT : left;

			Write(fh, node->buf + offs, len);
			offs += len;
			left -= len;
		}

		FreeMem(node, sizeof(struct deferbufnode));
	}

	Flush(fh);

	if (broken)
	{
		PrintFault(ERROR_BREAK, NULL);
	}
}

