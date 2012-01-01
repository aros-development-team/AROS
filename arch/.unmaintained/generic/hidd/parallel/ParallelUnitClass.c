/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parallel Unit hidd class implementation.
    Lang: english
*/

/*
 * This is the SerialUnitClass as used for Linux. You will have to
 * modify this code here and make it work with the hardware that
 * you want to write the hidd for.
 * I hope the Linux code inside the methods helps a little bit
 * in explaining what needs to be done inside each method.
 */



/* Some POSIX includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>



/* the rest are Amiga includes */
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/alib.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/lists.h>

#include <utility/tagitem.h>
#include <hidd/parallel.h>
#include <hidd/unixio.h>

#include "parallel_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

void parallelunit_receive_data();
void parallelunit_write_more_data();

char * unitname[] =
{
 "/dev/lp0",
 "/dev/lp1",
 "/dev/lp2"
 };

/*************************** Classes *****************************/

static OOP_AttrBase HiddParallelUnitAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_ParallelUnit, &HiddParallelUnitAB },
    { NULL,	NULL }
};

/******* ParallelUnit::New() ***********************************/
static OOP_Object *parallelunit_new(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
  struct HIDDParallelUnitData * data;
  static const struct TagItem tags[] = {{ TAG_END, 0}};
  struct TagItem *tag, *tstate;
  ULONG unitnum = 0;
  
  EnterFunc(bug("ParallelUnit::New()\n"));

  tstate = msg->attrList;
  while ((tag = NextTagItem(&tstate)))
  {
      ULONG idx;

      if (IS_HIDDPARALLELUNIT_ATTR(tag->ti_Tag, idx))
      {
	  switch (idx)
	  {
	      case aoHidd_ParallelUnit_Unit:
		  unitnum = (ULONG)tag->ti_Data;
		  break;
	  }
      }

  } /* while (tags to process) */
    
  D(bug("!!!!Request for unit number %d\n",unitnum));

  obj = (OOP_Object *)OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);

  if (obj)
  {
    data = OOP_INST_DATA(cl, obj);
    
    data->unitnum = unitnum;

    D(bug("Opening %s.\n",unitname[data->unitnum]));

    data->filedescriptor = open(unitname[data->unitnum], O_NONBLOCK|O_RDWR);

    D(bug("Opened %s on handle %d\n",unitname[data->unitnum], data->filedescriptor));
    
    if (-1 != data->filedescriptor)
    {
      /*
      ** Configure the tty driver ??!?!?!
      */
      {
        data->replyport_read = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC|MEMF_CLEAR);
        data->replyport_write= AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC|MEMF_CLEAR);

        if (data->replyport_read && data->replyport_write)
        {
          /*
          ** Init the msg ports. They don't need a signal to be allocated
          */
          NEWLIST(&data->replyport_read->mp_MsgList);
          data->replyport_read ->mp_Node.ln_Type = NT_MSGPORT;

          NEWLIST(&data->replyport_write->mp_MsgList);
          data->replyport_write->mp_Node.ln_Type = NT_MSGPORT;

          data->softint_read  = AllocMem(sizeof(struct Interrupt), MEMF_CLEAR);
          data->softint_write = AllocMem(sizeof(struct Interrupt), MEMF_CLEAR);

          if (data->softint_read && data->softint_write)
          {
            data->softint_read->is_Data = data;
            data->softint_read->is_Code = parallelunit_receive_data;

            data->softint_write->is_Data = data;
            data->softint_write->is_Code = parallelunit_write_more_data;

            data->replyport_read->mp_Flags = PA_SOFTINT;
            data->replyport_read->mp_SoftInt = data->softint_read;

            data->replyport_write->mp_Flags = PA_SOFTINT;
            data->replyport_write->mp_SoftInt = data->softint_write;

            data->unixio_read  = OOP_NewObject(NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);
            data->unixio_write = OOP_NewObject(NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);

            if (NULL != data->unixio_read && NULL != data->unixio_write)
            {
              ULONG error;
              D(bug("Creating UnixIO AsyncIO command!\n"));

              error = Hidd_UnixIO_AsyncIO(data->unixio_read,
                                          data->filedescriptor,
                                          data->replyport_read,
                                          vHidd_UnixIO_Read);
              goto exit;

            }

            if (NULL != data->unixio_read)
              OOP_DisposeObject(data->unixio_read);

            if (NULL != data->unixio_write)
              OOP_DisposeObject(data->unixio_write);
          }
          
          if (data->softint_read) 
            FreeMem(data->softint_read, sizeof(struct Interrupt));
          if (data->softint_write)
            FreeMem(data->softint_write, sizeof(struct Interrupt));
        }
        
        if (data->replyport_read)
          FreeMem(data->replyport_read , sizeof(struct MsgPort));
        if (data->replyport_write)
          FreeMem(data->replyport_write, sizeof(struct MsgPort));

      } 
      
      close(data->filedescriptor);  
    }

    OOP_DisposeObject(obj);
    obj = NULL;
  } /* if (obj) */

  D(bug("%s - an error occurred!\n",__FUNCTION__));

exit:
  ReturnPtr("ParallelUnit::New()", OOP_Object *, obj);
}

/******* ParallelUnit::Dispose() ***********************************/
static OOP_Object *parallelunit_dispose(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
  struct HIDDParallelUnitData * data;
  EnterFunc(bug("ParallelUnit::Dispose()\n"));

  data = OOP_INST_DATA(cl, obj);
  D(bug("Freeing filedescriptor (%d)!\n",data->filedescriptor));

  if (-1 != data->filedescriptor)
  { 
    Hidd_UnixIO_AbortAsyncIO(data->unixio_read,
                             data->filedescriptor);
//    Hidd_UnixIO_AbortAsyncIONotification(data->unixio_write,
//                                         data->filedescriptor);

    close(data->filedescriptor);
  
    FreeMem(data->replyport_read,  sizeof(struct MsgPort));
    FreeMem(data->replyport_write, sizeof(struct MsgPort));

    FreeMem(data->softint_read , sizeof(struct Interrupt));
    FreeMem(data->softint_write, sizeof(struct Interrupt));

    OOP_DisposeObject(data->unixio_read);
    OOP_DisposeObject(data->unixio_write);
  }
  OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
  ReturnPtr("ParallelUnit::Dispose()", OOP_Object *, obj);
}



/******* ParallelUnit::Init() **********************************/
BOOL parallelunit_init(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Init *msg)
{
  struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);
  
  EnterFunc(bug("ParallelUnit::Init()\n"));
  data->DataReceivedCallBack = msg->DataReceived;
  data->DataReceivedUserData = msg->DataReceivedUserData;
  data->DataWriteCallBack    = msg->WriteData;
  data->DataWriteUserData    = msg->WriteDataUserData;

  ReturnBool("ParallelUnit::Init()", TRUE);
}

/******* ParallelUnit::Write() **********************************/
ULONG parallelunit_write(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Write *msg)
{
  struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);
  ULONG len = 0;
  ULONG error;
  
  EnterFunc(bug("ParallelUnit::Write()\n"));

  D(bug("Writing %d bytes to fd %d (stream: %s)\n",
        msg->Length,
        data->filedescriptor,
        msg->Outbuffer));

  len = write(data->filedescriptor,
              msg->Outbuffer,
              msg->Length);

  if (len < msg->Length)
  {

    // !!!!!! FROM WHAT I CAN TELL THE FOLLOWING LINE
    //        CAUSES PROBLEMS. IT IS NECESSARY TO HAVE IT, THOUGH.
    error = Hidd_UnixIO_AsyncIO(data->unixio_write,
                                data->filedescriptor,
                                data->replyport_write,
                                vHidd_UnixIO_Write);

  }

  ReturnInt("ParallelUnit::Write()",ULONG, len);
}

/************* The software interrupt handler that gets data from PORT *****/


#undef OOPBase
#undef SysBase
#undef UtilityBase

#define READBUFFER_SIZE 513

AROS_UFH3(void, parallelunit_receive_data,
   AROS_UFHA(APTR, iD, A1),
   AROS_UFHA(APTR, iC, A5),
   AROS_UFHA(struct ExecBase *, SysBase, A6))
{
  ULONG error;
  struct HIDDParallelUnitData * data = iD;
  ssize_t len;
  UBYTE buffer[READBUFFER_SIZE];
  struct Message * msg;

  /*
  ** Get the unixio message from my port and free it
  */
  msg = GetMsg(data->replyport_read);
  FreeMem(msg, sizeof(struct uioMessage));

  /*
  ** Read the data from the port ...
  */
  len = read(data->filedescriptor, buffer, READBUFFER_SIZE);
  /*
  ** ... and deliver them to whoever is interested. 
  */

  if (NULL != data->DataReceivedCallBack)
    data->DataReceivedCallBack(buffer, len, data->unitnum, data->DataReceivedUserData);

  /*
  ** I want to be notified when the next data are coming in.
  */
  error = Hidd_UnixIO_AsyncIO(data->unixio_read,
                              data->filedescriptor,
                              data->replyport_read,
                              vHidd_UnixIO_Read);

}

AROS_UFH3(void, parallelunit_write_more_data,
   AROS_UFHA(APTR, iD, A1),
   AROS_UFHA(APTR, iC, A5),
   AROS_UFHA(struct ExecBase *, SysBase, A6))
{
  struct HIDDParallelUnitData * data = iD;
  struct Message * msg;

  /*
  ** Get the unixio message from my port and free it
  */
  msg = GetMsg(data->replyport_read);
  FreeMem(msg, sizeof(struct uioMessage));

  /*
  ** Ask for more data be written to the unit
  */
  D(bug("Asking for more data to be written to unit %d\n",data->unitnum));

  if (NULL != data->DataWriteCallBack)
    data->DataWriteCallBack(data->unitnum, data->DataWriteUserData);
}


/******* init_parallelunitclass ********************************/

#define SysBase     (csd->sysbase)
#define OOPBase     (csd->oopbase)
#define UtilityBase (csd->utilitybase)


#define NUM_ROOT_METHODS 2
#define NUM_PARALLELUNIT_METHODS 2

OOP_Class *init_parallelunitclass (struct class_static_data *csd)
{
    OOP_Class *cl = NULL;
    
    struct OOP_MethodDescr parallelunithiddroot_descr[NUM_ROOT_METHODS + 1] = 
    {
        {(IPTR (*)())parallelunit_new,		moRoot_New},
        {(IPTR (*)())parallelunit_dispose,	moRoot_Dispose},
/*
        {(IPTR (*)())parallelunit_set,		moRoot_Set},
        {(IPTR (*)())parallelunit_get,		moRoot_Get},
*/
        {NULL, 0UL}
    };
    
    struct OOP_MethodDescr parallelunithidd_descr[NUM_PARALLELUNIT_METHODS + 1] =
    {
        {(IPTR (*)())parallelunit_init,		moHidd_ParallelUnit_Init},
        {(IPTR (*)())parallelunit_write,	moHidd_ParallelUnit_Write},
        {NULL, 0UL}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {parallelunithiddroot_descr	, IID_Root		, NUM_ROOT_METHODS},
        {parallelunithidd_descr		, IID_Hidd_ParallelUnit	, NUM_PARALLELUNIT_METHODS},
        {NULL, NULL, 0}
    };

    OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
        { aMeta_SuperID,                (IPTR)CLID_Root},
        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_ParallelUnit},
        { aMeta_InstSize,               (IPTR)sizeof (struct HIDDParallelUnitData) },
        {TAG_DONE, 0UL}
    };


    EnterFunc(bug("    init_parallelunitclass(csd=%p)\n", csd));

    cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    D(bug("Class=%p\n", cl));
    if(cl)
    {
	if (OOP_ObtainAttrBases(attrbases))
	{
            D(bug("ParallelUnit Class ok\n"));
            cl->UserData = (APTR)csd;

            OOP_AddClass(cl);
	} else {
	    free_parallelunitclass(csd);
	    cl = NULL;
	}
    }

    ReturnPtr("init_parallelunitclass", OOP_Class *, cl);
}


void free_parallelunitclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_parallelhiddclass(csd=%p)\n", csd));

    if(csd)
    {
        OOP_RemoveClass(csd->parallelhiddclass);
	
        if(csd->parallelhiddclass) OOP_DisposeObject((OOP_Object *) csd->parallelhiddclass);
        csd->parallelhiddclass = NULL;
    }

    ReturnVoid("free_parallelhiddclass");
}

