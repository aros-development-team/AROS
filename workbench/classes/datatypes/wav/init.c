/*
 *	wave.datatype
 *	(c) Fredrik Wikstrom
 */

#define LIBNAME "wave.datatype"

#include "wave.datatype_rev.h"
#include "wave_class.h"

const char
#ifdef __GNUC__
__attribute__((used))
#endif
verstag[] = VERSTAG;

ULONG libObtain (struct LibraryManagerInterface *Self);
ULONG libRelease (struct LibraryManagerInterface *Self);
struct ClassBase *libOpen (struct LibraryManagerInterface *Self, ULONG version);
BPTR libClose (struct LibraryManagerInterface *Self);
BPTR libExpunge (struct LibraryManagerInterface *Self);

static CONST_APTR lib_manager_vectors[] = {
	(APTR)libObtain,
	(APTR)libRelease,
	NULL,
	NULL,
	(APTR)libOpen,
	(APTR)libClose,
	(APTR)libExpunge,
	NULL,
	(APTR)-1,
};

static const struct TagItem lib_managerTags[] = {
	{ MIT_Name,			(ULONG)"__library"			},
	{ MIT_VectorTable,	(ULONG)lib_manager_vectors	},
	{ MIT_Version,		1							},
	{ TAG_END }
};

ULONG dtObtain (struct DTClassIFace *Self);
ULONG dtRelease (struct DTClassIFace *Self);
Class * dtObtainEngine (struct DTClassIFace *Self);

static CONST_APTR lib_main_vectors[] = {
	(APTR)dtObtain,
	(APTR)dtRelease,
	NULL,
	NULL,
	(APTR)dtObtainEngine,
	(APTR)-1
};

static const struct TagItem lib_mainTags[] = {
	{ MIT_Name,			(ULONG)"main"				},
	{ MIT_VectorTable,	(ULONG)lib_main_vectors	},
	{ MIT_Version,		1							},
	{ TAG_END }
};

static CONST_APTR libInterfaces[] = {
	lib_managerTags,
	lib_mainTags,
	NULL
};

struct ClassBase * libInit (struct ClassBase *libBase, BPTR seglist, struct ExecIFace *ISys);

static const struct TagItem libCreateTags[] = {
	{ CLT_DataSize,		(ULONG)sizeof(struct ClassBase)	},
	{ CLT_InitFunc,		(ULONG)libInit						},
	{ CLT_Interfaces,	(ULONG)libInterfaces				},
	{ TAG_END }
};

const struct Resident lib_res
#ifdef __GNUC__
__attribute__((used))
#endif
=
{
    RTC_MATCHWORD,
    (struct Resident *)&lib_res,
    (APTR)(&lib_res + 1),
    RTF_NATIVE|RTF_AUTOINIT, /* Add RTF_COLDSTART if you want to be resident */
    VERSION,
    NT_LIBRARY, /* Make this NT_DEVICE if needed */
    0, /* PRI, usually not needed unless you're resident */
    LIBNAME,
    VSTRING,
    (APTR)libCreateTags
};

void _start () {
}

static int openDTLibs (struct ClassBase *libBase);
static void closeDTLibs (struct ClassBase *libBase);

struct ClassBase *libInit (struct ClassBase *libBase, BPTR seglist, struct ExecIFace *ISys) {

	libBase->libNode.lib_Node.ln_Type = NT_LIBRARY;
	libBase->libNode.lib_Node.ln_Pri  = 0;
	libBase->libNode.lib_Node.ln_Name = LIBNAME;
	libBase->libNode.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
	libBase->libNode.lib_Version      = VERSION;
	libBase->libNode.lib_Revision     = REVISION;
	libBase->libNode.lib_IdString     = VSTRING;


	libBase->SegList = seglist;
	if (openDTLibs(libBase)) {
		if (libBase->DTClass = initDTClass(libBase)) {
			return libBase;
		}
	}
	closeDTLibs(libBase);

	return NULL;
}

ULONG libObtain (struct LibraryManagerInterface *Self) {
	return Self->Data.RefCount++;
}

ULONG libRelease (struct LibraryManagerInterface *Self) {
	return Self->Data.RefCount--;
}

struct ClassBase *libOpen (struct LibraryManagerInterface *Self, ULONG version) {
	struct ClassBase *libBase;

	libBase = (struct ClassBase *)Self->Data.LibBase;

	libBase->libNode.lib_OpenCnt++;
	libBase->libNode.lib_Flags &= ~LIBF_DELEXP;

	return libBase;
}

BPTR libClose (struct LibraryManagerInterface *Self) {
	struct ClassBase *libBase;

	libBase = (struct ClassBase *)Self->Data.LibBase;

	libBase->libNode.lib_OpenCnt--;

	if (libBase->libNode.lib_OpenCnt) {
		return 0;
	}

	if (libBase->libNode.lib_Flags & LIBF_DELEXP) {
		return (BPTR)Self->LibExpunge();
	} else {
		return 0;
	}
}

BPTR libExpunge (struct LibraryManagerInterface *Self) {
	struct ClassBase *libBase;
	BPTR result = 0;

	libBase = (struct ClassBase *)Self->Data.LibBase;

	if (libBase->libNode.lib_OpenCnt == 0) {
		Remove(&libBase->libNode.lib_Node);

		result = libBase->SegList;

		freeDTClass(libBase, libBase->DTClass);
		libBase->DTClass = NULL;

		closeDTLibs(libBase);

		Remove(&libBase->libNode.lib_Node);
		DeleteLibrary(&libBase->libNode);
	} else {
		libBase->libNode.lib_Flags |= LIBF_DELEXP;
	}

	return result;
}

ULONG dtObtain (struct DTClassIFace *Self) {
	return(Self->Data.RefCount++);
}

ULONG dtRelease (struct DTClassIFace *Self) {
	return(Self->Data.RefCount--);
}

Class * dtObtainEngine (struct DTClassIFace *Self) {
	struct ClassBase *libBase;
	libBase = (struct ClassBase *)Self->Data.LibBase;
	return libBase->DTClass;
}


static int openDTLibs (struct ClassBase *libBase) {
	DOSBase = OpenLibrary("dos.library", 50);
	if (!DOSBase) return FALSE;

	IntuitionBase = OpenLibrary("intuition.library", 50);
	if (!IntuitionBase) return FALSE;

	UtilityBase = OpenLibrary("utility.library", 50);
	if (!UtilityBase) return FALSE;

	DataTypesBase = OpenLibrary("datatypes.library", 50);
	if (!DataTypesBase) return FALSE;

	return TRUE;
}

static void closeDTLibs (struct ClassBase *libBase) {
	if (DataTypesBase) CloseLibrary(DataTypesBase);

	if (UtilityBase) CloseLibrary(UtilityBase);

	if (IntuitionBase) CloseLibrary(IntuitionBase);

	if (DOSBase) CloseLibrary(DOSBase);
}
