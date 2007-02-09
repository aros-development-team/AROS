#include    <clib/alib_protos.h>
#include    <proto/dos.h>
#include    <proto/exec.h>
#include    <dos/dos.h>
#include    <dos/dosextens.h>
#include    <exec/memory.h>
#include	<exec/types.h>
#include    <utility/utility.h>
#include	<intuition/classusr.h>

#include    <string.h>
#include    <stdlib.h>
#include	<stdio.h>

#include	"filesystems.h"


			struct	 FileEntry	 {
               struct	FileEntry	*next;
               char		name[1];
   };

   extern	struct	 Library  *UtilityBase;
   extern	struct	 ExecBase *SysBase;
   extern	struct	 DosLibrary  *DOSBase;

   struct   TagItem        DummyTags[] = { TAG_DONE, };

   /*
   ** allocPath(char*)
   **
   ** input	:  str		full pathname including a filename
   ** return	:  char*    string without the last part of the passed pathname
   **
   ** bugs:	it strips the last part of the passed data, but it won't recognize if it's
   **			really a file
   */

   char *combinePath(APTR pool, char *path, char *file) {
       int l;
       char *out;
       if ((path == NULL) || (file == NULL)) return NULL;
       if (strlen(path) == NULL) return NULL;
       if (strlen(file) == NULL) return NULL;
       
       l = strlen(path) + strlen(file) + 1;
       if (path[strlen(path)-1] != '/') l++;

       if (pool == NULL) out = AllocMem(l, MEMF_CLEAR); else out = AllocPooled(pool, l);
       if (out) {
           strcpy(out, path);
           AddPart(out, file, l);
       }
       return out;
   }

   char *allocPath(APTR pool, char *str) {
       char *s0, *s1, *s;
       int	 l;

       s = NULL;
       s0 = str;

       s1 = PathPart(str);
       if (s1) {
           for (l=0; s0 != s1; s0++,l++);
           
           s = AllocPooled(pool, l+1);
           if (s) strncpy(s, str, l);
       }
       return s;
   }

   void  freeString(APTR pool, char *str) {
       if (str) {
           if (pool == NULL) FreeMem(str, strlen(str) + 1); else FreePooled(pool, str, strlen(str)+1);
       }
   }

/*
   ** allocates memory for a string and copies them to the new buffer
   **
   ** inputs:	   str		source string
   ** return:	   char		pointer to string or NULL
   **
*/

   char *allocString(APTR pool, char *str) {
       char  *b;
       int	 l;

       if (str == NULL) return NULL;

       l = strlen(str);
       
       if (pool == NULL) b = (char*) AllocMem(l+1, MEMF_CLEAR); else b = (char*) AllocPooled(pool, l+1);
       if (b && (l>0)) strncpy (b, str, l);
       return b;
   }

   void InfoRename(APTR pool, char *from, char *to) {

       char	 *frominfo, *toinfo;

       if ((from == NULL) || (to == NULL)) return;

       frominfo = AllocPooled(pool, strlen(from)+6);

       if (frominfo) {
           strncpy (frominfo, from, strlen(from));
           strcat(frominfo,".info");
           toinfo = AllocPooled(pool, strlen(to)+6);

           if (toinfo) {
               strncpy (toinfo, to, strlen(to));
               strcat(toinfo,".info");
               if (Rename(from, to)) Rename(frominfo, toinfo);
               freeString(pool, toinfo);
           }
           freeString(pool, frominfo);
       }
   }

   char  *allocPathFromLock(APTR pool, BPTR lock) {
       char *pathb, *path;

       path = NULL;
       pathb = AllocPooled(pool, PATHBUFFERSIZE);
       if (pathb) {
           if (NameFromLock(lock, pathb, PATHBUFFERSIZE)) {
               path = allocString(pool, pathb);
           }
           FreePooled(pool, pathb, PATHBUFFERSIZE);
       }
       return path;
   }

   ULONG isPathRecursive(APTR pool, char *source, char *destination) {
       BPTR		      srcLock, destLock;
       ULONG		  back;
       char			  *p1, *p2;

       back = PATH_NOINFO;
       srcLock = Lock(source, SHARED_LOCK);
       if (srcLock) {
           destLock = Lock(destination, SHARED_LOCK);
           if (destLock) {
               p1 = allocPathFromLock(pool, srcLock);
               if (p1) {
                   p2 = allocPathFromLock(pool, destLock);
                   if (p2) {
                       if (strstr(p2, p1) == p2) back = PATH_RECURSIVE; else back = PATH_NONRECURSIVE;

                       freeString(pool, p2);
                   }
                   freeString(pool, p1);
               }
               UnLock(destLock);
           }
           UnLock(srcLock);
       }
       return back;
   }

   BOOL FileExists(char *name) {
       BOOL		   info;
       BPTR		   nLock;
       APTR		   win;
       struct   Task		   *t;

       t = FindTask(NULL);
       win = ((struct Process *) t)->pr_WindowPtr;
       ((struct Process *) t)->pr_WindowPtr = (APTR) -1;  	  //disable error requester

       info = FALSE;
       nLock = Lock(name, SHARED_LOCK);
       if (nLock) {
           UnLock(nLock);
           info = TRUE;
       }
       ((struct Process *) t)->pr_WindowPtr = win;  	   //enable error requester
       return info;
   }

   LONG GetFileLength(char *name) {
       LONG		   info = -1;
       BPTR		   in;
       APTR		   win;
       struct   Task		   *t;

       t = FindTask(NULL);
       win = ((struct Process *) t)->pr_WindowPtr;
       ((struct Process *) t)->pr_WindowPtr = (APTR) -1;  	  //disable error requester

       in = Open(name, MODE_OLDFILE);
       if (in) {
           Seek(in, 0, OFFSET_END);
           info = Seek(in, 0, OFFSET_BEGINNING);
           Close(in);
       }
       ((struct Process *) t)->pr_WindowPtr = win;  	   //enable error requester
       return info;
   }

   void DisposeFileInformations(APTR pool, struct FileInfo *fi) {
       if (fi->comment) freeString(pool, fi->comment);
       fi->comment = NULL;
   }

   BOOL GetFileInformations(APTR pool, char *name, struct FileInfo *fi) {
       struct	FileInfoBlock  *FIB;
       LONG    Success2;
       BOOL    info = FALSE;
       BPTR    nLock;

       fi->len = 0;
       fi->comment = NULL;
       fi->protection = 0;
    
       FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
       if (FIB) {
           nLock = Lock(name, ACCESS_READ);
           if (nLock) {
               Success2 = Examine(nLock,FIB);
               if (Success2) {
                   info = TRUE;
                   fi->len = FIB->fib_Size;
                   if (strlen(FIB->fib_Comment) > 0) fi->comment = allocString(pool, FIB->fib_Comment);
                   fi->protection = FIB->fib_Protection;
               }
               UnLock(nLock);
           }
           FreeDosObject (DOS_FIB,(APTR) FIB);
       }
       return info;
   }

   LONG GetFileInfo(char *name) {
       struct	FileInfoBlock  *FIB;
       LONG		   info,Success2;
       BPTR		   nLock;

       info = -1;

       FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
       if (FIB) {
           nLock = Lock(name, ACCESS_READ);
           if (nLock) {
               Success2 = Examine(nLock,FIB);
               if (Success2) {
                   info = 0;
                   if (FIB->fib_DirEntryType>0) info |= FILEINFO_DIR;
                   if ((FIB->fib_Protection & FIBF_DELETE) != 0) info |= FILEINFO_PROTECTED;
                   if ((FIB->fib_Protection & FIBF_WRITE) != 0) info |= FILEINFO_WRITE;
               }
               UnLock(nLock);
           }
           FreeDosObject (DOS_FIB,(APTR) FIB);
       }
       return info;
   }

   LONG GetProtectionInfo(char *name) {
       struct	FileInfoBlock  *FIB;
       LONG		   info,Success2;
       BPTR		   nLock;

       info = 0;

       FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
       if (FIB) {
           nLock = Lock(name, ACCESS_READ);
           if (nLock) {
               Success2 = Examine(nLock,FIB);
               if (Success2) {
                   info = FIB->fib_Protection;
               }
               UnLock(nLock);
           }
           FreeDosObject (DOS_FIB,(APTR) FIB);
       }
       return info;
   }

   char *GetCommentInfo(APTR pool, char *name) {
       struct	FileInfoBlock  *FIB;
       LONG		   Success2;
       BPTR		   nLock;
       char		   *info;

       info = NULL;

       FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
       if (FIB) {
           nLock = Lock(name, ACCESS_READ);
           if (nLock) {
               Success2 = Examine(nLock,FIB);
               if (Success2) {
                   info = allocString(pool, (char*) &FIB->fib_Comment);
               }
               UnLock(nLock);
           }
           FreeDosObject (DOS_FIB,(APTR) FIB);
       }
       return info;
   }

   BOOL  deleteFile(char *file) {
       DeleteFile(file);
       return TRUE;
   }

   BOOL  copyFile(APTR pool, char *file, char *destpath, struct FileInfoBlock *fileinfo) {
       struct  FileInfoBlock  *fib;
       char	 *to;
       LONG	 clen, wlen;
       LONG bufferlen = COPYLEN;
       BOOL	 quit = TRUE;
       BPTR	 in, out;
       BYTE	 *buffer;
       BPTR	 nLock;

       if (fileinfo) {
           if (fileinfo->fib_Size <= COPYLEN) bufferlen = fileinfo->fib_Size;
           if (bufferlen < 128) bufferlen = 128;
       }
       to = combinePath(pool, destpath, FilePart(file));
       if (to) {
           buffer = AllocPooled(pool, bufferlen);
           if (buffer) {
               in = Open(file, MODE_OLDFILE);
               if (in) {
                   out = Open(to, MODE_NEWFILE);
                   if (out) {
                       fib = fileinfo;
                       if (fileinfo == NULL) {
                           fib = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
                           if (fib) {
                               nLock = Lock(file, ACCESS_READ);
                               if (nLock) {
                                   Examine(nLock,fib);
                                   UnLock(nLock);
                               }
                           }
                       }
                       do {
                           clen = Read(in, buffer, bufferlen);
                           if ((clen !=0) && (clen != -1)) {
                               wlen = Write(out, buffer,clen);
                               if (clen != wlen) clen = 0;
                           }
                       } while ((clen !=0) && (clen != -1));
                       quit = FALSE;
                       Close(out);
                       if (fib) {
                           SetComment(to, fib->fib_Comment);
                           SetProtection(to, fib->fib_Protection);
                           if (fileinfo == NULL) FreeDosObject (DOS_FIB,(APTR) fib);
                       }
                   }
                   Close(in);
               }
               FreePooled(pool, buffer, bufferlen);
           }
           freeString(pool, to);
       }

       return quit;
   }

   BOOL  actionDir(APTR pool, ULONG flags, char *source, char *dest, BOOL quit, UWORD delmode, UWORD protectmode, UWORD overwritemode, struct Hook *dHook, struct Hook *delHook, APTR userdata) {
       struct	FileInfoBlock  *FIB, *FIB2;
       struct	dCopyStruct	   display;
       struct	FileEntry	   *fe, *fef, *fel;

       BPTR		   NewLock, cDir, nDir, nLock;
       WORD		   dmode, pmode, omode, dm, pm, om;
       ULONG		   Success, Success1, Success2, DosError, len;
       char		   *dname, *comment, *dpath;
       BOOL		   del, created, unprotect, failure;
       BOOL		   stop, overwrite;
       LONG		   info, prot;

       if (quit) return TRUE;

       display.userdata = userdata;
   
       dmode = delmode;
       omode = overwritemode;
       pmode = protectmode;

       fef = NULL;
       fel = NULL;
       
       FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
       if (FIB) {
           FIB2 = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
           if (FIB2) {

               NewLock = Lock(source ,ACCESS_READ);
               if (NewLock) {
                   cDir = CurrentDir(NewLock);

                   Success1=Examine(NewLock,FIB);
                   if (Success1) {
                       stop = quit;
                       do {
                           Success=ExNext(NewLock,FIB);

                           if ((flags & (ACTION_DELETE | ACTION_COPY)) == ACTION_DELETE) {
                               if (dmode == DELMODE_NONE) Success = FALSE;
                           }

                           if (Success && Success1) {
                               if (FIB->fib_DirEntryType>0) {
                                   
                                   dname = NULL;

                                   if (((flags & ACTION_COPY) != 0) && dest) {
                                       dname = combinePath(pool, dest, FIB->fib_FileName);
                                       if (dname) {
                                           created = FALSE;
                                           nLock = Lock(dname, ACCESS_READ);
                                           if (nLock) {
                                               Success2 = Examine(nLock,FIB2);
                                               if (Success2) if (FIB2->fib_DirEntryType>0) created = TRUE;
                                               UnLock(nLock);
                                           }
                                           if (!created) {
                                               nDir = CreateDir(dname);
                                               if (nDir) {
                                                   prot = GetProtectionInfo(FIB->fib_FileName);
                                                   comment = GetCommentInfo(pool, FIB->fib_FileName);
                                                   if (comment) SetComment(dname, comment);
                                                   SetProtection(dname, prot);
                                                   freeString(pool, comment);

                                                   created = TRUE;
                                                   UnLock(nDir);
                                               }
                                           }
                                       }
                                   }
                                   unprotect = FALSE;
                                   del = FALSE;

                                   if (delHook && (dmode != DELMODE_NONE) && ((flags & ACTION_DELETE) != 0)) {
                                       if ((dmode == DELMODE_ASK) || (dmode == DELMODE_DELETE) || (dmode == DELMODE_ALL) || (dmode == DELMODE_NO)) {
                                           display.spath = FIB->fib_FileName;
                                           display.file = NULL;
                                           display.type = 0;
                                           display.filelen = FIB->fib_Size;
                                           if (dmode != DELMODE_ALL) dmode = CallHook(delHook, (Object *) &display, NULL);
                                           if ((dmode == DELMODE_ALL) || (dmode == DELMODE_DELETE)) {

                                               unprotect = FALSE;
                                               info = GetFileInfo(FIB->fib_FileName);
                                               if ((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) != 0) {
                                                   if (pmode != DELMODE_NONE) {
                                                       if ((pmode == DELMODE_ASK) || (pmode == DELMODE_DELETE) || (pmode == DELMODE_ALL)  || (pmode == DELMODE_NO)) {
                                                           display.spath = FIB->fib_FileName;
                                                           display.file = NULL;
                                                           display.type = 1;
                                                           display.filelen = 0;
                                                           if (pmode != DELMODE_ALL) pmode = CallHook(delHook, (Object *) &display, NULL);
                                                           if ((pmode == DELMODE_ALL) || (pmode == DELMODE_DELETE)) {
                                                               SetProtection(FIB->fib_FileName, 0);
                                                               unprotect = TRUE;
                                                           }
                                                       }
                                                   }
                                               } else unprotect = TRUE;
                                               if (unprotect) {
                                                   del = TRUE;
                                               }
                                           }
                                       }
                                   }

                                   dm = dmode;
                                   om = omode;
                                   pm = pmode;

                                   if (om == DELMODE_NO) om = DELMODE_NONE;
                                   if (om == DELMODE_DELETE) om = DELMODE_ALL;

                                   if (pm == DELMODE_NO) pm = DELMODE_NONE;
                                   if (pm == DELMODE_DELETE) pm = DELMODE_ALL;

                                   if (dm == DELMODE_NO) dm = DELMODE_NONE;
                                   if (dm == DELMODE_DELETE) dm = DELMODE_ALL;

                                   if (created || ((flags & ACTION_DELETE) !=0)) {
                                       if (((dmode == DELMODE_NO) || (dmode == DELMODE_NONE)) && (flags == ACTION_DELETE)) {
                                           quit = FALSE;
                                       } else {
                                           quit = actionDir(pool, flags, FIB->fib_FileName, dname, quit, dm, pm, om, dHook, delHook, userdata);
                                       }
                                   }

                                   if (!quit && del && unprotect) {
                                       if (FIB->fib_FileName) {
                                           len = strlen(FIB->fib_FileName);
                                           if (len>0) {
                                               
                                               fe = AllocPooled(pool, sizeof(struct FileEntry) + len);
                                               if (fe) {
                                                   strcpy(fe->name, FIB->fib_FileName);
                                                   if (fel) {
                                                       fel->next = fe;
                                                       fel = fe;
                                                   } else {
                                                       fef = fe;
                                                       fel = fe;
                                                   }
                                               }
                                           }
                                       }
                                   }




                                   if (dname) freeString(pool, dname);
                               } else {
                                   if (dHook) {
                                       display.file = FIB->fib_FileName;
                                       display.filelen = FIB->fib_Size;
                                       display.spath = source;
                                       display.dpath = dest;
                                       display.flags = flags;
                                       quit = CallHook(dHook, (Object *) &display, NULL);
                                   }

                                   overwrite = TRUE;

                                   if (((flags & ACTION_COPY) != 0) && dest){
                                       dpath = combinePath(pool, dest, FIB->fib_FileName);
                                       if (dpath) {
                                           info = GetFileInfo(dpath);
                                           if (info != -1) {
                                               overwrite = FALSE;
                                               if (delHook && (omode != DELMODE_NONE)) {
                                                   if ((omode == DELMODE_ASK) || (omode == DELMODE_DELETE) || (omode == DELMODE_ALL) || (omode == DELMODE_NO)) {
                                                       display.spath = dest;
                                                       display.file = FIB->fib_FileName;
                                                       display.type = 2;
                                                       display.filelen = 0;
                                                       if (omode != DELMODE_ALL) omode = CallHook(delHook, (Object *) &display, NULL);
                                                       if ((omode == DELMODE_ALL) || (omode == DELMODE_DELETE)) {
                                                           if ((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) !=0) {
                                                               if (pmode != DELMODE_NONE) {
                                                                   if ((pmode == DELMODE_ASK) || (pmode == DELMODE_DELETE) || (pmode == DELMODE_ALL)  || (pmode == DELMODE_NO)) {
                                                                       display.spath = dest;
                                                                       display.file = FIB->fib_FileName;
                                                                       display.type = 1;
                                                                       display.filelen = 0;
                                                                       if (pmode != DELMODE_ALL) pmode = CallHook(delHook, (Object *) &display, NULL);
                                                                       if ((pmode == DELMODE_ALL) || (pmode == DELMODE_DELETE)) {
                                                                           overwrite = TRUE;
                                                                           SetProtection(dpath, 0);
                                                                       }
                                                                   }
                                                               }
                                                           } else overwrite = TRUE;
                                                       }
                                                   }
                                               }
                                           }
                                       }
                                       if (dpath) freeString(pool, dpath);
                                   }

                                   failure = FALSE;
                                   if (!quit && ((flags & ACTION_COPY) !=0) && overwrite)  failure = copyFile(pool, FIB->fib_FileName, dest, FIB);

                                   if (failure && !quit) {
                                       if (delHook) {
                                           display.spath = source;
                                           display.file = FIB->fib_FileName;
                                           display.type = 3;
                                           display.filelen = 0;
                                           if (CallHook(delHook, (Object *) &display, NULL) == ACCESS_SKIP) quit = FALSE; else quit = TRUE;
                                       } else quit = FALSE;
                                   }

                                   if (!quit && delHook && (dmode != DELMODE_NONE) && ((flags & ACTION_DELETE) !=0)) {
                                       if ((dmode == DELMODE_ASK) || (dmode == DELMODE_DELETE) || (dmode == DELMODE_ALL) || (dmode == DELMODE_NO)) {
                                           display.spath = source;
                                           display.file = FIB->fib_FileName;
                                           display.type = 0;
                                           display.filelen = FIB->fib_Size;
                                           if (dmode != DELMODE_ALL) dmode = CallHook(delHook, (Object *) &display, NULL);
                                           if ((dmode == DELMODE_ALL) || (dmode == DELMODE_DELETE)) {

                                               info = GetFileInfo(FIB->fib_FileName);
                                               unprotect = FALSE;
                                               if ((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) != 0) {
                                                   if (pmode != DELMODE_NONE) {
                                                       if ((pmode == DELMODE_ASK) || (pmode == DELMODE_DELETE) || (pmode == DELMODE_ALL)  || (pmode == DELMODE_NO)) {
                                                           display.spath = source;
                                                           display.file = FIB->fib_FileName;
                                                           display.type = 1;
                                                           display.filelen = 0;
                                                           if (pmode != DELMODE_ALL) pmode = CallHook(delHook, (Object *) &display, NULL);
                                                           if ((pmode == DELMODE_ALL) || (pmode == DELMODE_DELETE)) {
                                                               unprotect = TRUE;
                                                               SetProtection(FIB->fib_FileName, 0);
                                                           }
                                                       }
                                                   }
                                               } else unprotect = TRUE;
                                               if (unprotect) {
                                                   if (FIB->fib_FileName) {
                                                       len = strlen(FIB->fib_FileName);
                                                       if (len>0) {
                                                           
                                                           fe = AllocPooled(pool, sizeof(struct FileEntry) + len);
                                                           if (fe) {
                                                               strcpy(fe->name, FIB->fib_FileName);
                                                               if (fel) {
                                                                   fel->next = fe;
                                                                   fel = fe;
                                                               } else {
                                                                   fef = fe;
                                                                   fel = fe;
                                                               }
                                                           }
                                                       }
                                                   }
                                               }
                                           }
                                       }
                                   }

                               }
                           } else {
                               DosError=IoErr();
                               if (DosError!=ERROR_NO_MORE_ENTRIES) Success=TRUE;
                           }
                       } while (Success && !quit);
                   }

                   while (fef) {
                       len = strlen(fef->name);
                       if (len > 0) {
                           deleteFile(fef->name);
                       }
                       fe = fef->next;
                       FreePooled(pool, fef, sizeof(struct FileEntry) + len);
                       fef = fe;
                   }

                   CurrentDir(cDir);
                   UnLock(NewLock);
               }
               FreeDosObject (DOS_FIB,(APTR) FIB2);
           }
           FreeDosObject (DOS_FIB,(APTR) FIB);
       }

       return quit;
   }

   BOOL CopyContent(APTR p, char *s, char *d, BOOL makeparentdir, ULONG flags, struct Hook *displayHook, struct Hook *delHook, APTR userdata) {

       struct	FileInfoBlock  *FIB;
       struct	dCopyStruct	   display;
       char		   *destname, *dest, *path, *comment, *dpath, *infoname, *destinfo;
       LONG		   len, Success2, prot;
       BPTR		   nLock, nDir;
       APTR        pool;
       BOOL		   created = FALSE;
       BOOL		   dir = TRUE;
       BOOL		   back = FALSE;
       BOOL		   deletesrc, unprotectsrc;
       LONG		   info;
       UWORD		   dmode = DELMODE_ASK;
       UWORD		   pmode = DELMODE_ASK;
       UWORD		   omode = DELMODE_ASK;

       if (p == NULL) {
           pool = CreatePool(MEMF_CLEAR|MEMF_ANY, POOLSIZE, POOLSIZE);
       } else pool = p;

       if (pool == NULL) return FALSE;

       infoname = AllocPooled(pool, strlen(s)+6);
       display.userdata = userdata;
       
       if (infoname) {
           strncpy (infoname, s, strlen(s));
           strcat(infoname,".info");
       }

       if (d) destinfo = AllocPooled(pool, strlen(d)+6); else destinfo = NULL;

       if (destinfo) {
           strncpy (destinfo, d, strlen(d));
           strcat(destinfo,".info");
       }
       
       destname = FilePart(s);

       info = GetFileInfo(s);

       if (info == -1) {
           freeString(pool, infoname);
           freeString(pool, destinfo);
           if (p == NULL) DeletePool(pool);
           return TRUE;
       }

       if ((info & FILEINFO_DIR) != 0) dir = TRUE; else dir = FALSE;
   
       dest = NULL;

       if ((flags & ACTION_COPY) !=0 ) dest = allocString(pool, d);

       if (makeparentdir && dir && dest) {
           if (destname) {
               if (strlen(destname)>0) {
                   freeString(pool, dest);
                   dest = NULL;
                   FIB = (struct FileInfoBlock*) AllocDosObject(DOS_FIB,DummyTags);
                   if (FIB) {
                       dest = combinePath(pool, d, destname);
                       if (dest) {
                           nLock = Lock(dest, ACCESS_READ);
                           if (nLock) {
                               Success2 = Examine(nLock,FIB);
                               if (Success2) if (FIB->fib_DirEntryType>0) created = TRUE;
                               UnLock(nLock);
                           }
                           if (!created) {
                               nDir = CreateDir(dest);
                               if (nDir) {
                                   created = TRUE;
                                   UnLock(nDir);
                                   prot = GetProtectionInfo(s);
                                   comment = GetCommentInfo(pool, s);
                                   if (comment) SetComment(dest, comment);
                                   SetProtection(dest, prot);
                                   freeString(pool, comment);
                               }
                           }
                           if (!created) {
                               freeString(pool, dest);
                               dest = NULL;
                               created = FALSE;
                           }
                       }
                       FreeDosObject (DOS_FIB,(APTR) FIB);
                   }
               }
           }
       }

       path = NULL;

       deletesrc = FALSE;
       unprotectsrc = TRUE;
       if (delHook && ((flags & ACTION_DELETE) != 0) && ((makeparentdir && dir) || !dir)) {
           if (dir) {
               display.spath = s;
               display.file = NULL;
           } else {
               path = allocPath(pool, s);
               display.spath = path;
               display.file = FilePart(s);
           }
           display.type = 0;

           dmode = CallHook(delHook, (Object *) &display, NULL);
           if ((dmode == DELMODE_ALL) || (dmode == DELMODE_DELETE)) {
               deletesrc = TRUE;
               if ((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) != 0) {
                   display.type = 1;
                   unprotectsrc = FALSE;
                   pmode = CallHook(delHook, (Object *) &display, NULL);
                   if ((pmode == DELMODE_ALL) || (pmode == DELMODE_DELETE)) {
                       SetProtection(s, 0);
                       if (infoname) SetProtection(infoname, 0);
                       unprotectsrc = TRUE;
                   }
               }
           }
       }

       if (dest) {
           if (delHook && !dir) {
               dpath = combinePath(pool, d, FilePart(s));
               if (dpath) {
                   info = GetFileInfo(dpath);
                   if (info != -1) {
                       if (delHook && (omode != DELMODE_NONE)) {
                           if ((omode == DELMODE_ASK) || (omode == DELMODE_DELETE) || (omode == DELMODE_ALL) || (omode == DELMODE_NO)) {
                               display.spath = d;
                               display.file = FilePart(s);
                               display.type = 2;
                               if (omode != DELMODE_ALL) omode = CallHook(delHook, (Object *) &display, NULL);
                               if ((omode == DELMODE_ALL) || (omode == DELMODE_DELETE)) {

                                   if (((info & (FILEINFO_PROTECTED|FILEINFO_WRITE)) != 0) && (pmode != DELMODE_NONE)) {
                                       if ((pmode == DELMODE_ASK) || (pmode == DELMODE_DELETE) || (pmode == DELMODE_ALL)  || (pmode == DELMODE_NO)) {
                                           display.spath = d;
                                           display.file = FilePart(s);
                                           display.type = 1;
                                           if (pmode != DELMODE_ALL) pmode = CallHook(delHook, (Object *) &display, NULL);
                                           if ((pmode == DELMODE_ALL) || (pmode == DELMODE_DELETE)) {
                                               SetProtection(dpath, 0);
                                           }
                                       }
                                   }
                               }
                           }
                       }
                   }
               }
               freeString(pool, dpath);
           }
           freeString(pool, path);
       }

       if (dir) {
           if (dest || ((flags & ACTION_DELETE) != 0)) {
               if (((dmode == DELMODE_NONE) || (dmode == DELMODE_NO)) && (flags & (ACTION_DELETE|ACTION_COPY)) == ACTION_DELETE) back = FALSE; else back = actionDir(pool, flags, s, dest, FALSE, dmode, pmode, omode, displayHook, delHook, userdata);
           } else back = TRUE;
       } else {
           if (flags == ACTION_DELETE) back = FALSE; else back = copyFile(pool, s, d, NULL);
       }
 
       if (!back && destinfo && infoname) {
           SetProtection(destinfo, 0);
           copyFile(pool, infoname, d, NULL);
       }

       if (!back && delHook && (dmode != DELMODE_NONE) && ((flags & ACTION_DELETE) !=0)) {
           if (unprotectsrc && deletesrc) {
               deleteFile(s);
               if (infoname) deleteFile(infoname);
           }
       }
       freeString(pool, infoname);
       freeString(pool, destinfo);
       freeString(pool, dest);

       if (p == NULL) DeletePool(pool);
       return !back;
   }
