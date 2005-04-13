/*****************************************************************
 ** FixFonts                                                    **
 **                                                             **
 ** A recompile of the CBM fixfonts program                     **
 **                                                             **
 ** Version 40.2 vom 16.01.1999 © THOR-Software, Thomas Richter **
 ** Changes to compile with AROS - Henning Kiel, hkiel@aros.org **
 *****************************************************************/

#include <exec/types.h>
#include <exec/memory.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <dos/exall.h>

#include <diskfont/diskfont.h>

#include <aros/macros.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/diskfont.h>

#include <string.h>

#define MAXFONTLEN 25L


char version[] = "$VER: FixFonts 40.2 (16.9.99) (c) THOR";

int main()
{
    struct ExAllControl       *exall;
    struct ExAllData          *ead;
    struct DevProc            *devproc=NULL;
    BPTR                       lock,
                               oldlock;
    BPTR                       file;
    struct FontContentsHeader *fhead;
    LONG                       size;
    ULONG                      fontflags;
    int                        len;
    long                       rc = 20;
    int                        more;
    char                       fullname[32];
    char                      *buffer;
    ULONG                      bufsize = 4096L;

    if ((buffer=AllocMem(bufsize,MEMF_PUBLIC)))
    {
        if ((exall=AllocDosObject(DOS_EXALLCONTROL,TAG_DONE)))
        {
            /* Set return code to fine */
            rc=0;
            
            do
            {
                if ((devproc=GetDeviceProc("FONTS:",devproc)))
                {
                    /* User abort? */
                    if (CheckSignal(SIGBREAKF_CTRL_C)) {
                    rc=ERROR_BREAK;
                    break;
                }
                
                exall->eac_LastKey=0L;
                if ((lock=DupLock(devproc->dvp_Lock)))
                {
                    oldlock=CurrentDir(lock);
                    do
                    {
                        ead=(struct ExAllData *)buffer;
                        more=ExAll(lock,ead,bufsize,ED_TYPE,exall);
                        if (!more)
                        {
                            size=IoErr();
                            if (size!=ERROR_NO_MORE_ENTRIES) rc=size; /* break on an error */
                        }
                        else 
                        {
                            /* if there are more entries and an error, abort */
                            if (rc) ExAllEnd(lock,ead,bufsize,ED_TYPE,exall);
                        }
                        
                        /* quit loop on error */
                        if (rc) break;
                        
                        if (exall->eac_Entries==0) continue; /* Ignore empty entries */
                        
                        do
                        {
                            if (ead->ed_Type>0)
                            {
                                len=strlen(ead->ed_Name);
                                if
                                (
                                len <= MAXFONTLEN 
                                && strncmp(ead->ed_Name, "TrueType", len) != 0
                                )
                                {
                                    /* check for user abort */
                                    if (CheckSignal(SIGBREAKF_CTRL_C))
                                    {
                                        rc=ERROR_BREAK;
                                        break;
                                    }
                                    /* Build name of the directory */
                                    strcpy(fullname,ead->ed_Name);
                                    strcpy(fullname+len,".font"); /* Add .font */
                                    
                                    /* Build a contents header */
                                    fhead=NewFontContents(lock,fullname);
                                    if (fhead)
                                    {
                                    if (fhead->fch_NumEntries>0)
                                    {
                                    #ifdef __AROS__
                                    #if !AROS_BIG_ENDIAN
                                    WORD i;
                                    
                                    for(i = 0; i < fhead->fch_NumEntries; i++)
                                    {
                                        if(fhead->fch_FileID == FCH_ID)
                                        {
                                            struct FontContents *fc;
                                            
                                            fc = (struct FontContents *)(fhead + 1);
                                            fc += i;
                                            
                                            fc->fc_YSize = AROS_WORD2BE(fc->fc_YSize);
                                        }
                                        else if
                                        (
                                               (fhead->fch_FileID == TFCH_ID) 
                                            || (fhead->fch_FileID == OFCH_ID)
                                        )
                                        {
                                            struct TFontContents *tfc;
                                            
                                            tfc = (struct TFontContents *)(fhead + 1);
                                            tfc += i;
                                            
                                            if (tfc->tfc_TagCount)
                                            {
                                                ULONG *tags;
                                                WORD t;
                                                
                                                tags = (ULONG *)(&tfc->tfc_FileName[MAXFONTPATH-(tfc->tfc_TagCount*8)]);
                                                for (t = 0; t < tfc->tfc_TagCount * 2 - 1; t++)
                                                {
                                                    tags[t] = AROS_LONG2BE(tags[t]);
                                                }
                                            }
                                            tfc->tfc_TagCount = AROS_WORD2BE(tfc->tfc_TagCount);
                                            tfc->tfc_YSize = AROS_WORD2BE(tfc->tfc_YSize);
                                        }
                                        
                                    } /* for(i = 0; i < fhead->fch_NumEntries; i++) */
                                    
                                    fhead->fch_FileID = AROS_WORD2BE(fhead->fch_FileID);
                                    fhead->fch_NumEntries = AROS_WORD2BE(fhead->fch_NumEntries);
                                    
                                    #endif
                                    #endif
                                    file=Open(fullname,MODE_NEWFILE);
                                    if (file)
                                    {
                                        /* Find the size of the header. This is definitely a hack... */
                                        size=*((LONG *)((UBYTE *)fhead-AROS_ALIGN(sizeof(ULONG))))-sizeof(LONG); /* Ehem. This is AllocVec'd */
                                        rc=0;
                                        if (Write(file,fhead,size)<0) rc=IoErr();
                                        Close(file);
                                        /* Delete the file on failure, clear executeable otherwise */
                                        if (rc) DeleteFile(fullname);
                                        else    SetProtection(fullname,FIBF_EXECUTE);
                                    }
                                    else
                                    {
                                        rc=IoErr();
                                    }
                                    }
                                    else
                                    {
                                        DeleteFile(fullname); /* Remove empty fonts */
                                    }
                                    DisposeFontContents(fhead);          /* remove the fcheader */
                                    }
                                    else
                                    {   
                                        /* FontContents allocation */
                                        size=IoErr();
                                        if (size==0) rc=ERROR_NO_FREE_STORE;
                                        else         rc=size;
                                    }
                                } /* if name small enough */
                            } /* if type is directory */
                            
                            if (rc) break;
                            /* Abort on failure */
                            
                            ead=ead->ed_Next;
                        } while(ead); /* ExAll buffer loop */
                    } while(more); /* while more entries in exall-loop */
                    
                    /* restore the old directory */
                    UnLock(lock);
                    CurrentDir(oldlock);
                }
                else
                {
                    rc=IoErr();
                } /* If DupLock() */
                
                /* No more entries? */
                if ((devproc->dvp_Flags & DVPF_ASSIGN)==0) break;
                
                }
                else
                {             /* GetDeviceProc worked */
                    rc=IoErr();
                    if (rc==ERROR_NO_MORE_ENTRIES) rc=0; /* Default fault is fine */
                    break;          /* loop abort */
                }
            } while(rc==0); /* GetDeviceProc loop */
            
            FreeDeviceProc(devproc);
            FreeDosObject(DOS_EXALLCONTROL,exall);
        }
        else
        {
            rc=ERROR_NO_FREE_STORE;
        }
        
        /* Still no error? Rewrite font cache! */
        if (rc==0)
        {
            fontflags=AFF_DISK;
            while (rc==0)
            {
                for (;;)
                {
                    size=AvailFonts((STRPTR)buffer,bufsize,fontflags);
                    if (size>0)
                    {
                        FreeMem(buffer,bufsize);
                        bufsize+=size;
                        buffer=AllocMem(bufsize,MEMF_PUBLIC);
                        if (buffer==NULL)
                        {
                            rc=ERROR_NO_FREE_STORE;
                            break;
                        }
                    }
                    else if (size==0)
                    {
                        break;
                    }
                    else
                    {
                        rc=IoErr();
                        break;
                    } /* size switch */
                } /* buffer enlarge loop */
                
                if (fontflags & AFF_TAGGED) break;
                
                fontflags |= AFF_TAGGED;        /* and now for the tagged stuff */
            }
        } /* FontCache rewrite */
        
        FreeMem(buffer,bufsize);
    } 
    else
    {
        rc=ERROR_NO_FREE_STORE;     /* buffer allocation */
    }
    
    if (rc)
    {
        if (rc>64)
        {
            PrintFault(rc,"FixFonts failed");
            
            if (rc==ERROR_BREAK) rc=0;
            else                 rc=10;
        }
    }

    return rc;
}
