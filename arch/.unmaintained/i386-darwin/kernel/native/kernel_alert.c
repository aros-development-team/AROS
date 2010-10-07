/*
 *  alert.c
 *  AROS
 *
 *  Created by Daniel Oberhoff on 03.03.08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "kernel_alert.h"
#include "../include/aros/kernel.h"
#include <stdio.h>
#include <stdlib.h>

void Alert(int alertNum)
{
  static const char * CPUStrings[] =
  {
	"Hardware bus fault/access error",
	"Illegal address access (ie: odd)",
	"Illegal instruction",
	"Divide by zero",
	"Check instruction error",
	"TrapV instruction error",
	"Privilege violation error",
	"Trace error",
	"Line 1010 Emulator error",
	"Line 1111 Emulator error",
	"Stack frame format error",
	"Spurious interrupt error",
	"AutoVector Level 1 interrupt error",
	"AutoVector Level 2 interrupt error",
	"AutoVector Level 3 interrupt error",
	"AutoVector Level 4 interrupt error",
	"AutoVector Level 5 interrupt error",
	"AutoVector Level 6 interrupt error",
	"AutoVector Level 7 interrupt error",
  },
  * GenPurposeStrings[] =
  {
	"No memory",
	"Make library",
	"Open library",
	"Open device",
	"Open resource",
	"I/O error",
	"No signal",
	"Bad parameter",
	"Close library",
	"Close device",
	"Create process",
  },
  * AlertObjects[] =
  {
	"Exec",
	"Graphics",
	"Layers",
	"Intuition",
	
	"Math",
	"DOS",
	"RAM",
	"Icon",
	
	"Expansion",
	"Diskfont",
	"Utility",
	"Keymap",
	
	NULL,
	NULL,
	NULL,
	NULL,
	/* 0x10 */
	"Audio",
	"Console",
	"Gameport",
	"Keyboard",
	
	"Trackdisk",
	"Timer",
	NULL,
	NULL,
	
	NULL,
	NULL,
	NULL,
	NULL,
	
	NULL,
	NULL,
	NULL,
	NULL,
	/* 0x20 */
	"CIA",
	"Disk",
	"Misc",
	NULL,
	
	NULL,
	NULL,
	NULL,
	NULL,
	
	NULL,
	NULL,
	NULL,
	NULL,
	
	NULL,
	NULL,
	NULL,
	NULL,
	/* 0x30 */
	"Bootstrap",
	"Workbench",
	"Diskcopy",
	"Gadtools",
	
	"Unknown",
  },
  * ExecStrings[] =
  {
	"68000 exception vector checksum",
	"Execbase checksum",
	"Library checksum failure",
	NULL,
	
	"Corrupt memory list detected in FreeMem",
	"No memory for interrupt servers",
	"InitStruct() of an APTR source",
	"A semaphore is in an illegal state at ReleaseSemaphore",
	
	"Freeing memory already freed",
	"Illegal 68k exception taken",
	"Attempt to reuse active IORequest",
	"Sanity check on memory list failed",
	
	"IO attempted on closed IORequest",
	"Stack appears to extend out of range",
	"Memory header not located. (Usually an invalid address passed to FreeMem())",
	"An attempt was made to use the old message semaphores",
	
  };
  
//  struct Task * task;
  
#   define GetSubSysId(a)       (((a) >> 24) & 0x7F)
#   define GetGenError(a)       (((a) >> 16) & 0xFF)
#   define GetSpecError(a)      ((a) & 0xFFFF)
  
//  task = FindTask (NULL);
  
  /* since this is an emulation, we just show the bug in the console */
  fprintf (stderr
		   , "\x07" "GURU Meditation %04lx %04lx\n"
		   , alertNum >> 16
		   , alertNum & 0xFFFF
		   );
  
  if (alertNum & 0x80000000)
	fprintf (stderr
			 , "Deadend/"
			 );
  else
	fprintf (stderr
			 , "Recoverable/"
			 );
  
  switch (GetSubSysId (alertNum))
  {
	case 0: /* CPU/OS/App */
	  if (GetGenError (alertNum) == 0)
	  {
		fprintf (stderr
				 , "CPU/"
				 );
		
		if (GetSpecError (alertNum) >= 2 && GetSpecError (alertNum) <= 0x1F)
		  fprintf (stderr
				   , "%s"
				   , CPUStrings[GetSpecError (alertNum) - 2]
				   );
	    else
		  fprintf (stderr
				   , "*unknown*"
				   );
	  }
	  else if (GetGenError (alertNum) <= 0x0B)
	  {
		fprintf (stderr
				 , "%s/"
				 , GenPurposeStrings[GetGenError (alertNum) - 1]
				 );
		
		if (GetSpecError (alertNum) >= 0x8001
			&& GetSpecError (alertNum) <= 0x8035)
		{
		  fprintf (stderr
				   , "%s"
				   , AlertObjects[GetSpecError (alertNum) - 0x8001]
				   );
		}
		else
		  fprintf (stderr
				   , "*unknown*"
				   );
	  }
	  
	  break;
	  
	  case 1: /* Exec */
	  fprintf (stderr
			   , "Exec/"
			   );
	  
	  if (!GetGenError (alertNum)
		  && GetSpecError (alertNum) >= 0x0001
		  && GetSpecError (alertNum) <= 0x0010)
	  {
		fprintf (stderr
				 , "%s"
				 , ExecStrings[GetSpecError (alertNum) - 0x0001]
				 );
	  }
	  else
	  {
		fprintf (stderr
				 , "*unknown*"
				 );
	  }
	  
	  break;
	  
	  case 2: /* Graphics */
	  fprintf (stderr
			   , "Graphics/*unknown*"
			   );
	  
	  break;
	  
	  default:
	  fprintf (stderr
			   , "*unknown*/*unknown*"
			   );
  }
  
//  fprintf (stderr
//		   , "\nTask: %p (%s)\n"
//		   , task
//		   , (task && task->tc_Node.ln_Name) ?
//		   task->tc_Node.ln_Name
//		   : "-- unknown task --"
//		   );
  fflush (stderr);
  
  if (alertNum & 0x80000000)
	abort ();
}

void * kernelAlert (struct Hook * hook, unsigned long object, unsigned long message)
{
  struct TagItem * msg = (struct TagItem *)message;
  int anum = (int)(msg->ti_Data);
  Alert(anum);
  return 0;
}

