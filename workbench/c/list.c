/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: List the contents of a directory
    Lang: english
*/
#include <clib/macros.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

static const char version[] = "$VER: list 41.2 (2.10.1997)\n";



int printfiledata(STRPTR filename, BOOL dir, struct DateStamp *ds, ULONG protection, ULONG size)
{
    int error = RETURN_OK;
    IPTR argv[5];
    UBYTE date[LEN_DATSTRING];
    UBYTE time[LEN_DATSTRING];
    struct DateTime dt;
    char flags[8];
    int i;

    CopyMem(ds, &dt.dat_Stamp, sizeof(struct DateStamp));
    dt.dat_Format  = FORMAT_DOS;
    dt.dat_Flags   = DTF_SUBST;
    dt.dat_StrDay  = NULL;
    dt.dat_StrDate = date;
    dt.dat_StrTime = time;
    DateToStr(&dt); /* returns 0 if invalid */

    for(i=0; i<7; i++)
        if (!(protection & (1<<i)))
            flags[6-i]="sparwed"[6-i];
        else
            flags[6-i]='-';
    flags[7] = 0x00;

    argv[0] = (IPTR)filename;
    if (dir) {
        argv[1] = (IPTR)flags;
        argv[2] = (IPTR)date;
        argv[3] = (IPTR)time;
        if (VPrintf("%-25.s   <Dir> %7.s %-11.s %s\n", argv) < 0)
            error = RETURN_ERROR;
    } else {
        argv[1] = size;
        argv[2] = (IPTR)flags;
        argv[3] = (IPTR)date;
        argv[4] = (IPTR)time;
        if (VPrintf("%-25.s %7.ld %7.s %-11.s %s\n", argv) < 0)
            error = RETURN_ERROR;
    }

    return error;
}


int main (int argc, char ** arvg)
{
    char *args[1]={ 0 };
    struct RDArgs *rda;
    LONG error = RETURN_OK;

    rda = ReadArgs("DIR", (IPTR *)args, NULL);
    if (rda) {
        BPTR dir;

	dir = Lock(args[0] != NULL ? args[0] : "", SHARED_LOCK);
	if (dir) {
            struct FileInfoBlock *fib;

            fib = AllocDosObject(DOS_FIB, NULL);
            if (fib) {
                if (Examine(dir, fib)) {
                    if (fib->fib_DirEntryType >= 0) {
                        /* the lock is a directory */
                        LONG files=0, dirs=0, blocks=0;
                        struct ExAllControl *eac;
                        static UBYTE buffer[4096];

                        eac = AllocDosObject(DOS_EXALLCONTROL,NULL);
                        if (eac) {
                            BOOL loop;
                            eac->eac_LastKey = 0;
                            do {
                                loop = ExAll(dir, (struct ExAllData *)buffer, sizeof(buffer), ED_COMMENT, eac);
                                if ((!loop) && (IoErr() != ERROR_NO_MORE_ENTRIES))
                                    /* an error occured */
                                    error = RETURN_ERROR;
                                else {
                                    if (eac->eac_Entries) {
                                        struct ExAllData *ead;
                                        ead = (struct ExAllData *)buffer;
                                        do {
                                            int tmperror;
                                            struct DateStamp ds = {
                                                ead->ed_Days,
                                                ead->ed_Mins,
                                                ead->ed_Ticks
                                            };
                                            tmperror = printfiledata(ead->ed_Name, ead->ed_Type>=0?TRUE:FALSE, &ds, ead->ed_Prot, ead->ed_Size);
                                            error = MAX(error,tmperror);
                                            if (ead->ed_Type >= 0)
                                                dirs++;
                                            else {
                                                files++;
                                                blocks += ead->ed_Size;
                                            }
                                            ead = ead->ed_Next;
                                        } while (ead != NULL);
                                    }
                                }
                            } while ((loop) && (!error));
                            if (!error) {
                                /* print directory summary */
                                IPTR argv[3];
                                argv[0] = files;
                                argv[1] = dirs;
                                argv[2] = blocks;
                                if (VPrintf("%ld files - %ld directories - %ld bytes used\n", argv) < 0)
                                    error = RETURN_ERROR;
                            }
                            FreeDosObject(DOS_EXALLCONTROL, eac);
                        } else {
                            SetIoErr(ERROR_NO_FREE_STORE);
                            error = RETURN_FAIL;
                        }
                    } else {
                      /* the lock is a file */
                      error = printfiledata(fib->fib_FileName, fib->fib_DirEntryType>=0?TRUE:FALSE, &fib->fib_Date, fib->fib_Protection, fib->fib_Size);
                    }
                } else
                    error = RETURN_FAIL;
                FreeDosObject(DOS_FIB, fib);
            } else {
                SetIoErr(ERROR_NO_FREE_STORE);
                error = RETURN_FAIL;
            }

	    UnLock(dir);
	} else
	    error = RETURN_FAIL;

	FreeArgs(rda);
    } else
	error=RETURN_FAIL;

    if (error)
	PrintFault(IoErr(),"List");

    return error;
}
