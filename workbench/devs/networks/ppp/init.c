/*
 * $Id$
 */

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/lists.h>

#include <aros/io.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/newstyle.h>
#include <devices/timer.h>
#include <devices/serial.h>
#include <devices/timer.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <proto/timer.h>
#include <proto/utility.h>

#include <stdlib.h>
#include <stdio.h>

#include <oop/oop.h>
#include <hidd/pci.h>

#include <ctype.h>


#include "ppp.h"
#include "device_protos.h"
#include LC_LIBDEFS_FILE






BOOL GetToken(BYTE *b,BYTE *t){
  BYTE *tok;
  ULONG tlen; 
  for( tok = b ; ( *tok == ' ' )&&( *tok != 0 ) ; tok++ );    
  for( tlen=0 ; ( tok[tlen] != ' ' )&&( tok[tlen] != 0 )&&( tok[tlen] != '\n' ) ; tlen++ ); // I am sorry
  tok[tlen] = 0; 
  if( tlen < 1 || tlen >= PPP_MAXARGLEN ) return FALSE;
  strcpy(t,tok);
  for(  ; b < tok+tlen+1 ; b++ )*b=' ';
  //bug("t:%s:",tok);
  return TRUE;
}   

BOOL GetLineEnd(BYTE *b,BYTE *t){
  BYTE *tok;
  ULONG tlen;
  for( tok = b ; ( *tok == ' ' )&&( *tok != 0 ) ; tok++ );    
  for( tlen=0 ; ( tok[tlen] != 0 )&&( tok[tlen] != '\n' ) ; tlen++ );
  tok[tlen] = 0; 
  if( tlen < 1 || tlen >= PPP_MAXARGLEN ) return FALSE;
  strcpy(t,tok);
  while( tok[ strlen( tok ) ] == ' ' ) tok[ strlen( tok ) ] = 0;
  //bug("t:%s:",tok);
  return TRUE;
} 


BOOL ReadConfig(LIBBASETYPEPTR LIBBASE)
{
    UBYTE *linebuff,tok[PPP_MAXARGLEN];
    BPTR ConfigFile;
    ULONG comnum=0;
    UWORD i;
    
     D(bug("ReadConfig:  ENV:AROSTCP/db/ppp.config\n"));
     
     strcpy( LIBBASE->username ,   "DummyName");
     strcpy( LIBBASE->password ,   "DummyName");
     strcpy( LIBBASE->DeviceName , "DummyName");
     LIBBASE->SerUnitNum = 0;
     LIBBASE->enable_dns = FALSE;
      
  
    for(i=0;i<PPP_MAXATCOM; i++) LIBBASE->atc[i].command = 0;// zeroing commands
     
    if(ConfigFile = Open("ENV:AROSTCP/db/ppp.config",MODE_OLDFILE))
    {
 
    if(linebuff = AllocMem(256,MEMF_CLEAR|MEMF_PUBLIC))
    {
       
        while(FGets(ConfigFile, linebuff, 255))
        {
           
           if( ( linebuff[0] == '#' ) | ( linebuff[0] == ';' ) ) /* Skip comment lines */
           continue;

         //  bug("line:%s:\n",linebuff); 
           
           if( GetToken(linebuff,tok) ){
               
             if( strcasecmp("DEVICE",tok) == 0 ){
               if( GetToken(linebuff,tok) ) strcpy( LIBBASE->DeviceName , tok );
             } 
             else if( strcasecmp("UNIT",tok) == 0 ){
               if( GetToken(linebuff,tok) ) LIBBASE->SerUnitNum = atoi( tok );
             }  
             else if( strcasecmp("USERNAME",tok) == 0 ){
               if( GetToken(linebuff,tok) ) strcpy( LIBBASE->username , tok );
             }
             else if( strcasecmp("PASSWORD",tok) == 0 ){
               if( GetToken(linebuff,tok) ) strcpy( LIBBASE->password , tok );
             }
             else if( strcasecmp("ENABLE",tok) == 0 ){
               if( GetToken(linebuff,tok) ){ 
                  if( strcasecmp("DNS",tok) == 0 ){
                       LIBBASE->enable_dns = TRUE;
                  }   
               }
             }
             
             if( comnum < PPP_MAXATCOM ){
                 
               if( strcasecmp("SEND",tok) == 0 ){  
                 if( GetLineEnd(linebuff,tok) ){
					 strcat( tok , "\r" );
                     strcpy( LIBBASE->atc[comnum].str , tok );
                     LIBBASE->atc[comnum].command = COM_SEND;
                //   bug("send=%s\n",LIBBASE->atc[comnum].str );
                     comnum++;
                 }     
               }
             
              else if( strcasecmp("WAIT",tok) == 0 ){     
                if( GetToken(linebuff,tok) ){
                    strcpy( LIBBASE->atc[comnum].str , tok );
                    LIBBASE->atc[comnum].command = COM_WAIT;
                    if( GetToken(linebuff,tok) ){ 
                        LIBBASE->atc[comnum].arg = atoi( tok );
                    }else{
                       LIBBASE->atc[comnum].arg = 5;
                    }      
                    comnum++;
                 }     
              }
            
               else if( strcasecmp("DELAY",tok) == 0 ){      
                  LIBBASE->atc[comnum].command = COM_DELAY;
                  if( GetToken(linebuff,tok) ){ 
                       LIBBASE->atc[comnum].arg = atoi( tok );
                     }else{
                       LIBBASE->atc[comnum].arg = 5;
                  }      
                  comnum++;
                }    
              }  
                
            }             
                        
          
        }
        FreeMem(linebuff, 256);
    }
    Close(ConfigFile);
    }else{
      bug("ppp.config missing !!!!!!!!!!!!!!!!!!!!!!!!!!\n");   
    }   
   return TRUE;
}




BOOL DialUp(LIBBASETYPEPTR LIBBASE)
{
    
   ULONG i=0;   
   
   while( LIBBASE->atc[i].command ){
       
      if( LIBBASE->atc[i].command == COM_DELAY ){
         bug("DELAY %d sec. RESPONSE IS:\n",LIBBASE->atc[i].arg);  
         SerDelay( LIBBASE , LIBBASE->atc[i].arg );  
         bug("\n");  
      } 
      
      if( LIBBASE->atc[i].command == COM_WAIT ){
         bug("WAIT \"%s\" %d sec. RESPONSE IS:\n",LIBBASE->atc[i].str , LIBBASE->atc[i].arg); 
         if( ! WaitStr( LIBBASE, LIBBASE->atc[i].str , LIBBASE->atc[i].arg ) ){
              bug("\n...FAIL:TIMEOUT!\n");
              return FALSE;
         }
         bug("\n");  
      } 
      
     if( LIBBASE->atc[i].command == COM_SEND ){
         bug("SEND \"%s\"\n",LIBBASE->atc[i].str); 
         DoStr( LIBBASE, LIBBASE->atc[i].str);	 
	   //  SendStr( LIBBASE, LIBBASE->atc[i].str ,15 );
      } 
     
      i++; 
   }       
    
  return TRUE;  
                 
}   
    
    
    
    
    

void SetTimer(LIBBASETYPEPTR LIBBASE,const ULONG t)
{
  
  if( LIBBASE->TimeReq ){
      
    AbortIO((struct IORequest *)LIBBASE->TimeReq);   
    WaitIO((struct IORequest *)LIBBASE->TimeReq);
    while(GetMsg(LIBBASE->TimeMsg));
    
    if( t <= 0 ) return;    
    
    LIBBASE->TimeReq->tr_time.tv_secs = t;
    LIBBASE->TimeReq->tr_time.tv_micro = 0;
    ((struct IORequest *)LIBBASE->TimeReq)->io_Command = TR_ADDREQUEST;
    SendIO( (struct IORequest *)LIBBASE->TimeReq ); 
    
  }  
}   


VOID PPP_Process(VOID)
{
    struct Process *proc;
    struct IOExtSer *ioser;
    struct IOSana2Req *ios2;
    ULONG waitmask,signals;
    UBYTE signalbit;
    BOOL initf = FALSE;
    
    LIBBASETYPEPTR LIBBASE; 
    LIBBASE = FindTask(NULL)->tc_UserData;
     
 bug("PPP process  hello!\n");

    proc = (struct Process *)FindTask(0L);

   
    signalbit = AllocSignal(-1L);
    if(signalbit != -1){
     
      LIBBASE->sd_Unit->unit_MsgPort.mp_SigBit = signalbit;
      LIBBASE->sd_Unit->unit_MsgPort.mp_SigTask = (struct Task *)proc;
      LIBBASE->sd_Unit->unit_MsgPort.mp_Flags = PA_SIGNAL;

      InitSemaphore(&LIBBASE->sdu_ListLock);
   
      NEWLIST((struct List *)&LIBBASE->sdu_Rx);
      NEWLIST((struct List *)&LIBBASE->sdu_Tx);
      
      if( LIBBASE->TimeMsg = CreateMsgPort() ){ 
        if( LIBBASE->TimeReq = CreateIORequest((struct MsgPort *)LIBBASE->TimeMsg, sizeof(struct timerequest))){
          if (!OpenDevice("timer.device", UNIT_VBLANK,(struct IORequest *)LIBBASE->TimeReq, 0)){    
             initf = TRUE;  
      }}}           
    }           
                

 if( initf ){
       
    bug("PPP process: forewer loop...\n");
    
    LIBBASE->TimeReq->tr_time.tv_secs = 10;
    LIBBASE->TimeReq->tr_time.tv_micro = 0;
    ((struct IORequest *)LIBBASE->TimeReq)->io_Command = TR_ADDREQUEST;
    SendIO( (struct IORequest *)LIBBASE->TimeReq );
    Delay(10);     
    SetTimer(LIBBASE,4);    
    
    LIBBASE->sdu_Proc_run = TRUE;
    
    for(;;){
        
        waitmask = (1L<< signalbit ) | 
                   (1L<< LIBBASE->sdu_RxPort->mp_SigBit ) |
                   (1L<< LIBBASE->sdu_TxPort->mp_SigBit ) |
                   (1L<< LIBBASE->TimeMsg->mp_SigBit ) |
                   SIGBREAKF_CTRL_F  ;
                
        signals = Wait(waitmask);
    
      if(GetMsg(LIBBASE->TimeMsg)){
		  
        // bug("PPP process: Timer\n");
        //bug(" ser %d,ppp %d,dev %d\n",LIBBASE->serial_ok,LIBBASE->ppp_online,LIBBASE->device_up );
    
       if( LIBBASE->device_up && ( ! LIBBASE->serial_ok ) ){
           
          D(bug("[PPP] ModemDemonProcess: trying OpenSerial..!\n")); 
          
          if( OpenSerial(LIBBASE) ){                           
              D(bug("[PPP] ModemDemonProcess: Serial OK !\n"));                                         

                   if( ! DialUp(LIBBASE) ){  
                       
                       CloseSerial(LIBBASE);
                       SetTimer(LIBBASE,5); 
                       
                   } else {
                         
                   init_ppp( LIBBASE );

                   QueueSerRequest(LIBBASE , PPP_MAXBUFF );   
                  
                 }         
             }  
         }     
         
         SetTimer(LIBBASE,5);   
         
      } 
    
    
    
        /* Have we been signaled to shut down? */
        if(signals & SIGBREAKF_CTRL_F){
          bug("PPP process: received SIGBREAKF_CTRL_F\n");
          break;
        }
        
       
      BOOL More = TRUE; 
      while( More ){
            
         More = FALSE;
         
         if(ios2 = (struct IOSana2Req *)GetMsg((struct MsgPort *)LIBBASE->sd_Unit)){
                More = TRUE;
                PerformIO(LIBBASE,ios2);
         }
          
         if( LIBBASE->serial_ok ){ 
            if(ioser = (struct IOExtSer *)GetMsg(LIBBASE->sdu_RxPort))    {    
               More = TRUE;
               CMD_READ_Ready(LIBBASE,ioser);   
            }
      
            if(ioser = (struct IOExtSer *)GetMsg(LIBBASE->sdu_TxPort)){
               More = TRUE;
               CMD_WRITE_Ready(LIBBASE);
            }   
         } 
             
       }
    }
    
    
  }
    
    bug("PPP process: shut everything down..\n");
    
    
    CloseSerial(LIBBASE);
    
    if( LIBBASE->TimeReq ){
       AbortIO((struct IORequest *)LIBBASE->TimeReq);
       WaitIO((struct IORequest *)LIBBASE->TimeReq);
       while(GetMsg(LIBBASE->TimeMsg));
       CloseDevice((struct IORequest *)LIBBASE->TimeReq);
    } 
    if(LIBBASE->TimeReq)
      DeleteIORequest(LIBBASE->TimeReq);
    if(LIBBASE->TimeMsg)
      DeleteMsgPort(LIBBASE->TimeMsg);

    
    
    if(signalbit)  FreeSignal(signalbit);
    
    bug("PPP process: shut down OK\n");
    LIBBASE->sdu_Proc_run = FALSE;
}




struct PPP_DevUnit *InitPPPUnit(LIBBASETYPEPTR LIBBASE,ULONG s2unit)
{
   
    struct PPP_DevUnit *sdu;
     D(bug("InitPPPUnit\n"));
   
  if(!LIBBASE->sd_Unit){
    
        /* Allocate a new Unit structure */
     if(sdu = AllocMem(sizeof(struct Unit), MEMF_CLEAR|MEMF_PUBLIC)){ 
        
        LIBBASE->sd_Unit = (struct Unit *)sdu;
                 
        ReadConfig(LIBBASE);
        
        /* Do some initialization on the Unit structure */

        NEWLIST(&LIBBASE->sd_Unit->unit_MsgPort.mp_MsgList);
        LIBBASE->sd_Unit->unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
        LIBBASE->sd_Unit->unit_MsgPort.mp_Flags = PA_IGNORE;
        LIBBASE->sd_Unit->unit_MsgPort.mp_Node.ln_Name = "PPP";


            LIBBASE->sdu_Proc_run = FALSE;
            
             D(bug("New process:\n"));
            if(LIBBASE->sdu_Proc = CreateNewProcTags(
                                                NP_Entry, PPP_Process,
                                                NP_Name, "PPP process",
                                                NP_Synchronous , FALSE,
                                                NP_Priority, 1,
                                                NP_UserData, LIBBASE,
                                                NP_StackSize, 30000,
                                                TAG_DONE))
                                                
            {

            D(bug("wait..\n"));
            while( ! LIBBASE->sdu_Proc_run ) Delay(5);
            D(bug("...ok\n")); 
            
            }else{
                 D(bug("New process:FAILL !!!\n"));
            }   
       
        }

        if(!LIBBASE->sdu_Proc){
            /* The Unit process couldn't start for some reason, so free the Unit structure. */
            FreeMem(sdu,sizeof(struct Unit));
            LIBBASE->sd_Unit = NULL;
        }   
       
    }
    return((struct PPP_DevUnit *)LIBBASE->sd_Unit);
}







static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
D(bug("[PPP] Init()\n"));

    InitSemaphore(&LIBBASE->sd_Lock);   
        
    LIBBASE->serial_ok  = FALSE;  
    LIBBASE->device_up  = TRUE;  // hmmm... why this is needed?
    LIBBASE->ppp_online = FALSE;  
    
    LIBBASE->sdu_SerTx  = NULL;
    LIBBASE->sdu_SerRx  = NULL;
    LIBBASE->sdu_TxPort = NULL; 
    LIBBASE->sdu_RxPort = NULL;
    LIBBASE->sdu_TxBuff = NULL;
    LIBBASE->sdu_RxBuff = NULL;
    
    if( ! (LIBBASE->sdu_TxBuff = AllocMem( PPP_MAXBUFF ,MEMF_CLEAR|MEMF_PUBLIC))){      
       return FALSE;   
    }       
    if( ! (LIBBASE->sdu_RxBuff = AllocMem( PPP_MAXBUFF ,MEMF_CLEAR|MEMF_PUBLIC))){   
       return FALSE;   
    }       
  
     return TRUE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
    
D(bug("[PPP] Expunge()\n"));
  
    return FALSE;
   /* 
    if(LIBBASE->sd_OpenCnt)
    {
    // Sorry, we're busy.  We'll expunge later on if we can. 
       LIBBASE->sd_Flags |= LIBF_DELEXP;
       D(bug("[PPP] Expunge,busy\n"));
       return FALSE;
    }
    return TRUE;   
	*/ 
}




static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req* req,
    ULONG unitnum,
    ULONG flags
)
{
    struct PPP_DevUnit *sdu;
    struct TagItem *bufftag;

    BOOL status = FALSE;
    
    D(bug("[PPP] Open unit %d\n",unitnum));
     
    if (req->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req)){
        bug("[PPP] ERROR wrong ios2_Req lenght\n");
        return FALSE;
    }
                
        
    ObtainSemaphore(&LIBBASE->sd_Lock);      
                          
    LIBBASE->sd_OpenCnt++;   

    if(unitnum == 0 ){  
      if(sdu = InitPPPUnit(LIBBASE,unitnum)){        
        if(bufftag = FindTagItem(S2_CopyToBuff, (struct TagItem *)req->ios2_BufferManagement)){   
          LIBBASE->CopyToBuffer =  (APTR)bufftag->ti_Data;
          if(bufftag = FindTagItem(S2_CopyFromBuff, (struct TagItem *)req->ios2_BufferManagement)){  
             LIBBASE->CopyFromBuffer =  (APTR)bufftag->ti_Data;
              
             status = TRUE;
             LIBBASE->sd_OpenCnt++;
             LIBBASE->sd_Flags &=~LIBF_DELEXP;
             LIBBASE->sd_Unit->unit_OpenCnt++;
            
             //req->ios2_BufferManagement = (VOID *)bm;
             req->ios2_Req.io_Error = 0;
             req->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
             req->ios2_Req.io_Unit =(APTR)sdu;
             req->ios2_Req.io_Device =(APTR)LIBBASE;                   
           }
        }    
      }
    }
    
    /* See if something went wrong. */
    if(!status)
    {   
      req->ios2_Req.io_Error = IOERR_OPENFAIL;
      req->ios2_Req.io_Unit = (struct Unit *) -1;
      req->ios2_Req.io_Device = (struct Device *) -1;
      LIBBASE->sd_OpenCnt--;
    }
    
    ReleaseSemaphore(&LIBBASE->sd_Lock);
    
    return( status );
}




VOID ExpungeUnit(LIBBASETYPEPTR LIBBASE)
{
   
    D(bug("[PPP] ExpungeUnit \n"));
    
    D(bug("[PPP] ExpungeUnit Signal\n"));
    Signal( LIBBASE->sdu_Proc , SIGBREAKF_CTRL_F ); 
    D(bug("[PPP] ExpungeUnit Wait\n")); 
    while(  LIBBASE->sdu_Proc_run ) Delay(5);
    
    D(bug("[PPP] ExpungeUnit FreeMem\n"));
    LIBBASE->sd_Unit = NULL;
    FreeMem(LIBBASE->sd_Unit, sizeof(struct Unit)); 
    D(bug("[PPP] ExpungeUnit ok\n"));
}



static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req* req
)
{
 
  //  BPTR seglist = 0L;
    D(bug("[PPP] Close\n"));
    ObtainSemaphore(&LIBBASE->sd_Lock);
    
    CloseSerial(LIBBASE);
    

    /* Trash the io_Device and io_Unit fields so that any attempt to use this
       request will die immediatly. */

    req->ios2_Req.io_Device = (struct Device *) -1;
    req->ios2_Req.io_Unit = (struct Unit *) -1;

    /* I always shut the unit process down if the open count drops to zero.
       That way, if I need to expunge, I never have to Wait(). */

    LIBBASE->sd_Unit->unit_OpenCnt--;
    
    if(!LIBBASE->sd_Unit->unit_OpenCnt){
       ExpungeUnit(LIBBASE);
    }

    LIBBASE->sd_OpenCnt--;
    ReleaseSemaphore(&LIBBASE->sd_Lock);

    /* Check to see if we've been asked to expunge. */
    //if(LIBBASE->sd_Flags & LIBF_DELEXP)
      // seglist = DevExpunge(LIBBASE);
    D(bug("[PPP] Close OK\n"));
    return TRUE;
}


/*
** This funcion is used to locate an IO request in a linked
** list and abort it if found.
*/
ULONG AbortReq(LIBBASETYPEPTR LIBBASE,struct MinList *minlist, struct IOSana2Req *ios2)
{
    struct Node *node, *next;
    ULONG result=IOERR_NOCMD;

    node = (struct Node *)minlist->mlh_Head;

    while(node->ln_Succ){
      next = node->ln_Succ;
      if(node == (struct Node *)ios2){
        Remove((struct Node *)ios2);
        ios2->ios2_Req.io_Error = IOERR_ABORTED;
        TermIO(LIBBASE,ios2);
        result = 0;
      }
      node = next;
    }
    return(result);
}



AROS_LH1(void, beginio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, PPPDev)
{
    AROS_LIBFUNC_INIT

   if(  ( ! LIBBASE->sdu_RxPort ) &&  LIBBASE->sdu_TxPort ){ // PPP_process is busy because openserial wait and wait...
       req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
       req->ios2_WireError = S2WERR_UNIT_OFFLINE;
       TermIO(LIBBASE,req);
   }else{
       req->ios2_Req.io_Flags &= ~IOF_QUICK;
       PutMsg(  (struct MsgPort *) req->ios2_Req.io_Unit , (struct Message *)req );
    }
    
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, PPPDev)
{
   AROS_LIBFUNC_INIT
   ULONG result = 0L;

D(bug("[PPP] AbortIO()\n"));

   Disable();
   if(req->ios2_Req.io_Message.mn_Node.ln_Type != NT_REPLYMSG){
    switch(req->ios2_Req.io_Command){
        case CMD_READ:  result=AbortReq(LIBBASE,&LIBBASE->sdu_Rx,req);
                        break;

        case CMD_WRITE: result=AbortReq(LIBBASE,&LIBBASE->sdu_Tx,req);
                        break;

        default:        result=IOERR_NOCMD;
                        break;
      }
    }
    Enable();
    return 0;
    AROS_LIBFUNC_EXIT
}



ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)











