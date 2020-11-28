#include <exec/types.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#include <string.h>

#include "asmsupport.h"
#include "debug.h"
#include "transactions_protos.h"

#include "req_protos.h"


void dreqArgs(UBYTE *fmt, APTR params)
{
  UBYTE *fmt2;

  if(globals->debugreqs!=FALSE) {
    struct {
        CONST_STRPTR name, device;
        ULONG unit;
        CONST_STRPTR fmt;
    } __packed args = {
        AROS_BSTR_ADDR(globals->devnode->dn_Name),
        AROS_BSTR_ADDR(globals->startupmsg->fssm_Device),
        globals->startupmsg->fssm_Unit,
        (CONST_STRPTR)fmt
    };

    if((fmt2=AllocVec(strlen(fmt)+100,0))!=0) {
      _DEBUG(("\nREQUESTER\n\n"));
      RawDoFmt("SmartFilesystem %s: (%s, unit %ld)\n\n%s",(RAWARG)&args,putChFunc,fmt2);

      if (requestArgs(PROGRAMNAME, fmt2, "Continue|No more requesters", params) == 0)
        globals->debugreqs=FALSE;

      FreeVec(fmt2);
    }
  }
}



LONG reqArgs(UBYTE *fmt, UBYTE *gads, APTR params)
{
  struct {
      CONST_STRPTR dl;
      CONST_STRPTR dn;
      CONST_STRPTR device;
      ULONG unit;
      CONST_STRPTR fmt;
  } __packed args;

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
    args.dl=AROS_BSTR_ADDR(globals->volumenode->dl_Name);
  }

  args.dn=AROS_BSTR_ADDR(globals->devnode->dn_Name);
  args.device=AROS_BSTR_ADDR(globals->startupmsg->fssm_Device);
  args.unit=globals->startupmsg->fssm_Unit;
  args.fmt=fmt;

  if((fmt2=AllocVec(strlen(fmt)+100,0))!=0) {

    if(globals->volumenode!=0) {
      RawDoFmt("Volume '%s' (%s: %s, unit %ld)\n\n%s",(RAWARG)&args.dl,putChFunc,fmt2);
    }
    else {
      RawDoFmt("Device %s: (%s, unit %ld)\n\n%s",(RAWARG)&args.dn,putChFunc,fmt2);
    }

    gadget = requestArgs(PROGRAMNAME " request", fmt2, gads, params);
    FreeVec(fmt2);
  }

  return(gadget);
}



LONG req_unusualArgs(UBYTE *fmt, APTR params)
{
  struct {
      CONST_STRPTR dn;
      CONST_STRPTR device;
      ULONG unit;
      CONST_STRPTR fmt;
      CONST_STRPTR msg;
  } __packed args;
  UBYTE *fmt2;
  LONG gadget=0;

  /* Simple requester function. */
  args.dn=AROS_BSTR_ADDR(globals->devnode->dn_Name);
  args.device=AROS_BSTR_ADDR(globals->startupmsg->fssm_Device);
  args.unit=globals->startupmsg->fssm_Unit;
  args.fmt=fmt;
  args.msg="This is a safety check requester, which should\n"\
                 "never appear under normal conditions.  Please\n"\
                 "notify the author about the error above and if\n"\
                 "possible under what circumstances it appeared.\n\n"\
                 "BEWARE: SFS might crash if you click Continue.\n"\
                 "        Please save your work first!";

  if((fmt2=AllocVec(strlen(fmt)+400,0))!=0) {

    RawDoFmt("SmartFilesystem %s: (%s, unit %ld)\n\n%s\n\n%s",(RAWARG)&args,putChFunc,fmt2);
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
