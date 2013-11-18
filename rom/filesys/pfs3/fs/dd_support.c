/* $Id$ */
/* $Log: dd_support.c $
 * Revision 1.9  1999/02/22  16:33:43  Michiel
 * Changes for increasing deldir capacity
 *
 * Revision 1.8  1998/09/27  11:26:37  Michiel
 * ErrorMsg param
 *
 * Revision 1.7  1997/03/03  22:04:04  Michiel
 * Release 16.21
 *
 * Revision 1.6  1996/03/29  17:02:54  Michiel
 * update sleeproutines
 *
 * Revision 1.5  1996/01/30  12:49:23  Michiel
 * --- working tree overlap ---
 * new notify routines
 * */

/**********************
 * function prototypes
 */

static BOOL RenameDisk(UBYTE *, globaldata *);

static void NotifyUser(struct NotifyRequest *nr, globaldata *g);
static struct notifyobject *CheckNotify(ULONG parentanodenr, DSTR objectname,
	struct notifyobject *no, BOOL checkparent, globaldata *);
void PFSDoNotify(struct fileinfo *object, BOOL checkparent, globaldata *g);
void PFSUpdateNotify(ULONG dirnode, DSTR filename, ULONG anodenr, globaldata *g);

/* AFS extensions */
#if EXTRAPACKETS
static void Sleep (globaldata *g);
static void Awake (globaldata *g);
static void Alarm (globaldata *g);
static LONG UpdateAnode (ULONG old, ULONG new, globaldata *g);
void HandleSleepMsg (globaldata *g);
#endif

/**********************
 * macros
 */

#define SkipColon(filename,pathname)                                \
do {                                                                   \
	if ((filename = strchr ((pathname), ':')))                       \
		filename++;                                                 \
	else                                                            \
		filename=(pathname);                                        \
} while (0)

#define LocateFile(parentfi,filename,objectfi,error)                \
do {                                                                   \
	if (!FindObject (parentfi, filename, &(objectfi), error, g))    \
		return DOSFALSE;                                            \
																	\
	if (IsVolume(objectfi))                                         \
	{                                                               \
		*error = ERROR_OBJECT_WRONG_TYPE;                           \
		return DOSFALSE;                                            \
	}                                                               \
} while (0)

#if MULTIUSER
#define muGetRelationship(extrafields)                              \
	((g->muFS_ready) ?                                              \
	(muGetRelationshipA (g->user, ((extrafields).uid << 16) + (extrafields).gid, NULL)) :   \
	((((extrafields).uid == muNOBODY_UID) << muRelB_NO_OWNER) | muRelF_NOBODY))

#if DELDIR
#define CheckPropertyAccess(objectfi,extrafields,flags, error)      \
do {                                                                   \
	GetExtraFieldsOI (&(objectfi), &(extrafields), g);                  \
	if (g->muFS_ready)                                              \
		flags = muGetRelationshipA (g->user, ((extrafields).uid << 16) + (extrafields).gid, NULL);  \
	else                                                                                        \
		flags = (((extrafields).uid == muNOBODY_UID) << muRelB_NO_OWNER) | muRelF_NOBODY;         \
																	\
	if (!(flags & muRel_PROPERTY_ACCESS))                           \
	{                                                               \
		*error = ERROR_WRITE_PROTECTED;                             \
		return DOSFALSE;                                            \
	}                                                               \
} while (0)
#else /* DELDIR */
#define CheckPropertyAccess(objectfi,extrafields,flags, error)      \
do {                                                                   \
	GetExtraFields ((objectfi).file.direntry, &(extrafields));          \
	if (g->muFS_ready)                                              \
		flags = muGetRelationshipA (g->user, ((extrafields).uid << 16) + (extrafields).gid, NULL);  \
	else                                                                                        \
		flags = (((extrafields).uid == muNOBODY_UID) << muRelB_NO_OWNER) | muRelF_NOBODY;         \
																	\
	if (!(flags & muRel_PROPERTY_ACCESS))                           \
	{                                                               \
		*error = ERROR_WRITE_PROTECTED;                             \
		return DOSFALSE;                                            \
	}                                                               \
} while (0)
#endif /* DELDIR */

#else /* MULTIUSER */
#define muGetRelationship(extrafields)
#define CheckPropertyAccess(objectfi,extrafields,flags,error)   
#endif /* MULTIUSER */

/*
 * BPTR, BOOL, listentry_t *, union objectinfo *
 */
#define GetFileInfoFromLock(argument, access, fe, fi)               \
do {                                                                   \
	if ((fe = LockEntryFromLock (argument)))                        \
	{                                                               \
		if (!CheckVolume ((fe)->le.volume, access, error, g))         \
			return DOSFALSE;                                        \
		UpdateLE ((listentry_t *)(fe), g);                                           \
		fi = &(fe)->le.info;                                             \
	}                                                               \
	else                                                            \
	{                                                               \
		if (!g->currentvolume)                                      \
		{                                                           \
			*error = ERROR_NO_DISK;                                 \
			return DOSFALSE;                                        \
		}                                                           \
		fi = NULL;                                                  \
	}                                                               \
} while (0)



/**********************
 * rename disk
 */

static BOOL RenameDisk (UBYTE *newname, globaldata *g)  //%4.5
{
  UBYTE *diskname;
  struct volumedata *volume = g->currentvolume;

	ENTER("RenameDisk");

	if (g->currentvolume && (strlen(newname) < DNSIZE))
	{
		/* Write changes directly to disk to prevent disk recognition problems.
		 * (So DON'T use updatedisk)
		*/
		diskname = volume->rootblk->diskname;
		*diskname = strlen(newname);
		CopyMem (newname, diskname+1, strlen(newname));
		RawWrite ((UBYTE *)volume->rootblk, 1, ROOTBLOCK, g);   /* %7.2 */
		g->request->iotd_Req.io_Command = CMD_UPDATE;
		DoIO ((struct IORequest *)g->request);
		volume->rootblockchangeflag = FALSE;
		return DOSTRUE;
	}
	else
	{
		return DOSFALSE;
	}
}


/**********************
 * notify stuff
 */

static void NotifyUser (struct NotifyRequest *nr, globaldata *g)
{
  struct NotifyMessage *msg;

	if (nr->nr_MsgCount && (nr->nr_Flags & NRF_WAIT_REPLY))
	{
		nr->nr_Flags |= NRF_MAGIC;
		return;
	}

	if (nr->nr_Flags & NRF_SEND_MESSAGE)
	{
		if (!(msg = AllocMemP (sizeof(struct NotifyMessage), g)))
			return;

		msg->nm_ExecMessage.mn_ReplyPort = g->notifyport;
		msg->nm_ExecMessage.mn_Length = sizeof(struct NotifyMessage); 
		msg->nm_Class = NOTIFY_CLASS;
		msg->nm_Code = NOTIFY_CODE;
		msg->nm_NReq = nr;
		PutMsg (nr->nr_stuff.nr_Msg.nr_Port, &msg->nm_ExecMessage);
		nr->nr_MsgCount++;
	}
	else if (nr->nr_Flags & NRF_SEND_SIGNAL)
	{
		Signal (nr->nr_stuff.nr_Signal.nr_Task, 1<<nr->nr_stuff.nr_Signal.nr_SignalNum);
	}
}

/*
 * Checks if there is a notify for the object identified by anodenr
 */
static struct notifyobject *
CheckNotify (ULONG parentanodenr, DSTR objectname, struct notifyobject *no,
			 BOOL checkparent, globaldata *g)
{
	if (!no)
		no = HeadOf(&g->currentvolume->notifylist);
	else
		no = no->next;

	for ( ; no->next; no=no->next)
	{
		/* file match */
		if (no->parentanodenr == parentanodenr)
		{
			if (intlcmp(no->objectname, objectname))
				return no;
		}

		/* parent match */
		if (checkparent && no->anodenr == parentanodenr)
			return no;
	}

	return NULL;
}

/*
 * Notifies change of object & dir object is in
 */
void PFSDoNotify (struct fileinfo *object, BOOL checkparent, globaldata *g)
{
	struct notifyobject *no = NULL;

#if DELDIR
	if ((IPTR)object->direntry <= SPECIAL_DELFILE)
		return;
#endif

	while ((no = CheckNotify (object->dirblock->blk.anodenr,
				(DSTR)&object->direntry->nlength, no, checkparent, g)))
		NotifyUser(no->req, g);
}


/*
 * To be called when a object named filename is created in dir
 * dirnode. This routine checks if there is a partially parsed
 * notify request in that directory that matches the newly
 * created object. If so, the parsing of the notify request
 * can continue.
 */
void PFSUpdateNotify (ULONG dirnode, DSTR filename, ULONG anodenr, globaldata *g)
{
  struct notifyobject *no = NULL;
  UBYTE *temp;

	for (no = HeadOf(&g->currentvolume->notifylist); no->next; no=no->next)
	{
		/* check for parsed path match */
		if (no->parentanodenr == dirnode)
		{
			if (no->unparsed)
			{
				/* check if we can continue parsing */
				temp = strchr(no->unparsed, '/');
				if (temp)
					*temp = 0;
		
				if (intlcdcmp(no->unparsed, filename))
				{
					if (temp)
						no->unparsed = *(temp+1) ? (temp+1) : NULL;
					else
						no->unparsed = NULL;

					no->parentanodenr = anodenr;
				}

				if (temp)
					*temp = '/';
			}
			else
			{
				/* check if new object is a notify object */
				if (intlcmp(no->objectname, filename))
				{
					no->anodenr = anodenr;
				}
			}
		}
	}
}


/**********************
 * AFS extension
 */

#if EXTRAPACKETS
/*
 * Enter MODE_SLEEP
 *
 * All direct directory block references are replaced by an
 * anode reference. The reference can later be restored by
 * searching the object in the directory identified by the
 * diranodenr.
 *
 * Examine chains have to be broken since sleepwalkers are
 * allowed to change directory order
 */
static void Sleep (globaldata *g)
{
	listentry_t *le;
	struct volumedata *volume = g->currentvolume;

	if (volume)
	{
		/* first update disk */
		UpdateDisk (g);

		/* flush references */
		for (le=HeadOf (&volume->fileentries); le->next; le=le->next)
		{
			if (!IsVolumeEntry(le))
			{
				/* load flushed references */
				if (le->dirblocknr)
					LoadDirBlock (le->dirblocknr, g);

				le->diranodenr = le->info.file.dirblock->blk.anodenr;
			}

			if (IsFileEntry(le))
			{
				fileentry_t *fe = (fileentry_t *)le;

				if (fe->anodechain)
					DetachAnodeChain (fe->anodechain, g);

				fe->currnode = NULL;
			}
		}

		/* now kill all flushed references (they are now replaced
		 * by anode references in le->diranodenr)
		 */
		for (le=HeadOf (&volume->fileentries); le->next; le=le->next)
		{
			le->dirblocknr = 0;

			/* terminate examine chains */
			if (le->type.flags.dir && !IsDelDir(le->info))
			{
				((lockentry_t *)le)->nextentry.direntry = NULL;
				((lockentry_t *)le)->nextentry.dirblock = NULL;
				((lockentry_t *)le)->nextdirblocknr = 0;
				((lockentry_t *)le)->nextdirblockoffset = 0;
			}
		}


		/* flush cache. Watch it: FreeUnusedResources also calls
		 * the conventional FlushBlock! 
		 */
		FreeUnusedResources (volume, g);
	}

	/* set sleepmode */
	g->sleepmode = TRUE;

	/* enter sleepmode */
	g->DoCommand = SleepCommands;
}

/*
 * Leave MODE_SLEEP
 *
 * Reload all essential blocks. References are restored by searching
 * by anodenr in the directory identified by diranodenr (using FetchObject)
 */
static void Awake (globaldata *g)
{
  listentry_t *le;
  struct volumedata *volume = g->currentvolume;
  struct rootblock *rootblock;
  SIPTR error;

	if (volume)
	{
		/* reload current rootblock  */
		rootblock = volume->rootblk;
		RawRead((UBYTE *)rootblock, rootblock->rblkcluster, ROOTBLOCK, g);

		/* reload rootblock extension */
		if (rootblock->extension && (rootblock->options & MODE_EXTENSION))
			RawRead((UBYTE *)&volume->rblkextension->blk, volume->rescluster,
				rootblock->extension, g);

		/* reload deldir */
		// if (rootblock->deldir && (rootblock->options & MODE_DELDIR))
		//	RawRead((UBYTE *)&volume->deldir->blk, volume->rescluster,
		//		rootblock->deldir, g);

		/* reconfigure modules */
		InitModules (volume, FALSE, g);

		/* restore references */
		for (le=HeadOf (&volume->fileentries); le->next; le=le->next)
		{
			if (!IsVolumeEntry(le))
			{
				if (!FetchObject(le->diranodenr, le->anodenr, &le->info, g))
					ErrorMsg(AFS_ERROR_UNSLEEP, NULL, g);  /*  -> kill, invalidate lock <- */

				if (IsFileEntry(le))
				{
				  ULONG offset;
				  fileentry_t *fe = (fileentry_t *)le;

					/* restore anodechain and fe->currnode */
					if (!(fe->anodechain = GetAnodeChain(fe->le.anodenr, g)))
						;   /* kill, invalidate */
				
					offset = fe->offset;
					fe->currnode = &fe->anodechain->head;
					fe->offset = fe->blockoffset = fe->anodeoffset = 0;
					if (SeekInObject(fe, offset, OFFSET_BEGINNING, &error, g) == -1)
						ErrorMsg(AFS_ERROR_UNSLEEP, NULL, g);  /* -> kill, invalidate */
				}
			}
		}
	}

	// --> prevent 'normal' reference-restore on loading
	//  directory blocks

	/* unset sleepmode */
	g->sleepmode = FALSE;

	/* awake */
	g->DoCommand = NormalCommands;
}

/*
 * Disk accessing packets are pending --> wake up!
 * Handshaking sleeptask <-> AFS
 */
static void Alarm (globaldata *g)
{
  ULONG sleepmask = 1<<g->alarmsignal;

	/* signal task that we want to wake up */
	Signal (g->sleeptask, sleepmask);

	while (g->sleepmode)
	{
		WaitPort (g->sleepport);
		HandleSleepMsg (g);
	}
}



/*
 * Reflect changed anode in locklist references
 * returns number of references updated
 * (MODE_SLEEP only)
 */
static LONG UpdateAnode (ULONG old, ULONG new, globaldata *g)
{
  listentry_t *le;
  struct notifyobject *no;
  LONG i=0;
  struct volumedata *volume = g->currentvolume;

	/*
	 * Update fileentry list
	 */

	for (le=(listentry_t *)HeadOf(&volume->fileentries); le->next; le=le->next)
	{
		if (!IsVolumeEntry(le))
		{
			if (le->anodenr == old)
			{
				i++;
				if (!new)
					;   /* invalidate le */
				le->anodenr = new;
			}

			if (le->diranodenr == old)
			{
				i++;
				le->diranodenr = new;
			}
		}
	}

	/*
	 * Update notify list
	 */

	for (no=HeadOf(&g->currentvolume->notifylist); no->next; no=no->next)
	{
		if (no->anodenr == old)
		{
			i++;
			no->anodenr = new;
		}
	}

	return i;
}

/*
 * Message handler for messages on the sleepport
 */
void HandleSleepMsg (globaldata *g)
{
  struct Message *msg;

	while ((msg = GetMsg (g->sleepport)))
	{
		if (g->sleepmode)
		{
			g->action = (struct DosPacket *)msg->mn_Node.ln_Name;
			(g->DoCommand)(g->action, g);
		}
		else
		{
			g->action->dp_Res1 = DOSFALSE;
			g->action->dp_Res2 = ERROR_ACTION_NOT_KNOWN;
		}

		ReturnPacket (g->action, g->sleepport, g);
	}
}

#endif
