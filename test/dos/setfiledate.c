/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <dos/datetime.h>

int main(void)
{
    struct DateTime curr;
    char day[LEN_DATSTRING];
    char time[LEN_DATSTRING];
    char date[LEN_DATSTRING];
    struct DateStamp stamp;

    curr.dat_Format  = FORMAT_DOS;
    curr.dat_Flags   = 0;
    curr.dat_StrDay  = day;
    curr.dat_StrDate = date;
    curr.dat_StrTime = time;  

    DateStamp(&curr.dat_Stamp);
    DateToStr(&curr);
    Printf("Current time: %s, %s, %s\n", day, date, time);
    
    BPTR fh = Open("__TEST__", MODE_NEWFILE);
    
    if (fh != BNULL)
    {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        
        if (fib != NULL)
        {
            if (ExamineFH(fh, fib))
            { 
        	curr.dat_Stamp = fib->fib_Date;
        	DateToStr(&curr);
                Printf("File modification time: %s, %s, %s\n", day, date, time);
            }
            else
        	PrintFault(IoErr(), "Examine failed");

            Printf("Waiting 5 seconds\n");
            Delay(5*50);
            
            DateStamp(&stamp);
            
            Printf("Calling SetFileDate\n");
            if(SetFileDate("__TEST__", &stamp)) {
                if (ExamineFH(fh, fib))
                { 
                    curr.dat_Stamp = fib->fib_Date;
                    DateToStr(&curr);
                    Printf("New file modification time: %s, %s, %s\n", day, date, time);
                }
                else
                    PrintFault(IoErr(), "Examine failed");        	
            }
            else
        	PrintFault(IoErr(), "SetFileDate");
            
            FreeDosObject(DOS_FIB, fib);
        }
        else 
            PrintFault(IoErr(), "Couldn't alloc FileInfoBlock");
            
        Close(fh);
        DeleteFile("__TEST__");
    }
    else
	PrintFault(IoErr(), "Couldn't create file");
    
    return 0;
}
