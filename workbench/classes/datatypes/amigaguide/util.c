/*
** $PROJECT: amigaguide.datatype
**
** $VER: util.c 50.1 (09.09.03)
**
** $AUTHOR: Stefan Ruppert <stefan@ruppert-it.de>
**
*/

/* ------------------------------- includes ------------------------------- */

#include <graphics/gfxbase.h>
#include "classbase.h"

#include <dos/dostags.h>

/* ------------------------- AmigaGuide commands -------------------------- */

static const struct AmigaGuideCmd globalcmds[] =
{
   {"$ver:",     CMD_VERSION},
   {"(c)",       CMD_COPYRIGHT},
   {"amigaguide",CMD_DATABASE},
   {"author",    CMD_AUTHOR},
   {"database",  CMD_DATABASE},
   {"dnode",     CMD_DNODE},
   {"font",      CMD_FONT},
   {"height",    CMD_HEIGHT},
   {"help",      CMD_HELP},
   {"index",     CMD_INDEX},
   {"macro",     CMD_MACRO},
   {"master",    CMD_MASTER},
   {"node",      CMD_NODE},
   {"onclose",   CMD_ONCLOSE},
   {"onopen",    CMD_ONOPEN},
   {"rem",       CMD_REMARK},
   {"remark",    CMD_REMARK},
   {"smartwrap", CMD_SMARTWRAP},
   {"tab",       CMD_TAB},
   {"toc",       CMD_TOC},
   {"width",     CMD_WIDTH},
   {"wordwrap",  CMD_WORDWRAP},
   {"worddelim", CMD_WORDDELIM},
   {NULL,~0}
};


static const struct AmigaGuideCmd nodecmds[] =
{
   {"endnode",   CMD_ENDNODE},
   {"font",      CMD_FONT},
   {"help",      CMD_HELP},
   {"index",     CMD_INDEX},
   {"keywords",  CMD_KEYWORDS},
   {"onclose",   CMD_ONCLOSE},
   {"onopen",    CMD_ONOPEN},
   {"next",      CMD_NEXT},
   {"node",      CMD_NODE},
   {"prev",      CMD_PREV},
   {"smartwrap", CMD_SMARTWRAP},
   {"tab",       CMD_TAB},
   {"title",     CMD_TITLE},
   {"toc",       CMD_TOC},
   {"wordwrap",  CMD_WORDWRAP},
   {NULL,-1}
};

/* ----------------- AmigaGuide command support functions ----------------- */

static
LONG GetCommand(struct ClassBase *cb,
		const struct AmigaGuideCmd *cmds,
		STRPTR cmd, STRPTR *args)
{
   const struct AmigaGuideCmd *cmdptr = cmds;

   while(cmdptr->agc_Id != -1)
   {
      int cmdlen = strlen(cmdptr->agc_Name);
      if(!Strnicmp(cmdptr->agc_Name, cmd, cmdlen))
      {
	 STRPTR ptr = cmd + cmdlen;
	 ptr = eatws(ptr);
	 *args = ptr;
	 return (LONG) cmdptr->agc_Id;
      }
      cmdptr++;
   }

   return -1;
}

LONG GetGlobalCommand(struct ClassBase *cb,
		      STRPTR cmd, STRPTR *args)
{
   return GetCommand(cb, globalcmds, cmd, args);
}

LONG GetNodeCommand(struct ClassBase *cb,
		    STRPTR cmd, STRPTR *args)
{
   return GetCommand(cb, nodecmds, cmd, args);
}

/* ----------------------- memory support functions ----------------------- */

APTR AllocAGMem(Class *cl, Object *obj, ULONG size)
{
   CLASSBASE;
   INSTDATA;
   APTR mem;

   mem = AllocPooled(data->ag_Pool,size);
   return mem;
}
void FreeAGMem(Class *cl, Object *obj, APTR mem, ULONG size)
{
   CLASSBASE;
   INSTDATA;
   FreePooled(data->ag_Pool, mem, size);
}

APTR AllocAGVec(Class *cl, Object *obj, ULONG size)
{
   ULONG *mem;
   size += sizeof(ULONG);
   if((mem = AllocAGMem(cl, obj, size)) != NULL)
      *mem++= size;

   return mem;
}
void FreeAGVec(Class *cl, Object *obj, APTR mem)
{
   ULONG *m = ((ULONG *) mem) - 1;
   FreeAGMem(cl, obj, m, *m);
}

/* ----------------------- string support functions ----------------------- */

STRPTR CopyAGString(Class *cl, Object *obj, STRPTR args)
{
   STRPTR str;
   STRPTR ptr = args;
   STRPTR help = ptr;

   if(*ptr == '"')
      help = ++ptr;

   while(*ptr != '\n' && *ptr != '\0' && *ptr != '"')
      ptr++;

   if((str = AllocAGVec(cl, obj, ptr-help+1)) != NULL)
      strncpy(str, help, ptr-help);

   return str;
}

STRPTR CopyString(Class *cl, Object *obj, STRPTR args)
{
   STRPTR str;
   STRPTR ptr = args;

   while(*ptr != '\n' && *ptr != '\0')
      ptr++;

   if((str = AllocAGVec(cl, obj, ptr-args+1)) != NULL)
      strncpy(str, args, ptr-args);

   return str;
}

/* --------------------------- clipping support --------------------------- */

struct Region *InstallClipRegionSafe(struct ClassBase *cb, struct GadgetInfo *ginfo,
				     struct Region *reg, struct Rectangle *rect)
{
   struct Region *oldreg;
   struct Layer *l = ginfo->gi_Layer;
   BOOL update = FALSE;

   if(l->Flags & LAYERUPDATING)
   {
      EndUpdate(l,FALSE);
      update = TRUE;
   }

   ClearRegion(reg);
   if(rect != NULL)
      OrRectRegion(reg,rect);

   oldreg = InstallClipRegion(l, reg);

   if(update)
   {
      BeginUpdate(l);
   }

   return oldreg;
}
void UnInstallClipRegionSafe(struct ClassBase *cb, struct GadgetInfo *ginfo,
			     struct Region *oldreg)
{
   struct Layer *l = ginfo->gi_Layer;
   BOOL update = FALSE;

   if(l->Flags & LAYERUPDATING)
   {
      EndUpdate(l,FALSE);
      update = TRUE;
   }

   InstallClipRegion(l,oldreg);

   if(update)
      BeginUpdate(l);
}

/* ----------------------------- tags support ----------------------------- */

#ifdef __AROS__
#warning "FIXME: Stupid NotifyAttrs() relying on stack based function param passing, etc."
ULONG NotifyAttrs(Object * o, void * ginfo, ULONG flags, ...)
{
	return DoMethod(o, OM_NOTIFY, (IPTR)(&flags + 1), (IPTR)ginfo, flags);
}
#else
ULONG NotifyAttrs(Object * obj, void * ginfo, ULONG flags, ...)
{
    struct opUpdate notify;
    ULONG rc;
    va_list args;
    va_start(args, flags);

    notify.MethodID = OM_NOTIFY;
    notify.opu_AttrList = (struct TagItem *) args->overflow_arg_area;
    notify.opu_GInfo = ginfo;
    notify.opu_Flags = flags;

    rc = DoMethodA(obj, (Msg) &notify);

    va_end(args);

    return rc;
}
#endif



/* ----------------------- datatype object support ------------------------ */

BOOL GetDTDomain(Class *cl, Object *obj, struct IBox *domain)
{
   struct IBox *tmp;
   struct opGet get;

   get.MethodID = OM_GET;
   get.opg_AttrID  = DTA_Domain;
   get.opg_Storage = (ULONG *) &tmp;

   /* CHECK: get domain from superclass, because we have overloaded
      the DTA_Domain attribute */
   if(DoSuperMethodA(cl, obj, (Msg) &get))
   {
      *domain = *tmp;
      return TRUE;
   }
   return FALSE;
}

/* ---------------------------- node functions ---------------------------- */

struct AmigaGuideNode *GetAGNode(Class *cl, Object *obj,
                                 struct AmigaGuideFile *agf, STRPTR name)
{
   CLASSBASE;
   struct Node *n;

   /* search node list for the given name case insensitive. */
   for(n = agf->agf_Nodes.lh_Head; n->ln_Succ != NULL ; n = n->ln_Succ)
   {
      struct AmigaGuideNode *agn = (struct AmigaGuideNode *) n;
      if(!Stricmp(name, n->ln_Name))
	 return agn;

      if(agn->agn_Keywords != NULL)
      {
         STRPTR ptr = agn->agn_Keywords;
         STRPTR keyword;
         LONG len = strlen(name);
         do
         {
            ptr = eatws(ptr);
            keyword = ptr;
            while(*ptr != '\0' && *ptr != ' ' && *ptr != '\t' && *ptr != ',')
               ++ptr;
            if(ptr-keyword == len && !Strnicmp(name, keyword, len))
               return agn;
            while(*ptr == ',' || *ptr == '\t' || *ptr == '\t')
               ++ptr;
         } while(*ptr != '\0');
      }
   }

   return NULL;
}


ULONG DoTrigger(Class *cl, Object *obj, struct GadgetInfo *ginfo,
		ULONG function, APTR data)
{
   struct dtTrigger trigger;

   trigger.MethodID     = DTM_TRIGGER;
   trigger.dtt_GInfo    = ginfo;
   trigger.dtt_Function = function;
   trigger.dtt_Data     = data;

   return DoMethodA(obj, (Msg) & trigger);
}

ULONG GotoObject(Class *cl, Object *obj, struct GadgetInfo *ginfo, STRPTR name, LONG line)
{
   ULONG rv = 0;
   if(name != NULL)
   {
      INSTDATA;

      struct dtGoto msg;
      struct TagItem attrs[] = { {DTA_TopVert, line}, {TAG_DONE, 0} };

      data->ag_Flags.GotoLine = TRUE;

      msg.MethodID     = DTM_GOTO;
      msg.dtg_GInfo    = ginfo;
      msg.dtg_NodeName = name;
      msg.dtg_AttrList = attrs;

      rv = DoMethodA(obj, (Msg) &msg);
   }
   return rv;
}

ULONG GotoObjectTag(Class *cl, Object *obj, struct GadgetInfo *ginfo, Tag tag)
{
   CLASSBASE;
   INSTDATA;
   STRPTR gobj = NULL;

   /* try to get the object to goto from the actual object */
   GetAttr(tag, data->ag_Actual, (ULONG *) &gobj);

   /* no object found. so try global object. */
   if(gobj == NULL)
      GetAttr(tag, obj, (ULONG *) &gobj);

   if(gobj == NULL)
      return 0;

   /* an object to goto found try to load it. */
   return GotoObject(cl, obj, ginfo, gobj, 0);
}

struct AmigaGuideObject *AllocAGObject(Class *cl, Object *obj)
{
   CLASSBASE;
   struct AmigaGuideObject *agobj;
   agobj = AllocAGMem(cl, obj, sizeof(struct AmigaGuideObject));
   if(agobj == NULL)
      SetIoErr(ERROR_NO_FREE_STORE);
   else
      agobj->ago_Node.ln_Name = "DTObject";
   return agobj;
}

struct AmigaGuideObject *AllocAGObjectNode(Class *cl, Object *obj,
                                           struct AmigaGuideFile *agf,
					   struct AmigaGuideNode *agnode)
{
   CLASSBASE;
   struct AmigaGuideObject *agobj;
   LONG err = 0;

   agobj = AllocAGMem(cl, obj, sizeof(struct AmigaGuideObject));
   if(agobj == NULL)
   {
      err = ERROR_NO_FREE_STORE;
   } else
   {
      agobj->ago_Node.ln_Name = agnode->agn_Node.ln_Name;
      agobj->ago_AGNode = agnode;
      agobj->ago_Buffer = AllocAGMem(cl, obj, agnode->agn_Length);
      agobj->ago_BufferLen = agnode->agn_Length;

      if(agobj->ago_Buffer == NULL)
      {
	 FreeAGMem(cl, obj, agobj, sizeof(struct AmigaGuideObject));
	 err = ERROR_NO_FREE_STORE;
	 agobj = NULL;
      } else
      {
	 Seek(agf->agf_Handle, agnode->agn_Pos, OFFSET_BEGINNING);
	 if(Read(agf->agf_Handle, agobj->ago_Buffer, agnode->agn_Length) != agnode->agn_Length)
	 {
	    FreeAGMem(cl, obj, agobj->ago_Buffer, agobj->ago_BufferLen);
	    FreeAGMem(cl, obj, agobj, sizeof(struct AmigaGuideObject));
	    agobj = NULL;
	 }
      }
   }

   if(err != 0)
      SetIoErr(err);
   return agobj;
}

void FreeAGObject(Class *cl, Object *obj, struct AmigaGuideObject *agobj)
{
   CLASSBASE;

   if(agobj != NULL)
   {
      if(agobj->ago_Object != NULL && !agobj->ago_NoDispose)
      {
         STRPTR name = AllocAGMem(cl, obj, 1024);
         /* if this is a amigaguidenode object delete tempory file */
         if(agobj->ago_AGNode != NULL && name != NULL)
         {
            BPTR handle = NULL;
            if(GetDTAttrs(agobj->ago_Object, DTA_Handle, (ULONG) &handle, TAG_DONE) == 1 &&
               handle != NULL)
            {
               NameFromFH(handle, name, 1024);
            }
         }
         /* nodes are created with NewObject() instead of NewDTObject() */
         if(agobj->ago_AGNode != NULL)
            DisposeObject(agobj->ago_Object);
         else
            DisposeDTObject(agobj->ago_Object);
         if(name != NULL)
         {
            if(*name != '\0')
            {
               DB(("delete file %s\n", name));
               DeleteFile(name);
            }
            FreeAGMem(cl, obj, name, 1024);
         }
      }
      if(agobj->ago_TmpHandle != NULL)
	 UnLock(agobj->ago_TmpHandle);
      if(agobj->ago_Buffer != NULL)
	 FreeAGMem(cl, obj, agobj->ago_Buffer, agobj->ago_BufferLen);
      FreeAGMem(cl, obj, agobj, sizeof(struct AmigaGuideObject));
   }
}

void ParseFontLine(Class *cl, Object *obj, STRPTR args, struct TextAttr *ta)
{
   CLASSBASE;
   STRPTR ptr  = args;
   STRPTR tmp;
   LONG size;
   UBYTE *fontname = ta->ta_Name;
   LONG fontlen;

   tmp = fontname;

   while(*ptr != ' ' && *ptr != '\t' && *ptr != '\n')
      *tmp++ = *ptr++;
   *tmp = '\0';
   fontlen = tmp - fontname;

   if(fontlen > 5 && strcmp(&fontname[fontlen-5],".font"))
      strcat(fontname,".font");
   ptr=eatws(ptr);

   if(*ptr == '\n')
      size = 8;
   else
      StrToLong(ptr,&size);

   ta->ta_Name  = fontname;
   ta->ta_YSize = size;
   ta->ta_Style = FS_NORMAL;
   ta->ta_Flags = 0;
}

/* ------------------------- locate file functions ------------------------- */

static BOOL AbsolutePath(STRPTR path)
{
   if(*path == ':')
      return FALSE;

   while(*path != '\0' && *path != '/')
   {
      if(*path == ':')
	 return TRUE;
      path++;
   }

   return FALSE;
}

/* lock file, also testing if an amigaguide nodename is in the path */
static BPTR MyLock(struct ClassBase *cb,STRPTR file, BOOL *nodetype, BPTR dir)
{
   BPTR fh;
   BPTR olddir = NULL;

   if(dir != NULL)
      olddir = CurrentDir(dir);

   fh = Lock(file,SHARED_LOCK);

   DB(("test : %s\n",file));

   if(fh == NULL && nodetype != NULL)
   {
      STRPTR ptr = PathPart(file);

      if(ptr != file)
      {
	 UBYTE chr = *ptr;
	 *ptr = '\0';

	 DB(("test PathPart() : %s\n",file));

	 if((fh = Lock(file,SHARED_LOCK)) != NULL)
	    *nodetype = TRUE;

	 *ptr = chr;
      }
   }

   if(olddir != NULL)
      if(CurrentDir(olddir) == dir)
         UnLock(dir);
   DB(("lock : %lx\n",fh));
   return fh;
}


static BPTR GetObjectDir(Class *cl, Object *obj)
{
   CLASSBASE;
   struct DataType *dt;
   BPTR handle;
   ULONG type;
   BPTR dir = NULL;

   if(GetDTAttrs(obj,DTA_DataType,  (ULONG) &dt,
		     DTA_SourceType,(ULONG) &type,
		     DTA_Handle,    (ULONG) &handle,
		     TAG_DONE) == 3)
   {
      if(handle != NULL && type == DTST_FILE)
      {
         DB(("handle : %lx, type : %ld\n",handle,type));

         switch(dt->dtn_Header->dth_Flags & DTF_TYPE_MASK)
         {
         case DTF_BINARY:
         case DTF_ASCII:
	    dir = ParentOfFH(handle);
	    break;
         case DTF_IFF:
	    dir = ParentOfFH((BPTR)((struct IFFHandle *) handle)->iff_Stream);
	    break;
         case DTF_MISC:
	    dir = DupLock(handle);
	    break;
         }
      }
   }
   return dir;
}

BPTR GetFileLock(Class *cl, Object *obj,
                 STRPTR file, BOOL *nodetype)
{
   CLASSBASE;
   INSTDATA;
   struct Process *thisproc = (struct Process *) FindTask(NULL);
   BPTR lock = NULL;
   APTR winptr = thisproc->pr_WindowPtr;

   if(file == NULL)
      return NULL;

   /* disable possible unknown assign requester */
   thisproc->pr_WindowPtr = (APTR) -1;

   if(AbsolutePath(file))
   {
      lock = MyLock(cb, file, nodetype, NULL);
   } else
   {
      UBYTE path[128];
      UBYTE language[64];
      BPTR dir;
      GetVar("language", language, sizeof(language), GVF_GLOBAL_ONLY);

      mysprintf(cb, path, sizeof(path), "PROGDIR:Help/%s", language);
      {
         dir = Lock(path, SHARED_LOCK);
         if(dir != NULL)
         {
            lock = MyLock(cb, file, nodetype, dir);
         }

         if(lock == NULL)
         {
            mysprintf(cb, path, sizeof(path), "Help:%s", language);
            dir = Lock(path, SHARED_LOCK);
            if(dir != NULL)
            {
               lock = MyLock(cb, file, nodetype, dir);
            }
         }
      }

      if(lock == NULL)
      {
         /* use current dir */
         lock = MyLock(cb, file, nodetype, NULL);

         if(lock == NULL)
         {
            dir = GetObjectDir(cl, obj);
            if(dir != NULL)
            {
               lock = MyLock(cb, file, nodetype, dir);
            }

            if(lock == NULL)
            {
               if(data->ag_ActualObject->ago_AGNode != NULL)
               {
                  dir = ParentDir(data->ag_ActualObject->ago_AGNode->agn_File->agf_Lock);
                  if(dir != NULL)
                  {
                     lock = MyLock(cb, file, nodetype, dir);
                  }
               }

               if(lock == NULL)
               {
      	          /* amigaguide compatibility!!!*/
      	          if((dir = Lock(":", SHARED_LOCK)) != NULL)
      	          {
      	             lock = MyLock(cb, file, nodetype, dir);
      	          }
               }
   	    }
         }
      }
   }

   DB(("lock : %lx\n",lock));
   thisproc->pr_WindowPtr = winptr;

   return lock;
}

/* ------------------------------- sprintf -------------------------------- */
#ifdef __MORPHOS__
static APTR mysprintf_hook(APTR s, UBYTE chr)
{
   STRPTR buf = (STRPTR) s;
   /* only write character until stop byte */
   if(*buf == 0)
   {
      *buf++ = chr;
      *buf = '\0';
   }
   return buf;
}
LONG mysprintf(struct ClassBase *cb, STRPTR buf, LONG len, STRPTR format,...)
{
   va_list ap;
   va_start(ap, format);
   memset(buf, 0, len);
   buf[len-1] = 127;
   VNewRawDoFmt(format, mysprintf_hook, buf, ap);
   buf[len-1] = 0;
   va_end(ap);
   return strlen(buf);
}
#elif defined(__AROS__)

#include <stdarg.h>
#warning "FIXME: stupid mysprintf() relying on vararg params being passed on stack etc."

struct spf
{
   STRPTR buf;
   STRPTR end;
};

AROS_UFH2S(void, mysprintf_hook,
    AROS_UFHA(UBYTE, chr, D0),
    AROS_UFHA(struct spf *, spf, A3))
{
   AROS_USERFUNC_INIT

   *(spf->buf++) = chr;
   
   AROS_USERFUNC_EXIT
}

LONG mysprintf(struct ClassBase *cb, STRPTR buf, LONG len, STRPTR format,...)
{
   struct spf data;
   va_list ap;

   data.buf = buf;
   data.end = buf+len-1;

   va_start(ap, format);

   RawDoFmt(format,ap,(void (*)()) mysprintf_hook, &data);

   va_end(ap);

   return strlen(buf);
}

#else
struct spf
{
   STRPTR buf;
   STRPTR end;
};
LibCall
static void mysprintf_hook((REG(d0, UBYTE chr), REG(a3, struct spf *spf)))
{
   *(spf->buf++) = chr;
}

LONG mysprintf(struct ClassBase *cb, STRPTR buf, LONG len, STRPTR format,...)
{
   struct spf data;
   va_list ap;

   data.buf = buf;
   data.end = buf+len-1;

   va_start(ap, format);

#ifdef __MORPHOS__
   VNewRawDoFmt(format, mysprintf_hook, buf, ap);
#else
   RawDoFmt(format,ap,(void (*)()) mysprintf_hook, &data);
#endif

   va_end(ap);

   return strlen(buf);
}
#endif


ULONG SendRexxCommand(Class *cl, Object *obj, STRPTR command, ULONG mode)
{
   CLASSBASE;
   INSTDATA;

   struct MsgPort *rxport;
   struct MsgPort *arexxport;
   struct RexxMsg *rxmsg;
   ULONG retval = RC_ERROR;
   BPTR olddir = NULL;
   BPTR dir;

   DB(("rexx cmd: %s\n",command));

   /* just ignore any rexx commands in secure mode */
   if(data->ag_Flags.Secure)
      return RC_OK;

   dir = GetObjectDir(cl, obj);
   if(dir != NULL)
      olddir = CurrentDir(dir);

   if((rxport = data->ag_RexxPort) != NULL)
   {
      Forbid();
      if((arexxport = FindPort(RXSDIR)) != NULL)
      {
	 DB(("replyport : %lx\n",rxport));

	 if(RexxSysBase != NULL)
	    if((rxmsg = CreateRexxMsg(rxport, "rexx" ,rxport->mp_Node.ln_Name)) != NULL)
	    {
	       if(mode == AGRX_RX)
		  rxmsg->rm_Action = RXCOMM | RXFF_RESULT;
	       else
		  rxmsg->rm_Action = RXCOMM | RXFF_STRING;

	       if((rxmsg->rm_Args[0] = (IPTR)CreateArgstring(command, strlen(command))))
	       {
		  if(FindTask(NULL)->tc_Node.ln_Type == NT_PROCESS && mode == AGRX_RX)
		     rxmsg->rm_Stdout = Open("CON:////AmigaGuide ARexx/AUTO/WAIT/CLOSE", MODE_NEWFILE);
		  else
		     rxmsg->rm_Stdout = NULL;

		  data->ag_RexxOutstanding++;
		  PutMsg(arexxport,(struct Message *) rxmsg);

		  retval = RC_OK;
	       } else
		  DeleteRexxMsg(rxmsg);
	    }
      }
      Permit();
   }

   if(dir != NULL)
      UnLock(CurrentDir(olddir));
   return retval;
}

ULONG SystemCommand(Class *cl, Object *obj, STRPTR command)
{
   CLASSBASE;
   struct TagItem tags[] = { {SYS_Output, 0}, {TAG_DONE, 0} };
   ULONG rc = 0;
   BPTR dir = GetObjectDir(cl, obj);
   BPTR olddir = NULL;
   BPTR cos;

   if(dir != NULL)
      olddir = CurrentDir(dir);

   if((cos = Open("CON:////Output/WAIT/CLOSE/AUTO", MODE_NEWFILE)) != NULL)
      tags[0].ti_Data = (ULONG) cos;
   else
      tags[0].ti_Tag = TAG_IGNORE;

   rc = System(command, tags);

   if(cos != NULL)
      Close(cos);

   DB(("System() returned with %ld\n",rc));

   if(dir != NULL)
   {
      CurrentDir(olddir);
      UnLock(dir);
   }

   return rc;
}

BOOL GetFontDimension(Class *cl, Object *obj, STRPTR font, WORD *x, WORD *y)
{
   CLASSBASE;
   struct TextFont *tf;
   UBYTE fontname[128];
   struct TextAttr ta;
   if(font == NULL)
   {
      tf = GfxBase->DefaultFont;
      *x = tf->tf_XSize;
      *y = tf->tf_YSize;
      return TRUE;
   }

   ta.ta_Name = fontname;
   ParseFontLine(cl, obj, font, &ta);

   if((tf = OpenDiskFont(&ta)) != NULL)
   {
      *x = tf->tf_XSize;
      *y = tf->tf_YSize;
      CloseFont(tf);
      return TRUE;
   }

   ta.ta_Name = "topaz";
   ta.ta_YSize = 8;
   ta.ta_Style = FS_NORMAL;
   ta.ta_Flags = 0;

   if((tf = OpenFont(&ta)) != NULL)
   {
      *x = tf->tf_XSize;
      *y = tf->tf_YSize;
      CloseFont(tf);
      return TRUE;
   }

   return FALSE;
}

