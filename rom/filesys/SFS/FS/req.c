#include <exec/types.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#include "asmsupport.h"
#include "debug.h"
#include "transactions_protos.h"

#include "req_protos.h"


void dreqArgs(UBYTE *fmt, APTR params)
{
  APTR args[4];
  UBYTE *fmt2;

  if(globals->debugreqs!=FALSE) {
    args[0]=AROS_BSTR_ADDR(globals->devnode->dn_Name);
    args[1]=AROS_BSTR_ADDR(globals->startupmsg->fssm_Device);
    args[2]=(APTR)globals->startupmsg->fssm_Unit;
    args[3]=fmt;

    if((fmt2=AllocVec(strlen(fmt)+100,0))!=0) {
      _DEBUG(("\nREQUESTER\n\n"));
      RawDoFmt("SmartFilesystem %s: (%s, unit %ld)\n\n%s",args,putChFunc,fmt2);

      if (requestArgs(PROGRAMNAME, fmt2, "Continue|No more requesters", params) == 0)
        globals->debugreqs=FALSE;

      FreeVec(fmt2);
    }
  }
}



LONG reqArgs(UBYTE *fmt, UBYTE *gads, APTR params)
{
  APTR args[5];
  APTR *arg=args;
  UBYTE *fmt2;
  LONG gadget=0;

  /* Simple requester function.  It will put up a requester
     in the form of:

     "Volume 'BOOT' (DH0: scsi.device, unit 0)"

     or:

     "Device DH0: (scsi.device, unit 0)"

     This depends on whether or not there is a valid
     VolumeNode. */

  if(globals->volumenode!=0) {
    *arg++=AROS_BSTR_ADDR(globals->volumenode->dl_Name);
  }

  *arg++=AROS_BSTR_ADDR(globals->devnode->dn_Name);
  *arg++=AROS_BSTR_ADDR(globals->startupmsg->fssm_Device);
  *arg++=(APTR)globals->startupmsg->fssm_Unit;
  *arg=fmt;

  if((fmt2=AllocVec(strlen(fmt)+100,0))!=0) {

    if(globals->volumenode!=0) {
      RawDoFmt("Volume '%s' (%s: %s, unit %ld)\n\n%s",args,putChFunc,fmt2);
    }
    else {
      RawDoFmt("Device %s: (%s, unit %ld)\n\n%s",args,putChFunc,fmt2);
    }

    gadget = requestArgs(PROGRAMNAME " request", fmt2, gads, params);
    FreeVec(fmt2);
  }

  return(gadget);
}



LONG req_unusualArgs(UBYTE *fmt, APTR params)
{
  APTR args[5];
  UBYTE *fmt2;
  LONG gadget=0;

  /* Simple requester function. */
  args[0]=AROS_BSTR_ADDR(globals->devnode->dn_Name);
  args[1]=AROS_BSTR_ADDR(globals->startupmsg->fssm_Device);
  args[2]=(APTR)globals->startupmsg->fssm_Unit;
  args[3]=fmt;
  args[4]="This is a safety check requester, which should\n"\
                 "never appear under normal conditions.  Please\n"\
                 "notify the author about the error above and if\n"\
                 "possible under what circumstances it appeared.\n\n"\
                 "BEWARE: SFS might crash if you click Continue.\n"\
                 "        Please save your work first!";

  if((fmt2=AllocVec(strlen(fmt)+400,0))!=0) {

    RawDoFmt("SmartFilesystem %s: (%s, unit %ld)\n\n%s\n\n%s",args,putChFunc,fmt2);
    gadget = requestArgs(PROGRAMNAME " request", fmt2, "Continue", params);
    FreeVec(fmt2);
  }

  return(gadget);
}



void request2(UBYTE *text) {
  request(PROGRAMNAME, text, "Ok", 0);
}



LONG requestArgs(UBYTE *title, UBYTE *fmt, UBYTE *gads, APTR params)
{
  struct EasyStruct es;

  es.es_StructSize=sizeof(struct EasyStruct);
  es.es_Flags=0;
  es.es_Title=title;
  es.es_TextFormat=fmt;
  es.es_GadgetFormat=gads;

  return EasyRequestArgs(0, &es, 0, params);
}



void outputcachebuffer(struct CacheBuffer *cb) {
  ULONG *a;
  UWORD n;

  _DEBUG(("CacheBuffer at address 0x%08lx of block %ld (Locked = %ld, Bits = 0x%02lx)\n",cb,cb->blckno,(LONG)cb->locked,(LONG)cb->bits));

  a=cb->data;

  for(n=0; n<(globals->bytes_block>>5); n++) {
    _DEBUG(("%08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]));
    a+=8;
  }
}
