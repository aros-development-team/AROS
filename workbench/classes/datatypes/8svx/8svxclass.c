/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/soundclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/datatypes.h>

#define DEBUG 1
#include "compilerspecific.h"
#include "debug.h"

#include "methods.h"

/**************************************************************************************************/

struct FileVoiceHeader
{
    UBYTE vh_OneShotHiSamples[4];
    UBYTE vh_RepeatHiSamples[4];
    UBYTE vh_SamplesPerHiCycle[4];
    UBYTE vh_SamplesPerSec[2];
    UBYTE vh_Octaves;
    UBYTE vh_Compression;
    UBYTE vh_Volume[4];
};

/**************************************************************************************************/

static LONG propchunks[] =
{
    ID_8SVX, ID_VHDR
};

/**************************************************************************************************/
/**************************************************************************************************/

BOOL Read8SVX(Class *cl, Object *o)
{

    struct FileVoiceHeader  *file_vhd;
    struct VoiceHeader      *vhd;
    struct IFFHandle	    *handle;
    struct StoredProperty   *vhdr_prop;
    struct ContextNode	    *cn;
    UBYTE   	    	    *sample;
    ULONG   	    	    samplesize;
    IPTR    	    	    sourcetype;
    LONG    	    	    error;
 
 
    D(bug("8svx.datatype/Read8SVX()\n"));
       
    if (GetDTAttrs(o, DTA_SourceType	, (IPTR)&sourcetype ,
    	    	      DTA_Handle    	, (IPTR)&handle     , 
		      SDTA_VoiceHeader  , (IPTR)&vhd	    ,
		      TAG_DONE	    	    	    	     ) != 3)
    {
    	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }
    
    if ((sourcetype != DTST_FILE) && (sourcetype != DTST_CLIPBOARD))
    {
    	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }
    
    if (!handle || !vhd)
    {
    	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }

    if (PropChunks(handle, propchunks, 3) != 0)
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
	D(bug("8svx.datatype error propchunks\n"));
	return FALSE;
    }
   
    if (StopChunk(handle, ID_8SVX, ID_BODY) != 0)
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
	D(bug("8svx.datatype error stopchunks\n"));
	return FALSE;
    }
    
    error = ParseIFF(handle, IFFPARSE_SCAN);
    if (error)
    {
    	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	D(bug("8svx.datatype error parseiff\n"));
	return FALSE;
    }
    
    vhdr_prop = FindProp(handle, ID_8SVX, ID_VHDR);

    cn = CurrentChunk(handle);
    if ((cn->cn_Type != ID_8SVX) ||
    	(cn->cn_ID != ID_BODY) ||
	(vhdr_prop == NULL))
    {
    	SetIoErr(ERROR_REQUIRED_ARG_MISSING);
	D(bug("8svx.datatype error currentchunk\n"));
	return FALSE;
    }

    file_vhd = (struct FileVoiceHeader *)vhdr_prop->sp_Data;
    vhd->vh_OneShotHiSamples = file_vhd->vh_OneShotHiSamples[0] * 0x1000000 +
    	    	    	       file_vhd->vh_OneShotHiSamples[1] * 0x10000 +
			       file_vhd->vh_OneShotHiSamples[2] * 0x100 +
			       file_vhd->vh_OneShotHiSamples[3];
    vhd->vh_RepeatHiSamples = file_vhd->vh_RepeatHiSamples[0] * 0x1000000 +
    	    	    	      file_vhd->vh_RepeatHiSamples[1] * 0x10000 +
			      file_vhd->vh_RepeatHiSamples[2] * 0x100 +
			      file_vhd->vh_RepeatHiSamples[3];
    vhd->vh_SamplesPerHiCycle = file_vhd->vh_SamplesPerHiCycle[0] * 0x1000000 +
    	    	    	        file_vhd->vh_SamplesPerHiCycle[1] * 0x10000 +
			        file_vhd->vh_SamplesPerHiCycle[2] * 0x100 +
			        file_vhd->vh_SamplesPerHiCycle[3];
    vhd->vh_SamplesPerSec = file_vhd->vh_SamplesPerSec[0] * 0x100 +
    	    	    	    file_vhd->vh_SamplesPerSec[1];
    vhd->vh_Octaves = file_vhd->vh_Octaves;
    vhd->vh_Compression = file_vhd->vh_Compression;
    vhd->vh_Volume = file_vhd->vh_Volume[0] * 0x1000000 +
    	    	     file_vhd->vh_Volume[1] * 0x10000 +
		     file_vhd->vh_Volume[2] * 0x100 +
		     file_vhd->vh_Volume[3];
    
    D(bug("8svx.datatype: OneShotHiSamples : %d\n"
    	  "               RepeatHiSampoles : %d\n"
	  "               SamplesPerHiCycle: %d\n"
	  "               SamplesPerSec    : %d\n"
	  "               Octaves          : %d\n"
	  "               Compression      : %d\n"
	  "               Volume           : %d\n",
	  vhd->vh_OneShotHiSamples,
	  vhd->vh_RepeatHiSamples,
	  vhd->vh_SamplesPerHiCycle,
	  vhd->vh_SamplesPerSec,
	  vhd->vh_Octaves,
	  vhd->vh_Compression,
	  vhd->vh_Volume));
	  
    if (vhd->vh_Compression != 0)
    {
    	SetIoErr(ERROR_NOT_IMPLEMENTED);
	return FALSE;
    }

    D(bug("8svx.datatype: BODY chunk size %d\n", cn->cn_Size));
    
    samplesize = vhd->vh_OneShotHiSamples + vhd->vh_RepeatHiSamples;
    
    if (samplesize > cn->cn_Size)
    {
    	SetIoErr(ERROR_BAD_NUMBER);
	return FALSE;
    }
    
    sample = AllocVec(samplesize, MEMF_PUBLIC);
    if (!sample)
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
    
    if (ReadChunkBytes(handle, sample, samplesize) != samplesize)
    {
    	FreeVec(sample);
	SetIoErr(ERROR_UNKNOWN);
	return FALSE;
    }
    
    SetDTAttrs(o, NULL, NULL, DTA_ObjName, (IPTR)"Unknown",
    	    		      SDTA_Sample, (IPTR)sample,
			      SDTA_SampleLength, samplesize,
			      SDTA_Period, 709379 * 5 / vhd->vh_SamplesPerSec,
			      SDTA_Volume, 64,
			      SDTA_Cycles, 1,
			      TAG_DONE);
    
    return TRUE;
}

/**************************************************************************************************/

static IPTR SVX_New(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
    	if (!Read8SVX(cl, (Object *)retval))
	{
	    CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
	    retval = 0;
	}
    }
    
    return retval;
}

/**************************************************************************************************/

#ifdef __AROS__
AROS_UFH3S(IPTR, DT_Dispatcher,
	   AROS_UFHA(Class *, cl, A0),
	   AROS_UFHA(Object *, o, A2),
	   AROS_UFHA(Msg, msg, A1))
#else
ASM IPTR DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object * o, register __a1 Msg msg)
#endif
{
#ifdef __AROS__
    AROS_USERFUNC_INIT
#endif

    IPTR retval;

    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);        /* Small Data */

    // D(bug("8svx.datatype/DT_Dispatcher: Entering\n"));

    switch(msg->MethodID)
    {
	case OM_NEW:
	    D(bug("8svx.datatype/DT_Dispatcher: Method OM_NEW\n"));
	    retval = SVX_New(cl, o, (struct opSet *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;

    } /* switch(msg->MethodID) */

    // D(bug("8svx.datatype/DT_Dispatcher: Leaving\n"));

    return retval;
    
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

/**************************************************************************************************/

struct IClass *DT_MakeClass(struct Library *libbase)
{
    struct IClass *cl;
    
    cl = MakeClass("8svx.datatype", "sound.datatype", 0, 0, 0);

    D(bug("8svx.datatype/DT_MakeClass: DT_Dispatcher 0x%lx\n", (unsigned long) DT_Dispatcher));

    if (cl)
    {
#ifdef __AROS__
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(DT_Dispatcher);
#else
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
#endif
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
	cl->cl_UserData = (IPTR)libbase; /* Required by datatypes (see disposedtobject) */
    }

    return cl;
}

/**************************************************************************************************/

