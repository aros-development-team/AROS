/* $Id$ */
/* $Log: dd_funcs.c $
 * Revision 1.20  1999/09/11  17:05:14  Michiel
 * bugfix version 18.4
 *
 * Revision 1.19  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 1.18  1999/03/25  22:03:12  Michiel
 * Allow setdeldir for muROOT_UID only
 * Check volume for setdeldir
 * Added deldir inquiry option
 *
 * Revision 1.17  1999/02/22  16:27:21  Michiel
 * dd_SetDeldir added
 *
 * Revision 1.16  1998/09/27  11:26:37  Michiel
 * Error incurred softlock can now be disabled in dd_WriteProtect
 *
 * Revision 1.15  1998/05/31  16:27:42  Michiel
 * added ACTION_IS_PFS2
 *
 * Revision 1.14  1998/05/29  19:31:18  Michiel
 * fixed bug 107
 *
 * Revision 1.13  1998/05/27  20:16:13  Michiel
 * AFS --> PFS2
 * Allow ID_PFS2_DISK in custom packets
 *
 * Revision 1.12  1997/03/03  22:04:04  Michiel
 * Release 16.21
 *
 * Revision 1.11  1996/03/14  19:29:43  Michiel
 * using new NewFile specification
 *
 * Revision 1.10  1996/03/07  10:06:34  Michiel
 * DICE MakeIndex fix (wt)
 * Rename MuAF access (wt)
 *
 * Revision 1.9  1996/01/30  12:48:52  Michiel
 * --- working tree overlap ---
 * new notify routines
 *
 * Revision 1.8  1995/12/29  11:03:17  Michiel
 * dd_SetRollover added
 *
 * Revision 1.7  1995/12/20  11:28:10  Michiel
 * indented
 *
 * Revision 1.6  1995/12/07  15:25:38  Michiel
 * rollover support and bugfixes
 *
 * Revision 1.5  1995/11/07  14:52:34  Michiel
 * FreeUnusedResources after flush
 *
 * Revision 1.4  1995/09/01  11:14:40  Michiel
 * fixed enforcer hit in dd_Open
 * */


/**********************
 * function prototypes
 */

static SIPTR NotKnown(struct DosPacket *pkt, globaldata * g);
static SIPTR NotYetImplemented(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_IsFileSystem(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Quit(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_CurrentVolume(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Lock(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Unlock(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_DupLock(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_CreateDir(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Parent(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_SameLock(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Open(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_OpenFromLock(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Close(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_ChangeMode(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_SeekRead(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_WriteSFS(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Relabel(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_AddBuffers(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Info(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Flush(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_WriteProtect(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_SerializeDisk(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_DeleteObject(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Rename(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_SetProperty(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Examine(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_ExamineAll(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_MakeLink(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_ReadLink(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_InhibitOn(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_InhibitOff(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_Format(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_AddNotify(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_RemoveNotify(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_SignalIdle(struct DosPacket *pkt, globaldata * g);

/* internal packets; PFS2 extensions */
static int dd_CheckCustomPacket(LONG id);
static SIPTR dd_IsPFS2 (struct DosPacket *pkt, globaldata *g);
static SIPTR dd_KillEmpty(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_RemoveDirEntry(struct DosPacket *pkt, globaldata * g);

#if EXTRAPACKETS
static SIPTR dd_Sleep(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_UpdateAnode(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_SetFileSize(struct DosPacket *pkt, globaldata *g);
#if ROLLOVER
static SIPTR dd_MakeRollover(struct DosPacket *pkt, globaldata * g);
static SIPTR dd_SetRollover(struct DosPacket *pkt, globaldata *g);
#endif /* ROLLOVER */
#if DELDIR
static SIPTR dd_SetDeldir(struct DosPacket *pkt, globaldata *g);
#endif
#endif /* ExtraPackets */
#if defined(__MORPHOS__)
static LONG dd_MorphOSQueryAttr(struct DosPacket *pkt, globaldata *g);
#endif


/**********************
 * table functions 
 */

static SIPTR NotKnown(struct DosPacket *pkt, globaldata * g)
{
	pkt->dp_Res2 = ERROR_ACTION_NOT_KNOWN;
	return DOSFALSE;
}

static SIPTR NotYetImplemented(struct DosPacket *pkt, globaldata * g)
{
	pkt->dp_Res2 = ERROR_NOT_IMPLEMENTED;
	return DOSFALSE;
}

static SIPTR dd_IsFileSystem(struct DosPacket *pkt, globaldata * g)
{
	return DOSTRUE;
}

/* causes PFS2 to quit. The dieing-flag will cause main to call 
 * Quit() after returning the packet
 */
static SIPTR dd_Quit(struct DosPacket *pkt, globaldata * g)
{
#if UNSAFEQUIT
	g->dieing = TRUE;
	return DOSTRUE;
#else
	struct volumedata *volume = g->currentvolume;

	if (!volume || (IsMinListEmpty(&volume->fileentries) && IsMinListEmpty(&volume->notifylist)))
	{
		g->dieing = TRUE;
		return DOSTRUE;
	}
	else
	{
		pkt->dp_Res2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}
#endif
}

static SIPTR dd_CurrentVolume(struct DosPacket *pkt, globaldata * g)
{
	/* arguments fixed 18-04-94 */
	// ARG1 = APTR fileentry. (filled in by Open()) of NULL
	// RES1 = BPTR to volume node structure
	// RES2 = Unit number

	pkt->dp_Res2 = g->startup->fssm_Unit;

	if (!pkt->dp_Arg1)
	{
		if (g->currentvolume)
			return (SIPTR)MKBADDR(g->currentvolume->devlist);
		else
			return (SIPTR)BNULL;
	}
	else
		return (SIPTR)MKBADDR(((fileentry_t *) pkt->dp_Arg1)->le.volume->devlist);
}


/**********************
 * Lock functions
 */

static SIPTR dd_Lock(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = Lock on directory ARG2 is relative to (BPTR)
	// ARG2 = BSTR Name of object
	// ARG3 = LONG mode: SHARED_LOCK, EXCLUSIVE_LOCK
	// RES1 = Lock on requested object (0=failure)
	// RES2 = failurecode

	lockentry_t *parentfe;
	listentry_t *filefe;
	union objectinfo filefi, *parentfi;
	UBYTE pathname[PATHSIZE], *fullname;
	listtype type;
	SIPTR *error = &pkt->dp_Res2;
#if MULTIUSER
	struct extrafields extrafields;
	ULONG flags;
#endif

	ENTER("Lock");

	GetFileInfoFromLock(pkt->dp_Arg1, 0, parentfe, parentfi);
	BCPLtoCString(pathname, (DSTR)BARG2(pkt));
	DB(Trace(1, "Lock", "locking : %s parent: %lx \n", pathname, pkt->dp_Arg1));
	DB(if (parentfi) Trace(1, "Lock", "anodenr = %lx and %lx \n", parentfe->nextanode,
		(parentfi->file.direntry ? parentfi->file.direntry->anode : ANODE_ROOTDIR)));
	SkipColon(fullname, pathname);

	/* Locate object
	 * .. if no filename then lock parent
	 */
	if (parentfi && !*fullname)
		filefi = *parentfi;
	else if (!FindObject(parentfi, fullname, &filefi, error, g))
	{
		DB(Trace(1, "Lock", "failed at locktime : %s\n", fullname));
		return 0L;
	}

	// Add object to list
	if (IsVolume(filefi))
	{
		type.value = ET_VOLUME + ET_SHAREDREAD;
	}
	else
	{
		type.value = ET_LOCK;
		type.flags.access = (pkt->dp_Arg3 == EXCLUSIVE_LOCK ? ET_EXCLREAD : ET_SHAREDREAD);

#if MULTIUSER
#if DELDIR
		GetExtraFieldsOI(&filefi, &extrafields, g);
#else /* DELDIR */
		GetExtraFields(filefi.file.direntry, &extrafields);
#endif /* DELDIR */

		flags = muGetRelationship(extrafields);

		if (!(flags & muRel_PROPERTY_ACCESS) &&
			(*error = muFS_CheckReadAccess(extrafields.prot, flags, g)))
			return 0;
#endif /* MULTIUSER */
	}

	if (!(filefe = MakeListEntry(&filefi, type, error, g)))
		return 0;

	if (!AddListEntry(filefe))
	{
		DB(Trace(1, "dd_Lock", "object in use"));

		FreeListEntry(filefe, g);
		*error = ERROR_OBJECT_IN_USE;
		return 0;
	}

	DB(Trace(1, "Lock", "adres: %lx\n", filefe));
	pkt->dp_Res2 = 0;
	return (SIPTR)MKBADDR(&filefe->lock);
}

static SIPTR dd_Unlock(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = LOCK to free
	// RES1 = BOOL TRUE

	listentry_t *listentry;

	listentry = ListEntryFromLock(pkt->dp_Arg1);
	if (listentry)
	{
		// not needed: UpdateLE(listentry, g);
		RemoveListEntry(listentry, g);
	}

	return DOSTRUE;
}

static SIPTR dd_DupLock(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BPTR lock to duplicate / filehandle->fh_Arg1 to dup
	// RES1 = Duplicated lock
	// RES2 = failure code if RES1=0
	//
	// scrfe: oorspronkelijke fileentry
	// dstfe: copy van scrfe

	lockentry_t *srcfe, *dstfe;
	union objectinfo filefi;
	listtype type;

	if (!pkt->dp_Arg1)
	{
		if (!g->currentvolume)
		{
			pkt->dp_Res2 = ERROR_NO_DISK;
			return 0;
		}

		/* return lock to root (GURU 605) */
		filefi.volume.root = 0;
		filefi.volume.volume = g->currentvolume;
		type.value = ET_VOLUME + ET_SHAREDREAD;
		if (!(dstfe = (lockentry_t *)MakeListEntry(&filefi, type, &pkt->dp_Res2, g)))
			return 0;

		goto dl_add;
	}

	/* get entry struct */
	if (pkt->dp_Type == ACTION_COPY_DIR)
		srcfe = LockEntryFromLock(pkt->dp_Arg1);
	else
		srcfe = (lockentry_t *)pkt->dp_Arg1;

	if (!CheckVolume(srcfe->le.volume, 0, &pkt->dp_Res2, g))
		return DOSFALSE;
	UpdateLE((listentry_t *)srcfe, g);

	/* only allow SHAREDREAD locks & fe's */
	if (srcfe->le.type.flags.access != ET_SHAREDREAD)
	{
		pkt->dp_Res2 = ERROR_OBJECT_IN_USE;
		return 0;
	}

	if (!(dstfe = (lockentry_t *) AllocMemP(sizeof(lockentry_t), g)))
	{
		pkt->dp_Res2 = ERROR_NO_FREE_STORE;
		return 0;
	}

	/* only copy the listentry_t part. The extension fields (nextanode,
	 * nextentry and nextdirblock are all zero
	 */
	CopyMem(srcfe, dstfe, sizeof(listentry_t));

	/* if COPY_DIR_FH then update type to LOCK */
	if (pkt->dp_Type == ACTION_COPY_DIR_FH)
		dstfe->le.type.flags.type = ETF_LOCK;

  dl_add:
	if (!AddListEntry((listentry_t *)dstfe))
	{
		FreeListEntry((listentry_t *)dstfe, g);    // Niet mogelijk SHAREDREAD always copyable

		pkt->dp_Res2 = ERROR_OBJECT_IN_USE;
		return 0;
	}

	DB(Trace(1, "DupLock", "of %lx adres: %lx\n", srcfe, dstfe));
	return (SIPTR)MKBADDR(&dstfe->le.lock);
}

static SIPTR dd_CreateDir(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = Lock on directory ARG2 is relative to (BPTR)
	// ARG2 = BSTR Name of new directory
	// RES1 = Lock on new directory
	// RES2 = failurecode (if res1 = DOSFALSE)

	lockentry_t *parentle, *newdirle;
	union objectinfo path, *parentfi;
	UBYTE *dirname, pathname[PATHSIZE];
	UBYTE *zonderpad;
	SIPTR *error = &pkt->dp_Res2;
#if MULTIUSER
#if MU_CHECKDIR
	struct extrafields extrafields;
	ULONG flags;
#endif
#endif

	GetFileInfoFromLock(pkt->dp_Arg1, 1, parentle, parentfi);
	BCPLtoCString(pathname, (DSTR)BARG2(pkt));
	SkipColon(dirname, pathname);
	zonderpad = GetFullPath(parentfi, dirname, &path, error, g);
	if (!zonderpad)
		return DOSFALSE;

#if MULTIUSER
#if MU_CHECKDIR
	if (!IsVolume(path))
	{
#if DELDIR
		GetExtraFieldsOI(&path, &extrafields);
#else /* DELDIR */
		GetExtraFields(path.file.direntry, &extrafields);
#endif /* DELDIR */
		flags = muGetRelationship(extrafields);
		if (*error = muFS_CheckWriteAccess(extrafields.prot, flags, g))
			return DOSFALSE;
	}
#endif /* MU_CHECKDIR */
#endif /* MULTIUSER */

	newdirle = NewDir(&path, zonderpad, error, g);

	if (newdirle)
	{
		PFSDoNotify(&newdirle->le.info.file, TRUE, g);
		return (SIPTR)MKBADDR(&newdirle->le.lock);
	}
	else
		return DOSFALSE;
}

static SIPTR dd_Parent(struct DosPacket *pkt, globaldata * g)
{
	// ACTION_PARENT en ACTION_PARENT_FH
	// ARG1 = BPTR Lock on object to get the parent of
	// ARG1.FH = filehandle->dp_Arg1 == &listentry
	// RES1 = BPTR Parent lock
	// RES2 = failure code (if res1 = 0)

	lockentry_t *childfe;
	listentry_t *parentfe;
	listtype type;
	union objectinfo *childfi, parentfi;
	SIPTR *error = &pkt->dp_Res2;

	// get fe & fi of child
	if (pkt->dp_Type == ACTION_PARENT)
		childfe = LockEntryFromLock(pkt->dp_Arg1);
	else
		childfe = (lockentry_t *) (pkt->dp_Arg1);

	/* check if volume present and update lock */
	if (childfe)
	{
		if (!CheckVolume(childfe->le.volume, 0, error, g))
			return DOSFALSE;
		UpdateLE((listentry_t *)childfe, g);
		childfi = &childfe->le.info;
	}
	else
	{
		if (!g->currentvolume)
		{
			*error = ERROR_NO_DISK;
			return 0;
		}
		childfi = NULL;
	}

	// get fi of parent
	if (!GetParent(childfi, &parentfi, &pkt->dp_Res2, g))
		return 0;

	// make and enter a fileentry
	type.value = 0;
	type.flags.type = IsVolume(parentfi) ? (ETF_VOLUME) : (ETF_LOCK);
	type.flags.access = ET_SHAREDREAD;

	if (!(parentfe = MakeListEntry(&parentfi, type, &pkt->dp_Res2, g)))
		return 0;

	if (!AddListEntry(parentfe))
	{
		FreeListEntry(parentfe, g);
		pkt->dp_Res2 = ERROR_OBJECT_IN_USE;
		//NormalErrorMsg("ßE Parent failed", NULL);
		return (0);
	}

	return (SIPTR)MKBADDR(&parentfe->lock);
}


/* Somehow a filehandler should send opposite values: */
#define H_LOCK_SAME 1
#define H_LOCK_SAME_VOLUME 0

static SIPTR dd_SameLock(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BPTR Lock 1 to compare
	// ARG2 = BPTR Lock 2 to compare
	// RES1 = LONG result of compare
	// RES2 = CODE failurecode (if res1 = LOCK_DIFFERENT)

	lockentry_t *lock1, *lock2;

	lock1 = LockEntryFromLock(pkt->dp_Arg1);
	UpdateLE((listentry_t *)lock1, g);
	lock2 = LockEntryFromLock(pkt->dp_Arg2);
	UpdateLE((listentry_t *)lock2, g);
	pkt->dp_Res2 = 0;

	if (!lock1 || !lock2)
	{
		pkt->dp_Res2 = ERROR_INVALID_LOCK;
		return LOCK_DIFFERENT;
	}

	if (lock1->le.volume != lock2->le.volume)
		return LOCK_DIFFERENT;

	if (lock1->le.anodenr == lock2->le.anodenr)
		return H_LOCK_SAME;
	else
		return H_LOCK_SAME_VOLUME;
}


/**********************
 * Filehandle functions
 */

static SIPTR dd_Open(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BPTR Filehandle to fill in
	// ARG2 = BPTR LOCK on directory ARG3 is relative to
	// ARG3 = BSTR Name of file to be opened
	// RES1 = Success/Failure (DOSTRUE/DOSFALSE)
	// RES2 = failure code
	//
	// fullname: name inclusief pat
	// filename: filename zonder pat
	// pathfi: fileinfo van path van te openen file
	// filefi: fileinfo van te openen file
	// mode: nodetype van fileentry van file
	//
	// GURUbook: ACTION_FINDINPUT premits write access (598)

	struct FileHandle *filehandle;
	listentry_t *filefe;
	lockentry_t *parentfe;
	union objectinfo pathfi, filefi, *parentfi;
	listtype type;
	SIPTR *error = &pkt->dp_Res2;
	UBYTE pathname[PATHSIZE], *fullname, *filename = NULL;
	BOOL found;
#if MULTIUSER
	struct extrafields extrafields;
	ULONG flags;
#if MU_CHECKDIR
	struct extrafields path_extrafields;
	ULONG path_flags;
#endif /* MU_CHECKDIR */
#endif /* MULTIUSER */

	// -I- benodigde waarden afleiden van pakket
	filehandle = (struct FileHandle *)BADDR(pkt->dp_Arg1);
	GetFileInfoFromLock(pkt->dp_Arg2, 0, parentfe, parentfi);
	BCPLtoCString(pathname, (DSTR)BARG3(pkt));
	DB(Trace(1, "Open", "%s\n", pathname));
	SkipColon(fullname, pathname);

	/* 15.9: check if path is file. If so it has to be opened
	 * if an empty string was specified as filename
	 * (see GuruBook:599)
	 */
	if (parentfi && IsFile(*parentfi) && *fullname == 0)
	{
		found = TRUE;
		filefi = *parentfi;
	}
	else
	{
		/* Get path to file */
		if (!(filename = GetFullPath(parentfi, fullname, &pathfi, error, g)))
			return DOSFALSE;

		/* try to locate file */
		found = FindObject(&pathfi, filename, &filefi, error, g);
	}

	if (found)
	{
		/* softlinks cannot directly be opened */
		if (IsSoftLink(filefi))
		{
			*error = ERROR_IS_SOFT_LINK;
			return DOSFALSE;
		}

		/* check if file (only files can be opened) */
#if DELDIR
		if ((IsVolume(filefi) || IsDelDir(filefi) || IsDir(filefi)))
#else
		if ((IsVolume(filefi) || IsDir(filefi)))
#endif
		{
			*error = ERROR_OBJECT_WRONG_TYPE;
			return DOSFALSE;
		}
	}

	type.value = ET_FILEENTRY;

#if MULTIUSER
#if MU_CHECKDIR
	if (IsVolume(pathfi))
		memset(&path_extrafields, 0, sizeof(struct extrafields));
	else
#if DELDIR
		GetExtraFieldsOI(&pathfi, &path_extrafields);
#else /* DELDIR */
		GetExtraFields(pathfi.file.direntry, &path_extrafields);
#endif /* DELDIR */
#endif /* MU_CHECKDIR */

	if (found)
#if DELDIR
		GetExtraFieldsOI(&filefi, &extrafields, g);
#else
		GetExtraFields(filefi.file.direntry, &extrafields);
#endif

	if (g->muFS_ready)
	{
#if MU_CHECKDIR
		path_flags = muGetRelationshipA(g->user, (path_extrafields.uid << 16) + path_extrafields.gid, NULL);
#endif
		if (found)
			flags = muGetRelationshipA(g->user, (extrafields.uid << 16) + extrafields.gid, NULL);
	}
	else
	{
#if MU_CHECKDIR
		path_flags = ((path_extrafields.uid == muNOBODY_UID) << muRelB_NO_OWNER) | muRelF_NOBODY;
#endif
		if (found)
			flags = ((extrafields.uid == muNOBODY_UID) << muRelB_NO_OWNER) | muRelF_NOBODY;
	}
#endif /* MULTIUSER */

	switch (pkt->dp_Type)
	{
		case ACTION_FINDINPUT:
			if (!found)
				return DOSFALSE;

#if MULTIUSER
			if ((*error = muFS_CheckReadAccess(extrafields.prot, flags, g)))
				return DOSFALSE;
#endif
			type.flags.access = ET_SHAREDREAD;
			break;

		case ACTION_FINDUPDATE:

#if MULTIUSER
			if (found)
			{
				if ((*error = muFS_CheckWriteAccess(extrafields.prot, flags, g)))
					return DOSFALSE;
			}
#if MU_CHECKDIR
			else
			{
				if ((*error = muFS_CheckWriteAccess(path_extrafields.prot, path_flags, g)))
					return DOSFALSE;
			}
#endif /* MU_CHECKDIR */
#endif /* MULTIUSER */

			if (!found)
			{
				if ((*error = NewFile (found, &pathfi, filename, &filefi, g)))
				{
					DB(Trace(1, "NewFile", "update failed"));
					return DOSFALSE;
				}
			}

			type.flags.access = ET_SHAREDWRITE;
			break;

		case ACTION_FINDOUTPUT:
			if (found)
			{
#if MULTIUSER
				if ((*error = muFS_CheckDeleteAccess(extrafields.prot, flags, g)))
					return DOSFALSE;

				if ((*error = muFS_CheckWriteAccess(extrafields.prot, flags, g)))
					return DOSFALSE;
			}

#if MU_CHECKDIR
			if ((*error = muFS_CheckWriteAccess(path_extrafields.prot, path_flags, g)))
				return DOSFALSE;
#endif /* MU_CHECKDIR */
#else /* MULTIUSER */
			}
#endif /* MULTIUSER */

			if ((*error = NewFile (found, &pathfi, filename, &filefi, g)))
			{
				DB(Trace(1, "Newfile", "output failed"));
				return DOSFALSE;
			}

			type.flags.access = ET_EXCLWRITE;
			break;

		default:
			*error = ERROR_ACTION_NOT_KNOWN;
			return DOSFALSE;
	}

	/* Add file to list  */
	if (!(filefe = MakeListEntry(&filefi, type, error, g)))
		return DOSFALSE;

	if (!AddListEntry(filefe))
	{
		DB(Trace(1, "dd_Open", "AddListEntry failed"));
		FreeListEntry(filefe, g);
		*error = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	/* if the file was created, the user has to be notified */
	((fileentry_t *) filefe)->checknotify = !found;
	filehandle->fh_Arg1 = (SIPTR)filefe;     // We get this with Read(), Write() etc

	return DOSTRUE;
}

static SIPTR dd_OpenFromLock(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BPTR to filehandle
	// ARG2 = BPTR lock on file to open
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = failurecode (if res1 = DOSFALSE)

	struct FileHandle *filehandle;
	listentry_t *lockentry, *fileentry;
	listtype type;

	filehandle = (struct FileHandle *)BADDR(pkt->dp_Arg1);
	if (!(lockentry = ListEntryFromLock(pkt->dp_Arg2)) ||
		!CheckVolume(lockentry->volume, 0, &pkt->dp_Res2, g))
		return DOSFALSE;

	UpdateLE(lockentry, g);
	if (!IsFile(lockentry->info))
	{
		pkt->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
		return DOSFALSE;
	}

	type.value = (lockentry->lock.fl_Access == EXCLUSIVE_LOCK) ? \
		(ET_FILEENTRY | ET_EXCLWRITE) : (ET_FILEENTRY | ET_SHAREDREAD);
	if (!(fileentry = MakeListEntry(&lockentry->info, type, &pkt->dp_Res2, g)))
		return DOSFALSE;

	/* oude lock verwijderen; nieuwe fileentry toevoegen
	 * addlistentry zou eigenlijk niet fout moeten kunnen gaan
	 */
	RemoveListEntry(lockentry, g);
	if (!AddListEntry(fileentry))   /* should not go wrong */
	{
		FreeListEntry(fileentry, g);
		pkt->dp_Res2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	filehandle->fh_Arg1 = (SIPTR)fileentry;
	return DOSTRUE;
}

static SIPTR dd_Close(struct DosPacket *pkt, globaldata * g)
{
	SIPTR error;
	fileentry_t *fe = (fileentry_t *)pkt->dp_Arg1;

	if (!fe)
		return DOSFALSE;

	if (fe->checknotify)
	{
		if (!CheckVolume(fe->le.volume, 1, &error, g))
			return DOSFALSE;
		UpdateLE((listentry_t *) fe, g);
		Touch(&fe->le.info.file, g);
		if (fe->originalsize != fe->le.info.file.direntry->size)
			UpdateLinks(fe->le.info.file.direntry, g);

		PFSDoNotify(&fe->le.info.file, TRUE, g);
		fe->checknotify = 0;
	}

	RemoveListEntry((listentry_t *) fe, g);
	return DOSTRUE;
}

static SIPTR dd_ChangeMode(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = LONG type of object to change - either CHANGE_FH or CHANGE_LOCK
	// ARG2 = BPTR object to be changed
	// ARG3 = LONG new mode for object
	// RES1 = Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = errorcode

	listentry_t *listentry;

	if (pkt->dp_Arg1 == CHANGE_FH)
		listentry = (listentry_t *)
			(((struct FileHandle *)BADDR(pkt->dp_Arg2))->fh_Arg1);
	else
		listentry = ListEntryFromLock(pkt->dp_Arg2);

	if (listentry)
	{
		if (!CheckVolume(listentry->volume, 0, &pkt->dp_Res2, g))
			return DOSFALSE;

		UpdateLE(listentry, g);
	}
	else
		return DOSFALSE;

	return ChangeAccessMode(listentry, pkt->dp_Arg3, &pkt->dp_Res2);
}

/* reading and seeking in an open file */
static SIPTR dd_SeekRead(struct DosPacket *pkt, globaldata * g)
{
	// ACTION_READ
	// ARG1 = APTR fileentry. (filled in by Open())
	// ARG2 = APTR buffer to put data into
	// ARG3 = LONG #bytes to read
	// RES1 = LONG #bytes read, 0=eof, -1=error
	// RES2 = CODE failurecode if RES1=-1

	// ACTION_SEEK
	// ARG1 = APTR fileentry. (filled in by Open())
	// ARG2 = LONG offset
	// ARG3 = LONG seek mode
	// RES1 = LONG absolute offset before seek
	// RES2 = CODE failurecode if RES1=-1

	listentry_t *listentry;

	listentry = (listentry_t *) pkt->dp_Arg1;
	if (!CheckVolume(listentry->volume, 0, &pkt->dp_Res2, g))
		return -1;

	UpdateLE(listentry, g);

	if (pkt->dp_Type == ACTION_READ)
	{
		return (LONG)ReadFromObject((fileentry_t *) listentry,
									(UBYTE *)pkt->dp_Arg2, (ULONG)pkt->dp_Arg3,
									&pkt->dp_Res2, g);
	}
	else
	{
		return SeekInObject((fileentry_t *) listentry,
							(LONG)pkt->dp_Arg2, (LONG)pkt->dp_Arg3,
							&pkt->dp_Res2, g);
	}
}

/* Write to an open file, or change its size (SFS = SetFileSize) */
static SIPTR dd_WriteSFS(struct DosPacket *pkt, globaldata * g)
{
	// ACTION_WRITE
	// ARG1 = APTR fileentry. (filled in by Open())
	// ARG2 = APTR buffer to put data into
	// ARG3 = LONG #bytes to read
	// RES1 = LONG #bytes read, 0=eof, -1=error
	// RES2 = CODE failurecode if RES1=-1

	// ACTION_SET_FILE_SIZE
	// ARG1 = APTR fileentry. (filled in by Open())
	// ARG2 = LONG offset
	// ARG3 = LONG mode
	// RES1 = LONG new file size
	// RES2 = CODE failurecode if RES1=-1

	listentry_t *listentry;

	listentry = (listentry_t *)pkt->dp_Arg1;
	if (!CheckVolume(listentry->volume, 1, &pkt->dp_Res2, g))
		return -1;
	UpdateLE(listentry, g);

	if (pkt->dp_Type == ACTION_WRITE)
	{
		return (LONG)WriteToObject((fileentry_t *)listentry,
								   (UBYTE *)pkt->dp_Arg2, (ULONG)pkt->dp_Arg3,
								   &pkt->dp_Res2, g);
	}
	else                        /* SetFileSize */
	{
		return ChangeObjectSize((fileentry_t *)listentry,
					   pkt->dp_Arg2, pkt->dp_Arg3, &pkt->dp_Res2, g);
	}
}


/**********************
 * Volume functions
 */

/* change the name of a volume */
static SIPTR dd_Relabel(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BSTR new disk name
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)

	UBYTE newlabel[FNSIZE];
	struct volumedata *volume;
	struct DeviceList *devlist;
	listentry_t *fe;

#if MULTIUSER
	if (g->user->uid != muROOT_UID)
	{
		pkt->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
		return DOSFALSE;
	}
#endif

	BCPLtoCString(newlabel, (DSTR)BARG1(pkt));
	volume = g->currentvolume;

	if (!CheckVolume(volume, 1, &pkt->dp_Res2, g))
		return DOSFALSE;

	/* make new doslist entry COPY VAN DISKINSERTSEQUENCE */
	devlist = (struct DeviceList *)MakeDosEntry(newlabel, DLT_VOLUME);
	if (devlist)
	{
		/* change rootblock */
		if (RenameDisk(newlabel, g))
		{
			/* free old devlist */
//          LockDosList (LDF_VOLUMES|LDF_READ);
			Forbid();
			RemDosEntry((struct DosList *)volume->devlist);
			FreeDosEntry((struct DosList *)volume->devlist);
//          UnLockDosList (LDF_VOLUMES|LDF_READ);
			Permit();

			/* fill in new. Diskname NIET */
			g->currentvolume->devlist = devlist;
			devlist->dl_Task = g->msgport;
			devlist->dl_VolumeDate.ds_Days = volume->rootblk->creationday;
			devlist->dl_VolumeDate.ds_Minute = volume->rootblk->creationminute;
			devlist->dl_VolumeDate.ds_Tick = volume->rootblk->creationtick;
			devlist->dl_LockList = BNULL;    // disk still inserted
			devlist->dl_DiskType = volume->rootblk->disktype;

			/* toevoegen */
			AddDosEntry((struct DosList *)devlist);
			volume->devlist = (struct DeviceList *)devlist;

			/* alle locks veranderen */
			for (fe = HeadOf(&volume->fileentries); fe->next; fe = fe->next)
				fe->lock.fl_Volume = MKBADDR(devlist);
		}
		else
		{
			pkt->dp_Res2 = ERROR_INVALID_COMPONENT_NAME;
			return DOSFALSE;
		}
	}
	else
	{
		pkt->dp_Res2 = ERROR_NO_FREE_STORE;     // ??
		return DOSFALSE;
	}

	return DOSTRUE;
}

/* change the size of the cache */
static SIPTR dd_AddBuffers(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = LONG (number of buffers)
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = new total number of cache buffers or failurecode (if res1 = DOSFALSE)

	struct lru_cachedblock *lrunode, *nextnode;
	LONG numbuffers;

	/* kill old cache */
	UpdateDisk(g);

	for (lrunode = (struct lru_cachedblock *)g->glob_lrudata.LRUqueue.mlh_Head; lrunode->next;
		 lrunode = nextnode)
	{
		FlushBlock(&lrunode->cblk, g);
		nextnode = lrunode->next;
		MinRemove(lrunode);
	}

	FreeVec(g->glob_lrudata.LRUarray);
	numbuffers = g->dosenvec->de_NumBuffers;
	numbuffers = max(0, numbuffers + (LONG)pkt->dp_Arg1);
	g->dosenvec->de_NumBuffers = numbuffers;

	while (!InitLRU(g))
		g->dosenvec->de_NumBuffers--;

	pkt->dp_Res2 = g->dosenvec->de_NumBuffers;
	return (LONG)g->dosenvec->de_NumBuffers;
}

/* disk info */
static SIPTR dd_Info(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = diskinfo: BPTR to InfoData to fill in
	//        info: BPTR lock on volume
	// ARG2 = info: BPTR to InfoData to fill in
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)

	struct InfoData *info;
	lockentry_t *le;
	struct volumedata *volume;

	ENTER("dd_info");
	if (pkt->dp_Type == ACTION_DISK_INFO)
	{
		info = (struct InfoData *)BARG1(pkt);
		volume = g->currentvolume;
	}
	else
	{
		le = LockEntryFromLock(pkt->dp_Arg1);
		volume = le ? (le->le.volume) : (g->currentvolume);
		if (volume != g->currentvolume)
		{
			pkt->dp_Res2 = ERROR_DEVICE_NOT_MOUNTED;
			return DOSFALSE;
		}
		info = (struct InfoData *)BARG2(pkt);
	}

	if (volume)
	{
		info->id_NumSoftErrors = volume->numsofterrors;
		info->id_UnitNumber = g->startup->fssm_Unit;
		info->id_DiskState = g->softprotect ? ID_WRITE_PROTECTED : g->diskstate;
		info->id_NumBlocks = volume->numblocks - g->rootblock->lastreserved
			- g->rootblock->alwaysfree - 1;
		info->id_NumBlocksUsed = info->id_NumBlocks - alloc_data.alloc_available;
		info->id_BytesPerBlock = volume->bytesperblock;
#ifdef KS13WRAPPER
		// 1.x C:Info only understands DOS\0
		info->id_DiskType = DOSBase->dl_lib.lib_Version >= 37 ? ID_INTER_FFS_DISK : ID_DOS_DISK;
#else
		info->id_DiskType = ID_INTER_FFS_DISK;  // c:Info does not like this
#endif
		info->id_VolumeNode = MKBADDR(volume->devlist);
		info->id_InUse = !IsMinListEmpty(&volume->fileentries);

		return DOSTRUE;
	}
	else
	{
		info->id_NumSoftErrors = 0;
		info->id_UnitNumber = g->startup->fssm_Unit;
		info->id_DiskState = g->diskstate;
		info->id_NumBlocks = g->geom->dg_TotalSectors - 2;
		info->id_NumBlocksUsed = 0;
		info->id_BytesPerBlock = g->geom->dg_SectorSize;
		info->id_DiskType = g->disktype;
		info->id_VolumeNode = 0;
		info->id_InUse = 0;

		return DOSTRUE;
	}
}

/* flush cache (BTW: doesn't actually flush the cache right now) */
static SIPTR dd_Flush(struct DosPacket *pkt, globaldata * g)
{
	UpdateDisk(g);
	FreeUnusedResources(g->currentvolume, g);
	g->timeout = 0;
	return DOSTRUE;
}

/* soft-writeprotect disk */
static SIPTR dd_WriteProtect(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BOOL DOSTRUE = write protect; DOSFALSE = un-writeprotect
	// ARG2 = LONG 32 bit pass key
	// RES1 = Success/failure (DOSTRUE/DOSFALSE)

	if (pkt->dp_Arg1)           /* protecting */
	{
		if (g->softprotect)     /* already protected */
			return DOSFALSE;
		UpdateDisk(g);
		/* updatedisk can cause a softprotect, so check again */
		if (!g->softprotect)
		{
			g->softprotect = 1;
			g->protectkey = pkt->dp_Arg2;
		}
		return DOSTRUE;
	}
	else                        /* un protecting */
	{
		if (!g->softprotect)
			return DOSTRUE;
		if (g->softprotect < 0)
			return DOSFALSE;

		if (!g->protectkey || g->protectkey == pkt->dp_Arg2 ||
			 g->protectkey == ~0)
		{
			g->softprotect = 0;
			g->protectkey = 0;
			return DOSTRUE;
		}
		return DOSFALSE;
	}
}

/* make disk unique by timestamping it */
static SIPTR dd_SerializeDisk(struct DosPacket *pkt, globaldata * g)
{
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = failurecode (if res1 = DOSFALSE)
	struct DateStamp time;
	struct rootblock *rbl;

	DateStamp(&time);

	/* bitmap is not needed, so read just bear rootblock */
	if (!(rbl = AllocBufmem(BLOCKSIZE, g)))
	{
		pkt->dp_Res2 = ERROR_NO_FREE_STORE;
		return DOSFALSE;
	}

	pkt->dp_Res2 = RawRead((UBYTE *)rbl, 1, ROOTBLOCK, g);
	if (pkt->dp_Res2)
		goto inh_error;

	/* Adding 3 to the tick prevents problems after format */
	rbl->creationday = (UWORD)time.ds_Days;
	rbl->creationminute = (UWORD)time.ds_Minute;
	rbl->creationtick = (UWORD)time.ds_Tick + 3;
	pkt->dp_Res2 = RawWrite((UBYTE *)rbl, 1, ROOTBLOCK, g);
	if (pkt->dp_Res2)
		goto inh_error;

	g->request->iotd_Req.io_Command = CMD_UPDATE;
	DoIO((struct IORequest *)g->request);
	FreeBufmem(rbl, g);
	return DOSTRUE;

  inh_error:
	FreeBufmem(rbl, g);
	return DOSFALSE;
}


/**********************
 * Object functions
 */
static SIPTR dd_DeleteObject(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = Lock to which ARG2 is relative (BPTR)
	// ARG2 = BSTR Name of object to be deleted
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = failurecode (if res1 = DOSFALSE)

	lockentry_t *parentfe;
	union objectinfo *parentfi, filefi;
	UBYTE *filename, pathname[PATHSIZE];
	SIPTR *error = &pkt->dp_Res2;
#if MULTIUSER
	struct extrafields extrafields;
	ULONG flags;
#endif

	GetFileInfoFromLock(pkt->dp_Arg1, 1, parentfe, parentfi);
	BCPLtoCString(pathname, (DSTR)BARG2(pkt));
	SkipColon(filename, pathname);
	LocateFile(parentfi, filename, filefi, error);

#if MULTIUSER
#if DELDIR
	GetExtraFieldsOI(&filefi, &extrafields, g);
#else /* DELDIR */
	GetExtraFields(filefi.file.direntry, &extrafields);
#endif /* DELDIR */
	flags = muGetRelationship(extrafields);
	if (*error = muFS_CheckDeleteAccess(extrafields.prot, flags, g))
		return DOSFALSE;
#endif /* MULTIUSER */

	PFSDoNotify(&filefi.file, TRUE, g);
	return DeleteObject(&filefi, &pkt->dp_Res2, g);
}

static SIPTR dd_Rename(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BPTR Lock to which ARG2 is relative
	// ARG2 = BSTR Name of object to rename
	// ARG3 = BPTR Lock to target directory
	// ARG4 = BSTR New name of object
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = failurecode (if res1 = DOSFALSE)

	lockentry_t *srcdirfe, *dstdirfe;
	union objectinfo *srcdirfi, *dstdirfi;
	UBYTE *srcname, srcpathname[PATHSIZE], *dstname, dstpathname[PATHSIZE];
	struct volumedata *srcvol, *dstvol;
	SIPTR *error = &pkt->dp_Res2;
	union objectinfo pathoi, sourceoi;
	UBYTE *objectname;
	BOOL result;
#if MULTIUSER
	struct extrafields extrafields;
	ULONG flags;
#endif

	GetFileInfoFromLock(pkt->dp_Arg1, 1, srcdirfe, srcdirfi);
	GetFileInfoFromLock(pkt->dp_Arg3, 1, dstdirfe, dstdirfi);
	BCPLtoCString(srcpathname, (DSTR)BARG2(pkt));
	BCPLtoCString(dstpathname, (DSTR)BARG4(pkt));
	DB(Trace(1, "Rename", "renaming %s to %s\n", srcpathname, dstpathname));

	SkipColon(srcname, srcpathname);
	SkipColon(dstname, dstpathname);

	/* check rename across devices (NOT needed; done by DOS */
	srcvol = srcdirfe ? srcdirfe->le.volume : g->currentvolume;
	dstvol = dstdirfe ? dstdirfe->le.volume : g->currentvolume;
	if (srcvol != dstvol)
	{
		*error = ERROR_RENAME_ACROSS_DEVICES;
		return DOSFALSE;
	}
	else
	{
#if MU_CHECKDIR
		--> check source AND destination directories against write access
#endif /* MU_CHECKDIR */

		if (!(objectname = GetFullPath (srcdirfi, srcname, &pathoi, error, g)) ||
			!FindObject (&pathoi, objectname, &sourceoi, error, g))
			return DOSFALSE;

		CheckPropertyAccess (sourceoi, extrafields, flags, error);
		if (!CheckVolume(srcvol, 1, error, g))
			return DOSFALSE;
		else
		{
			result = RenameAndMove (&pathoi, &sourceoi, dstdirfi, dstname,
				error, g);
			return result;
		}
	}
}

/* change a object property: filenote, protection, owner, date */
static SIPTR dd_SetProperty(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = Unused
	// ARG2 = BPTR Lock to which arg3 is relative
	// ARG3 = BSTR Name of object
	// ARG4 = LONG new property
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = failurecode (if res1 = DOSFALSE)

	lockentry_t *parentfe;
	union objectinfo *parentfi, objectfi;
	SIPTR *error = &pkt->dp_Res2;
	UBYTE *filename, pathname[PATHSIZE], comment[PATHSIZE];
#if MULTIUSER
	struct extrafields extrafields;
	ULONG flags;
#endif

	GetFileInfoFromLock(pkt->dp_Arg2, 1, parentfe, parentfi);
	BCPLtoCString(pathname, (DSTR)BARG3(pkt));
	SkipColon(filename, pathname);
	LocateFile(parentfi, filename, objectfi, error);
	CheckPropertyAccess(objectfi, extrafields, flags, error);
	PFSDoNotify(&objectfi.file, TRUE, g);

	switch (pkt->dp_Type)
	{
		case ACTION_SET_PROTECT:
			return ProtectFile(&objectfi.file, (ULONG)pkt->dp_Arg4, error, g);

		case ACTION_SET_COMMENT:
			BCPLtoCString(comment, (DSTR)BARG4(pkt));
			return AddComment(&objectfi, comment, error, g);

		case ACTION_SET_DATE:
			return SetDate(&objectfi, (struct DateStamp *)pkt->dp_Arg4, error, g);

		case ACTION_SET_OWNER:
			return SetOwnerID(&objectfi.file, pkt->dp_Arg4, error, g);
	}

	return DOSFALSE;
}

/*
 * Examine directory contents
 */
static SIPTR dd_Examine(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = LOCK or filehandle of object to examine
	// ARG2 = BPTR FileInfoBlock to fill in
	// RES1 = Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = errorcode

	listentry_t *listentry;

	if (pkt->dp_Type == ACTION_EXAMINE_FH)
		listentry = (listentry_t *) pkt->dp_Arg1;
	else
		listentry = ListEntryFromLock(pkt->dp_Arg1);

	if (listentry)
	{
		if (!CheckVolume(listentry->volume, 0, &pkt->dp_Res2, g))
			return DOSFALSE;

		UpdateLE(listentry, g);
		UpdateLE_exa((lockentry_t *) listentry, g);
	}
	else if (!g->currentvolume)
	{
		pkt->dp_Res2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	if (pkt->dp_Type == ACTION_EXAMINE_NEXT)
		return ExamineNextFile((lockentry_t *) listentry,
							   (struct FileInfoBlock *)BARG2(pkt),
							   &pkt->dp_Res2, g);
	else
		return ExamineFile(listentry,
						   (struct FileInfoBlock *)BARG2(pkt),
						   &pkt->dp_Res2, g);
}

static SIPTR dd_ExamineAll(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BPTR Lock on directory to examine
	// ARG2 = APTR Buffer to store resultsobject to be changed
	// ARG3 = LONG Length (in bytes) of buffer (arg2)
	// ARG4 = LONG Type of request
	// ARG5 = APTR (!!) Control structure to store information
	// RES1 = Continuation-flag - DOSFALSE indicates termination
	// RES2 = Failure code if RES1 is DOSFALSE

	listentry_t *listentry;
	DB(Trace(1, "ExamineAll", "1: %lx 2: %lx 3: %ld 4: %ld 5: %lx\n", pkt->dp_Arg1, pkt->dp_Arg2,
			 pkt->dp_Arg3, pkt->dp_Arg4, pkt->dp_Arg5));

	listentry = ListEntryFromLock(pkt->dp_Arg1);
	if (listentry)
	{
		if (!CheckVolume(listentry->volume, 0, &pkt->dp_Res2, g))
			return DOSFALSE;

		UpdateLE(listentry, g);
		UpdateLE_exa((lockentry_t *) listentry, g);
	}
	else if (!g->currentvolume)
	{
		pkt->dp_Res2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	return ExamineAll((lockentry_t *)listentry,
					  (UBYTE *)pkt->dp_Arg2, (ULONG)pkt->dp_Arg3, pkt->dp_Arg4,
					  (struct ExAllControl *)(pkt->dp_Arg5),
					  &pkt->dp_Res2, g);
}

/* make a hard or softlink */
static SIPTR dd_MakeLink(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BPTR LOCK on directory ARG2 is relative to
	// ARG2 = BSTR Name of link
	// ARG3 = BPTR FileLock (LINK_HARD)
	//        const char *  (LINK_SOFT)
	// ARG4 = LONG type of link
	// RES1 = success
	// RES2 = failure code

	lockentry_t *parentle, *targetle;
	union objectinfo path, linkinfo, *parentfi;
	UBYTE linkname[PATHSIZE];
	UBYTE *zonderpad, *fullname;
	BOOL result;
#if MULTIUSER
#if MU_CHECKDIR
	struct extrafields extrafields;
	ULONG flags;
#endif
#endif
	SIPTR *error = &pkt->dp_Res2;

	GetFileInfoFromLock(pkt->dp_Arg1, 1, parentle, parentfi);
	BCPLtoCString(linkname, (DSTR)BARG2(pkt));
	SkipColon(fullname, linkname);

	if (!(zonderpad = GetFullPath(parentfi, fullname, &path, &pkt->dp_Res2, g)))
		return DOSFALSE;

#if MULTIUSER
#if MU_CHECKDIR
	if (!IsVolume(path))
	{
#if DELDIR
		GetExtraFieldsOI(&path, &extrafields);
#else /* DELDIR */
		GetExtraFields(path.file.direntry, &extrafields);
#endif /* DELDIR */
		flags = muGetRelationship(extrafields);
		if (pkt->dp_Res2 = muFS_CheckWriteAccess(extrafields.prot, flags, g))
			return DOSFALSE;
	}
#endif /* MU_CHECKDIR */
#endif /* MULTIUSER */


	/*
	 * Arg4 sometimes seems to be -1 instead of 1 when softlinks are meant,
	 * so I will treat everything not LINK_HARD as LINK_SOFT
	 */
	if (pkt->dp_Arg4 != LINK_HARD)
	{
		result = CreateSoftLink(&path, zonderpad, (STRPTR) pkt->dp_Arg3, &linkinfo,
								&pkt->dp_Res2, g);
		if (result)
			PFSDoNotify(&linkinfo.file, TRUE, g);

		return result;
	}

	/* check if lock is ours (it could be alien) */
	targetle = LockEntryFromLock(pkt->dp_Arg3);
	if (!targetle || (BADDR(targetle->le.lock.fl_Volume) != g->currentvolume->devlist))
	{
		pkt->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
		return DOSFALSE;
	}

	/* don't do this until you know the lock is ours */
	UpdateLE((listentry_t *)targetle, g);

	result = CreateLink(&path, zonderpad, &targetle->le.info, &linkinfo, &pkt->dp_Res2, g);
	if (result)
	{
		PFSUpdateNotify(linkinfo.file.dirblock->blk.anodenr, zonderpad,
					 targetle->le.info.file.direntry->anode, g);
		PFSDoNotify(&linkinfo.file, TRUE, g);
	}

	return result;
}

/* read a softlink: finding out which file (ascii) it is referring to */
static SIPTR dd_ReadLink(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BPTR LOCK on directory ARG2 is relative to
	// ARG2 = BSTR Name of link
	// ARG3 = STRPTR buffer to store result
	// ARG4 = ULONG size of buffer
	// RES1 = success
	// RES2 = failure code

	lockentry_t *parentle;
	union objectinfo linkfi, *parentfi;
	char *fullname;
	SIPTR *error = &pkt->dp_Res2;

	GetFileInfoFromLock(pkt->dp_Arg1, 0, parentle, parentfi);

	/* strip upto first : */
	SkipColon(fullname, (char *)pkt->dp_Arg2);

	if (!(FindObject(parentfi, fullname, &linkfi,
					 &pkt->dp_Res2, g)))
	{
		if (pkt->dp_Res2 != ERROR_IS_SOFT_LINK)
			return DOSFALSE;
	}

	return ReadSoftLink(&linkfi, (char *)pkt->dp_Arg3, pkt->dp_Arg4, &pkt->dp_Res2, g);
}


/**********************
 * goto inhibited state
 */

/* inhibit called from uninhibited state */
static SIPTR dd_InhibitOn(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BOOL DOSTRUE = inhibit; DOSFALSE = uninhibit
	// RES1 = Success/failure (DOSTRUE/DOSFALSE)

	if (pkt->dp_Arg1 != DOSFALSE)   /* don't check for DOSTRUE (Holger Kruse!) */
	{
		while (g->currentvolume)    /* inefficiënt.. */
			DiskRemoveSequence(g);
		g->inhibitcount++;
		g->timeron = FALSE;
		g->timeout = 0;
		g->DoCommand = InhibitedCommands;
		g->disktype = ID_BUSY;
	}

	/* else ->already uninhibited */
	return DOSTRUE;
}

/* Inhibit called from inhibited state */
static SIPTR dd_InhibitOff(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = BOOL DOSTRUE = inhibit; DOSFALSE = uninhibit
	// RES1 = Success/failure (DOSTRUE/DOSFALSE)

	if (pkt->dp_Arg1 != DOSFALSE)   /* don't check for DOSTRUE (Holger Kruse) */
	{
		g->inhibitcount++;
	}
	else
	{
		g->inhibitcount--;
		if (g->inhibitcount <= 0)
		{
			if (g->trackdisk)
			{
				g->request->iotd_Req.io_Command = CMD_CLEAR;
				DoIO((struct IORequest *)g->request);
			}

			g->DoCommand = NormalCommands;
			g->timeron = FALSE;
			NewVolume(TRUE, g);
		}
	}

	return DOSTRUE;
}


static SIPTR dd_Format(struct DosPacket *pkt, globaldata * g)
{
	/* argumenten stemmen NIET met de dosmanual overeen */
	// ARG1 = BSTR Name of device (with trailing ':')
	// ARG2 = LONG Type of format (file system specific ==> ID_FDOS_DISK of 0)
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = failurecode (if res1 = DOSFALSE)

#if MULTIUSER
	if (g->user->uid != muROOT_UID)
	{
		pkt->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
		return DOSFALSE;
	}
#endif

	/* Ik neem aan dat er geen Lock check nodig is ... */
	/* if not inhibited then 'remove disk' */
	if (g->inhibitcount == 0)
	{
		g->dirty = FALSE;
		while (g->currentvolume)
			DiskRemoveSequence(g);  // should always succeed now

	}

	/* format disk */
	return FDSFormat((DSTR)BADDR(pkt->dp_Arg1), pkt->dp_Arg2, &pkt->dp_Res2, g);
}


/**********************
* Notify stuff
*/
static SIPTR dd_AddNotify (struct DosPacket *pkt, globaldata *g)
{
	// ARG1 = APTR struct NotifyRequest *
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = failurecode (if res1 = DOSFALSE)

	struct NotifyRequest *nr = (struct NotifyRequest *)pkt->dp_Arg1;
	struct notifyobject *no;
	union objectinfo oi, filefi;
	char *name, *temp;
	BOOL found = FALSE;
	int t;

	if (!(no = AllocMemP (sizeof(struct notifyobject), g)))
		goto memerror1;

	/* strip upto first : */
	name = strchr (nr->nr_FullName, ':');
	if (name)
		name++;
	else
		name = nr->nr_FullName;

	/* search for notification object */
	no->req = nr;
	no->unparsed = NULL;
	temp = FilePart (name);
	t = strlen (temp);
	if (!(no->objectname = AllocMemP (t+2, g)))
		goto memerror2;
	ctodstr (temp, no->objectname);
	intltoupper (no->objectname);

	if (!(GetFullPath(NULL, name, &oi, &pkt->dp_Res2, g)))
	{
		if (g->unparsed)
		{
			temp = PathPart (g->unparsed);
			*temp = 0;          /* cut of filepart */
			t = strlen (g->unparsed);
			if (!(no->unparsed = no->namemem = AllocMemP(t+2, g)))
				goto memerror3;
			ctodstr (g->unparsed, no->unparsed);
			intltoupper (no->unparsed);
			no->unparsed++;     /* make it a cstring!! */
		}
		else
			return DOSFALSE;
	}
	else
	{
		/* try to locate object */
		found = FindObject (&oi, no->objectname+1, &filefi, &pkt->dp_Res2, g);
		if (found)
		{
			no->anodenr = IsVolume(filefi) ? ANODE_ROOTDIR : filefi.file.direntry->anode;
		}
	}

#if DELDIR
	if (IsDelDir (oi))
	{
		pkt->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
		return DOSFALSE;
	}
#endif

	/* set object id (anodenr) */
	if (IsVolume (oi))
		no->parentanodenr = ANODE_ROOTDIR;
	else
		no->parentanodenr = oi.file.direntry->anode;

	/* add notification to list */
	MinAddHead (&g->currentvolume->notifylist, no);

	/* do initial message, if necessary */
	if (found && (nr->nr_Flags & NRF_NOTIFY_INITIAL))
		NotifyUser (nr, g);

	return DOSTRUE;

  memerror3:
	FreeMemP (no->objectname, g);
  memerror2:
	FreeMemP (no, g);
  memerror1:
	pkt->dp_Res2 = ERROR_NO_FREE_STORE;
	return DOSFALSE;
}

static SIPTR dd_RemoveNotify (struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = APTR struct NotifyRequest *
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = failurecode (if res1 = DOSFALSE)

	struct NotifyRequest *nr = (struct NotifyRequest *)pkt->dp_Arg1;
	struct notifyobject *no;

	/* 
	 * BTW: pending messages that return after RemoveNotify
	 * may cause trouble
	 */

	/* remove from queue */
	for (no = HeadOf(&g->currentvolume->notifylist); no->next; no = no->next)
	{
		if (no->req == nr)
		{
			MinRemove (no);
			FreeMemP (no->namemem, g);
			FreeMemP (no->objectname, g);
			FreeMemP (no, g);
			break;
		}
	}

	return DOSTRUE;
}


/**********************
 * internal and PFS2 extended packets
 */

/*
 * Check if packet really is a PFS2 custom packet
 */
static int dd_CheckCustomPacket(LONG id)
{
	if (id == ID_PFS2_DISK || id == ID_AFS_DISK)
		return 1;
	else
		return 0;
}

/*
 * Check if filesystem is PFS2
 */
static SIPTR dd_IsPFS2 (struct DosPacket *pkt, globaldata *g)
{
	// ARG1 = ID_PFS2_DISK
	// RES1 = DOSTRUE (is PFS2), otherwise it isn't
	// RES2 = Upper: version, Lower: revision

	if (!dd_CheckCustomPacket(pkt->dp_Arg1))
	{
		return NotKnown (pkt, g);
	}
	else
	{
		pkt->dp_Res2 = (VERNUM<<16) + REVNUM;
		return DOSTRUE;
	}
}

/*
 * Remove empty file
 */
static SIPTR dd_KillEmpty(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = lock to parent directory
	// ARG2 = ID_PFS2_DISK or ID

	lockentry_t *parentfe;
	union objectinfo *parentfi;
	SIPTR *error = &pkt->dp_Res2;

	if (!dd_CheckCustomPacket(pkt->dp_Arg2))
		return DOSFALSE;

	GetFileInfoFromLock(pkt->dp_Arg1, 1, parentfe, parentfi);
	return KillEmpty(parentfi, g);
}


/*
 * remove direntry, without freeing diskspace or checking
 * validity. Useful for corrupt files & entries.
 */
static SIPTR dd_RemoveDirEntry(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = ID_PFS2_DISK
	// ARG2 = Lock to which ARG2 is relative (BPTR)
	// ARG3 = STRPTR Name of object to be deleted
	// RES1 = BOOL Success/failure (DOSTRUE/DOSFALSE)
	// RES2 = failurecode (if res1 = DOSFALSE)

	lockentry_t *parentfe;
	union objectinfo *parentfi, filefi;
	UBYTE *filename, *pathname;
	SIPTR *error = &pkt->dp_Res2;

	if (!dd_CheckCustomPacket(pkt->dp_Arg1))
		return DOSFALSE;

	GetFileInfoFromLock(pkt->dp_Arg2, 1, parentfe, parentfi);
	pathname = (UBYTE *)pkt->dp_Arg3;
	SkipColon(filename, pathname);
	LocateFile(parentfi, filename, filefi, error);

	return forced_RemoveDirEntry(&filefi, &pkt->dp_Res2, g);
}

#if EXTRAPACKETS

/*
 * exit or leave SLEEP_MODE
 */
static SIPTR dd_Sleep(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = MODE_PFS2_DISK
	// ARG2 = on/off (DOSTRUE = on; DOSFALSE = off)
	// ARG3 = ULONG signalnr
	// ARG4 = struct Task *task
	// RES1 = struct MsgPort *sleepport or NULL for failure;
	// RES2 = errorcode

	if (!dd_CheckCustomPacket(pkt->dp_Arg1))
		return DOSFALSE;

	if (g->sleepmode)
	{
		if (pkt->dp_Arg2 == DOSFALSE)
		{
			Awake(g);
			g->sleeptask = NULL;
			g->alarmsignal = 0;
			return DOSTRUE;
		}
		else
		{
			pkt->dp_Res2 = ERROR_OBJECT_IN_USE;     /* already locked */
			return DOSFALSE;
		}
	}
	else
	{
		if (pkt->dp_Arg2 == DOSFALSE)
			return DOSFALSE;
		else
		{
			g->sleeptask = (struct Task *)pkt->dp_Arg4;
			g->alarmsignal = (ULONG)pkt->dp_Arg3;
			Sleep(g);
			return (SIPTR)g->sleepport;
		}
	}
}

/*
 * add or remove Idle message
 *
 * ARG1 = MODE_PFS2_DISK
 * ARG2 = add/remove (DOSTRUE = add; DOSFALSE = remove)
 * ARG3 = ULONG signalnr (UPPER = read, LOWER = write)
 *        remove: handle to remove
 * ARG4 = struct Task *task
 * RES1 = Idlehandle or DOSFALSE for failure
 * RES2 = errorcode
 */
static SIPTR dd_SignalIdle(struct DosPacket *pkt, globaldata * g)
{
	struct idlehandle *handle;

	if (!dd_CheckCustomPacket(pkt->dp_Arg1))
		return DOSFALSE;

	if (pkt->dp_Arg2 == DOSTRUE)
	{
		handle = (struct idlehandle *)AllocMemP (sizeof(struct idlehandle), g);
		if (!handle)
			return DOSFALSE;

		handle->task = (struct Task *)pkt->dp_Arg4;
		handle->cleansignal = pkt->dp_Arg3 >> 16;
		handle->dirtysignal = pkt->dp_Arg3 & 0xffff;
		MinAddHead (&g->idlelist, handle);
		return (SIPTR)handle;
	}
	else
	{
		MinRemove (pkt->dp_Arg3);
		FreeMemP ((void *)pkt->dp_Arg3, g);
		return DOSTRUE;
	}
}


static SIPTR dd_UpdateAnode(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = MODE_PFS2_DISK
	// ARG2 = old anodenr
	// ARG3 = new anodenr (0 = invalidate references)
	// RES1 = updatecount or -1 for failure
	// RES2 = failurecode if Res1 = -1

	if (!dd_CheckCustomPacket(pkt->dp_Arg1))
		return -1;

	return UpdateAnode(pkt->dp_Arg2, pkt->dp_Arg3, g);
}
#endif

#if ROLLOVER
static SIPTR dd_MakeRollover(struct DosPacket *pkt, globaldata * g)
{
	// ARG1 = MODE_PFS2_DISK
	// ARG2 = BPTR LOCK on directory ARG3 is relative to
	// ARG3 = APTR name of rollover file 
	// ARG4 = WORD desired rollover size in blocks
	// RES1 = success
	// RES2 = failure code

	lockentry_t *parentle;
	union objectinfo path, rolloverinfo, *parentfi;
	UBYTE *rollovername;
	UBYTE *zonderpad, *fullname;
	SIPTR *error = &pkt->dp_Res2;
	BOOL result;

	if (!dd_CheckCustomPacket(pkt->dp_Arg1))
		return DOSFALSE;

	GetFileInfoFromLock(pkt->dp_Arg2, 1, parentle, parentfi);
	rollovername = (UBYTE *)pkt->dp_Arg3;
	SkipColon(fullname, rollovername);

	if (!(zonderpad = GetFullPath(parentfi, fullname, &path, error, g)))
		return DOSFALSE;

	result = CreateRollover(&path, zonderpad, (ULONG)pkt->dp_Arg4,
								&rolloverinfo, error, g);
	if (result)
	{
		PFSDoNotify (&rolloverinfo.file, TRUE, g);
	}

	return result;
}

/* Read and set rollover info
 */
static SIPTR dd_SetRollover(struct DosPacket *pkt, globaldata *g)
{
	// ARG1 = MODE_PFS2_DISK
	// ARG2 = APTR fileentry (filled in by Open())
	// ARG3 = APTR struct rolloverinfo
	// RES1 = success
	// RES2 = failure code

	listentry_t *rlfile;
	SIPTR *error = &pkt->dp_Res2;

	if (!dd_CheckCustomPacket(pkt->dp_Arg1))
		return DOSFALSE;
	rlfile = (listentry_t *)pkt->dp_Arg2;
	if (!CheckVolume(rlfile->volume, 1, error, g))
		return 0;
	if ((*error = SetRollover((fileentry_t *)rlfile, (struct rolloverinfo *)pkt->dp_Arg3, g)))
		return DOSFALSE;
	return DOSTRUE;
}
#endif

#if DELDIR
static SIPTR dd_SetDeldir(struct DosPacket *pkt, globaldata *g)
{
	// ARG1 = MODE_PFS2_DISK
	// ARG2 = number of deldirblocks wanted (0 = disable, -1 = check)
	// RES1 = success
	// RES2 = failure code

	SIPTR *error = &pkt->dp_Res2;

	if (!dd_CheckCustomPacket(pkt->dp_Arg1))
		return DOSFALSE;

#if MULTIUSER
	if (g->user->uid != muROOT_UID)
	{
		*error = ERROR_DISK_WRITE_PROTECTED;
		return DOSFALSE;
	}
#endif

	/* check if a volume is inserted */
	if (!CheckVolume(g->currentvolume, 1, error, g))
		return DOSFALSE;

	/* just an inquiry */
	if (pkt->dp_Arg2 == -1)
	{
		*error = g->currentvolume->rblkextension->blk.deldirsize;
		return DOSTRUE;
	}

	if ((*error = SetDeldir(pkt->dp_Arg2, g)))
		return DOSFALSE;

	return DOSTRUE;
}

#endif

/* set filename size. failure codes:
 * ACTION_BAD_NUMBER = grootte illegaal of kleiner dan huidig
 */
static SIPTR dd_SetFileSize(struct DosPacket *pkt, globaldata *g)
{
	// ACTION_SET_FNSIZE 2222
	// ARG1 = MODE_PFS2_DISK
	// ARG2 = New maximum filename size (0 = huidige waarde opvragen)
	// RES1 = success
	// RES2 = failure code / current or new fnsize

	if (!dd_CheckCustomPacket(pkt->dp_Arg1))
		return DOSFALSE;

	if (pkt->dp_Arg2)
	{
		if (pkt->dp_Arg2 < 30 || pkt->dp_Arg2 < FILENAMESIZE)
		{
			pkt->dp_Res2 = ERROR_BAD_NUMBER;
			return DOSFALSE;
		}
		
		if (pkt->dp_Arg2 >= FNSIZE)
		{
			pkt->dp_Res2 = ERROR_OBJECT_TOO_LARGE;
			return DOSFALSE;
		}
	}

	if (g->currentvolume && g->currentvolume->rblkextension)
	{
		if (pkt->dp_Arg2)
		{
			g->rootblock->options |= MODE_LONGFN;
			g->fnsize = g->currentvolume->rblkextension->blk.fnsize = pkt->dp_Arg2;
			g->dirty = TRUE;
		}
		pkt->dp_Res2 = g->fnsize;
		return DOSTRUE;
	}

	pkt->dp_Res2 = ERROR_NO_DISK;
	return DOSFALSE;
}

#if defined(__MORPHOS__)
static LONG dd_MorphOSQueryAttr(struct DosPacket *pkt, globaldata *g)
{
	// ACTION_QUERY_ATTR 26407
	// ARG1 = LONG attr, which attribute you want to know about
	// ARG2 = void *storage, memory to hold the return value
	// ARG3 = LONG storagesize, size of storage reserved for
	// RES1 = success
	// RES2 = failure code
	//   ERROR_BAD_NUMBER for unimplemented/unknown attributes
	//   ERROR_LINE_TOO_LONG for buffer too small to hold the result

	APTR storage = (APTR) pkt->dp_Arg2;
	LONG storage_size = pkt->dp_Arg3;

	switch (pkt->dp_Arg1)
	{
		case FQA_MaxFileNameLength:
			if (storage_size >= sizeof(LONG))
			{
				*(LONG *)storage = FILENAMESIZE - 1;
				return DOSTRUE;
			}
			break;

		case FQA_MaxVolumeNameLength:
			if (storage_size >= sizeof(LONG))
			{
				*(LONG *)storage = DNSIZE - 1;
				return DOSTRUE;
			}
			break;

		case FQA_IsCaseSensitive:
			if (storage_size >= sizeof(LONG))
			{
				*(LONG *)storage = FALSE;
				return DOSTRUE;
			}
			break;

		case FQA_MaxFileSize:
			if (storage_size >= sizeof(QUAD))
			{
				*(QUAD *)storage = 0x7fffffffLL;
				return DOSTRUE;
			}
			break;

		/* let dos.library handle these - fall thru */
		case FQA_DeviceType:
		case FQA_NumBlocks:
		case FQA_NumBlocksUsed:

		/* unknown/unhandled attribute */
		default:
			pkt->dp_Res2 = ERROR_BAD_NUMBER;
			return DOSFALSE;
	}

	/* Not enough buffer storage */
	pkt->dp_Res2 = ERROR_LINE_TOO_LONG;
	return DOSFALSE;
}
#endif
