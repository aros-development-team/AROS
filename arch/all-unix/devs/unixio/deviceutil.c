static int unixio_Open(struct unixioDev *, struct IORequest *ioreq, IPTR unit, ULONG flags)
{
    struct UnitData *data;

    D(bug("unixio.device: Open unit %s\n",unit));

    if (ioreq->io_Message.mn_Length < sizeof(struct IOStdReq))
    {
        D(bug("unixio.device/open: IORequest structure passed to OpenDevice is too small!\n"));

        ioreq->io_Error = IOERR_OPENFAIL;
        return FALSE;
    }

    ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

    /*
     * In the list of available units look for the one with the same 
     UnitNumber as the given one  */
  if (0 == ioreq->io_Error)
  {
    PU = findUnit(ParallelDevice, unitnum);

    /* If there is no such unit, yet, then create it */
    if (NULL == PU)
    {
      D(bug("Creating Unit %d\n",unitnum));
      PU = AllocMem(sizeof(struct ParallelUnit), MEMF_CLEAR|MEMF_PUBLIC);
      if (NULL != PU)
      {
        PU->pu_OpenerCount	= 1;
        PU->pu_UnitNum		= unitnum;
        PU->pu_Flags		= ioreq->io_Flags;
        
        /*
        ** Initialize the message ports
        */
        NEWLIST(&PU->pu_QReadCommandPort.mp_MsgList);
        PU->pu_QReadCommandPort.mp_Node.ln_Type = NT_MSGPORT;
          
        NEWLIST(&PU->pu_QWriteCommandPort.mp_MsgList);
        PU->pu_QWriteCommandPort.mp_Node.ln_Type= NT_MSGPORT;
   
        InitSemaphore(&PU->pu_Lock);
        /* do further initilization here. Like getting the ParallelUnit Object etc. */

        PU->pu_Unit  = HIDD_Parallel_NewUnit(ParallelDevice->ParallelObject, unitnum);
        if (NULL != PU->pu_Unit)
        {
          HIDD_ParallelUnit_Init(PU->pu_Unit, RBF_InterruptHandler, NULL, WBE_InterruptHandler, NULL);
          ioreq->io_Device = (struct Device *)ParallelDevice;
          ioreq->io_Unit   = (struct Unit *)PU;  

          /*
          ** put it in the list of open units
          */
          AddHead(&ParallelDevice->UnitList, (struct Node *)PU);

          ioreq->io_Error  = 0;
 
          return TRUE;
        }

        D(bug("ParallelUnit could not be created!\n"));
          
        FreeMem(PU, sizeof(struct ParallelUnit));

        ioreq->io_Error = ParErr_DevBusy;
      }
    }
    else
    {
      /* the unit does already exist. */
      /* 
      ** Check whether one more opener to this unit is tolerated 
      */
      if (0 != (PU->pu_Flags & PARF_SHARED))
      {
        /*
        ** This unit is in shared mode and one more opener
        ** won't hurt.
        */
        ioreq->io_Device = (struct Device *)ParallelDevice;
        ioreq->io_Unit   = (struct Unit *)PU;
        ioreq->io_Error  = 0;

        PU->pu_OpenerCount++;
      }
      else
      {
        /*
        ** I don't allow another opener
        */
        ioreq->io_Error = ParErr_DevBusy;
      }
    }
  }

  return TRUE;
}


/****************************************************************************************/

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR ParallelDevice,
    struct IORequest *ioreq
)
{
  struct ParallelUnit * PU = (struct ParallelUnit *)ioreq->io_Unit;

  /*
  ** Check whether I am the last opener to this unit
  */
  if (1 == PU->pu_OpenerCount)
  {
    /*
    ** I was the last opener. So let's get rid of it.
    */
    /*
    ** Remove the unit from the list
    */
    Remove((struct Node *)&PU->pu_Node);
    
    HIDD_Parallel_DisposeUnit(ParallelDevice->ParallelObject, PU->pu_Unit);
  
    FreeMem(PU, sizeof(struct ParallelUnit));
  
  }
  else
  {
    /*
    ** There are still openers. Decrease the counter.
    */
    PU->pu_OpenerCount--;
  }

  return TRUE;
}

/****************************************************************************************/
