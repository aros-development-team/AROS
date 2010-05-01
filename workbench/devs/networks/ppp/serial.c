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
#include <ctype.h>
#include <devices/serial.h>
#include <devices/timer.h>

#include "ppp.h"
#include "device_protos.h"
#include LC_LIBDEFS_FILE


BOOL SendStr(LIBBASETYPEPTR LIBBASE,const STRPTR str ,LONG timeout)
{ 
   struct MsgPort *Tprt =LIBBASE->TimeMsg;
   BOOL result = TRUE;
   ULONG sigset;
   ULONG recvd;
   
   if( ! LIBBASE->serial_ok ) return FALSE;
   
    SetTimer(LIBBASE,timeout);  
   
    LIBBASE->sdu_SerTx->IOSer.io_Length = strlen((char*) str);
    LIBBASE->sdu_SerTx->IOSer.io_Data = str;
    LIBBASE->sdu_SerTx->IOSer.io_Command =  CMD_WRITE;               
    SendIO((struct IORequest *)LIBBASE->sdu_SerTx); 
    //bug("SendStr \"%s\"\n",str);                                  
    sigset = (1L << Tprt->mp_SigBit) |  
             (1L<< LIBBASE->sdu_TxPort->mp_SigBit );               
    recvd = Wait(sigset);    
    
    if( GetMsg( LIBBASE->TimeMsg ) ){  
        bug("SendStr \"%s\" timeout\n",str); 
        result = FALSE;     
    }                                           
           
   SetTimer(LIBBASE,2); 
   //bug("SendStr clean\n",str);  
   if( LIBBASE->sdu_SerTx ){  
      AbortIO((struct IORequest *)LIBBASE->sdu_SerTx);
      WaitIO((struct IORequest *)LIBBASE->sdu_SerTx);
      while(GetMsg(LIBBASE->sdu_TxPort));
   }   
   //bug("SendStr ok\n",str);   
  return result;    
}



VOID DoStr(LIBBASETYPEPTR LIBBASE,const STRPTR str)
{ 
    
  if( ! LIBBASE->serial_ok  ) return;  
    
  LIBBASE->sdu_SerTx->IOSer.io_Length = strlen((char*) str);
  LIBBASE->sdu_SerTx->IOSer.io_Data = str;
  LIBBASE->sdu_SerTx->IOSer.io_Command =  CMD_WRITE;    
  DoIO((struct IORequest*)LIBBASE->sdu_SerTx);  
      
}

void DoBYTE(LIBBASETYPEPTR LIBBASE,const BYTE b)
{   
  BYTE mb = b;  
  
   if( ! LIBBASE->serial_ok ) return;  
       
  LIBBASE->sdu_SerTx->IOSer.io_Length = 1;
  LIBBASE->sdu_SerTx->IOSer.io_Data = &mb;
  LIBBASE->sdu_SerTx->IOSer.io_Command =  CMD_WRITE;    
  DoIO((struct IORequest*)LIBBASE->sdu_SerTx);                 
}


void DoBYTES(LIBBASETYPEPTR LIBBASE, BYTE *p,ULONG len)
{   
    
  if( ! LIBBASE->serial_ok ) return;  
     
  LIBBASE->sdu_SerTx->IOSer.io_Length = len;
  LIBBASE->sdu_SerTx->IOSer.io_Data = p;
  LIBBASE->sdu_SerTx->IOSer.io_Command =  CMD_WRITE;    
  DoIO((struct IORequest*)LIBBASE->sdu_SerTx);                 
}


void SendBYTES(LIBBASETYPEPTR LIBBASE, BYTE *p, ULONG len)
{   
   
  if( ! LIBBASE->serial_ok ) return;  
       
  LIBBASE->sdu_SerTx->IOSer.io_Length = len;
  LIBBASE->sdu_SerTx->IOSer.io_Data = p;
  LIBBASE->sdu_SerTx->IOSer.io_Command =  CMD_WRITE;    
  SendIO((struct IORequest*)LIBBASE->sdu_SerTx);                 
}


BYTE ReadBYTE(LIBBASETYPEPTR LIBBASE)
{ 
  BYTE b;  
   
  if( ! LIBBASE->serial_ok ) return 0; 
  
  LIBBASE->sdu_SerRx->IOSer.io_Length = 1;
  LIBBASE->sdu_SerRx->IOSer.io_Data = &b;
  LIBBASE->sdu_SerRx->IOSer.io_Command =  CMD_READ;    
  DoIO((struct IORequest*)LIBBASE->sdu_SerRx);    
  return b;            
}




#define MAXSBUFFER 200
BOOL WaitStr(LIBBASETYPEPTR LIBBASE,const STRPTR str, LONG timeout)
{ 
    
    struct MsgPort *Tprt =LIBBASE->TimeMsg;
    BYTE Buffer[MAXSBUFFER];
    ULONG sigset;
    BOOL result = FALSE;
    BYTE c[2];
    
    ULONG len = 0;
    Buffer[0]=0;
    c[1]=0;
    ULONG recvd;
    
   if( ! LIBBASE->serial_ok ) return FALSE;
   
   SetTimer(LIBBASE,timeout);  
                 
  // QueueSerRequest(LIBBASE);
   
    LIBBASE->sdu_SerRx->IOSer.io_Command = CMD_READ;
    LIBBASE->sdu_SerRx->IOSer.io_Data = LIBBASE->sdu_RxBuff;
    LIBBASE->sdu_SerRx->IOSer.io_Length = 1 ;
    SendIO((struct IORequest *)LIBBASE->sdu_SerRx); 
                      
 //  bug( "Waiting \"%s\"...\n",str );            
   for(;;){    
                     
            // bug( "testing \"%s\" \"%s\"\n",Buffer,str ); 

      if ( strcasestr(Buffer,str) != NULL ){ 
            result = TRUE;  
            break;
       }
                          
       sigset = (1L << Tprt->mp_SigBit) |
                (1L<< LIBBASE->sdu_RxPort->mp_SigBit );
                   
       recvd = Wait(sigset);
              
           if( GetMsg( LIBBASE->sdu_RxPort ) ){ 
              if( len <= ( MAXSBUFFER-1 ) ){
                  Buffer[len] = LIBBASE->sdu_RxBuff[0];
                  len++;
                  Buffer[len] = 0;   
                  bug( "%s", &Buffer[len-1] );  
              } 
                 LIBBASE->sdu_SerRx->IOSer.io_Command = CMD_READ;
                 LIBBASE->sdu_SerRx->IOSer.io_Data = LIBBASE->sdu_RxBuff;
                 LIBBASE->sdu_SerRx->IOSer.io_Length = 1 ;
                 SendIO((struct IORequest *)LIBBASE->sdu_SerRx); 
          } 
      
          if( GetMsg( LIBBASE->TimeMsg ) ){   
            result = FALSE;
            break;  
          }   
                                            
   }        
   SetTimer(LIBBASE,0); 
   
   
    if( LIBBASE->sdu_SerRx ){    
        AbortIO((struct IORequest *)LIBBASE->sdu_SerRx);     
        WaitIO((struct IORequest *)LIBBASE->sdu_SerRx); 
        while(GetMsg(LIBBASE->sdu_RxPort));
     } 
   
   return result;               
}



void SerDelay(LIBBASETYPEPTR LIBBASE, LONG timeout)
{ 
    
    struct MsgPort *Tprt =LIBBASE->TimeMsg;
    ULONG sigset;
    ULONG recvd;
    
   if( ! LIBBASE->serial_ok ) return ;
   
    SetTimer(LIBBASE,timeout);  
                 
    LIBBASE->sdu_SerRx->IOSer.io_Command = CMD_READ;
    LIBBASE->sdu_SerRx->IOSer.io_Data = LIBBASE->sdu_RxBuff;
    LIBBASE->sdu_SerRx->IOSer.io_Length = 1 ;
    SendIO((struct IORequest *)LIBBASE->sdu_SerRx); 
                         
   for(;;){    
                                          
       sigset = (1L << Tprt->mp_SigBit) |
                (1L<< LIBBASE->sdu_RxPort->mp_SigBit );        
       recvd = Wait(sigset);
              
           if( GetMsg( LIBBASE->sdu_RxPort ) ){ 
               
                 LIBBASE->sdu_RxBuff[1] = 0;
                 bug( "%s",  LIBBASE->sdu_RxBuff );   
                        
                 LIBBASE->sdu_SerRx->IOSer.io_Command = CMD_READ;
                 LIBBASE->sdu_SerRx->IOSer.io_Data = LIBBASE->sdu_RxBuff;
                 LIBBASE->sdu_SerRx->IOSer.io_Length = 1 ;
                 SendIO((struct IORequest *)LIBBASE->sdu_SerRx); 
           } 
       
          if( GetMsg( LIBBASE->TimeMsg ) ){  
            break;  
          }   
                                            
   }        
   SetTimer(LIBBASE,0); 
   
   
    if( LIBBASE->sdu_SerRx ){    
        AbortIO((struct IORequest *)LIBBASE->sdu_SerRx);     
        WaitIO((struct IORequest *)LIBBASE->sdu_SerRx); 
        while(GetMsg(LIBBASE->sdu_RxPort));
     } 
                   
}




BOOL OpenSerial(LIBBASETYPEPTR LIBBASE)
{
  
   
    
    D(bug("OpenSerial\n"));
     
    if( ! (LIBBASE->sdu_TxPort = CreateMsgPort())){
       goto OpenSerial_fail;
    }
    
    if( ! (LIBBASE->sdu_SerTx = CreateIORequest(LIBBASE->sdu_TxPort,sizeof(struct IOExtSer)))){
       goto OpenSerial_fail;
    }  
        
    D(bug("OpenDevice: \"%s\" unit %d\n",LIBBASE->DeviceName,LIBBASE->SerUnitNum));
    if(  OpenDevice( LIBBASE->DeviceName , LIBBASE->SerUnitNum , (struct IORequest *)LIBBASE->sdu_SerTx,0)){
       LIBBASE->sdu_SerTx = NULL;
       goto OpenSerial_fail;
    }
        
     LIBBASE->sdu_SerTx->IOSer.io_Command = CMD_RESET;    
     D(bug("  CMD_RESET  --> Tx\n"));
     if( DoIO((struct IORequest *)LIBBASE->sdu_SerTx)){ 
        goto OpenSerial_fail;
     }  
     
     LIBBASE->sdu_SerTx->IOSer.io_Command = CMD_CLEAR;    
     D(bug("  CMD_CLEAR  --> Tx\n"));
     if( DoIO((struct IORequest *)LIBBASE->sdu_SerTx)){ 
        goto OpenSerial_fail;
     }  
     
     // Set up our serial parameters 
     LIBBASE->sdu_SerTx->IOSer.io_Command = SDCMD_SETPARAMS;
    // LIBBASE->sdu_SerTx->io_Baud = 9600;
    // LIBBASE->sdu_SerTx->io_RBufLen = 1500L;
     //LIBBASE->sdu_SerTx->io_ReadLen = 8;
     //LIBBASE->sdu_SerTx->io_WriteLen = 8;
     //LIBBASE->sdu_SerTx->io_StopBits = 1;
     LIBBASE->sdu_SerTx->io_SerFlags = SERF_XDISABLED | SERF_RAD_BOOGIE;
        
     D(bug("  SDCMD_SETPARAMS  --> Tx\n"));
     if( DoIO((struct IORequest *)LIBBASE->sdu_SerTx)){ 
        goto OpenSerial_fail;
     }  
    
    if( ! (LIBBASE->sdu_RxPort = CreateMsgPort()) ){ 
      goto OpenSerial_fail;
    }   
       
    if( ! (LIBBASE->sdu_SerRx = CreateIORequest(LIBBASE->sdu_RxPort,sizeof(struct IOExtSer)))){
      goto OpenSerial_fail;
    }
            
    LIBBASE->sdu_SerRx->IOSer.io_Device = LIBBASE->sdu_SerTx->IOSer.io_Device;
    LIBBASE->sdu_SerRx->IOSer.io_Unit   = LIBBASE->sdu_SerTx->IOSer.io_Unit;
     
    LIBBASE->sdu_SerRx->IOSer.io_Command = SDCMD_QUERY;
    D(bug("SDCMD_QUERY  ---> Rx\n"));
    if( DoIO((struct IORequest *)LIBBASE->sdu_SerRx)){  
       goto OpenSerial_fail;
    }   
   
    D(bug("OpenSerial OK\n")); 
    LIBBASE->serial_ok = TRUE;

    return TRUE;
   
    OpenSerial_fail:
      D(bug("OpenSerial FAIL !!\n")); 
      
      CloseSerial(LIBBASE);
      
      return FALSE;
}



VOID CloseSerial(LIBBASETYPEPTR LIBBASE)
{
  
  D(bug("CloseSerial\n"));

    if( LIBBASE->sdu_SerRx ){
      D(bug("  clearing Rx\n"));     
      AbortIO((struct IORequest *)LIBBASE->sdu_SerRx);  
      WaitIO((struct IORequest *)LIBBASE->sdu_SerRx);
      while(GetMsg(LIBBASE->sdu_RxPort));      
    }
    
    if( LIBBASE->sdu_SerTx ){  
      D(bug("  clearing Tx\n"));
      AbortIO((struct IORequest *)LIBBASE->sdu_SerTx);
      WaitIO((struct IORequest *)LIBBASE->sdu_SerTx);
      while(GetMsg(LIBBASE->sdu_TxPort));
    } 

  D(bug("  CloseDevice\n")); 

    if(LIBBASE->sdu_SerTx)
      CloseDevice((struct IORequest *)LIBBASE->sdu_SerTx);
    
    if(LIBBASE->sdu_SerTx)
      DeleteIORequest(LIBBASE->sdu_SerTx);

    if(LIBBASE->sdu_TxPort)
      DeleteMsgPort(LIBBASE->sdu_TxPort);

    if(LIBBASE->sdu_SerRx)
      DeleteIORequest(LIBBASE->sdu_SerRx);

    if(LIBBASE->sdu_RxPort)
      DeleteMsgPort(LIBBASE->sdu_RxPort);
    
    LIBBASE->serial_ok = FALSE;  
    LIBBASE->sdu_SerTx  = NULL;
    LIBBASE->sdu_SerRx  = NULL;
    LIBBASE->sdu_TxPort = NULL; 
    LIBBASE->sdu_RxPort = NULL;
       
    D(bug("CloseSerial OK!\n")); 
}
 

VOID QueueSerRequest(LIBBASETYPEPTR LIBBASE)
{

    if( ! ( LIBBASE->serial_ok ) ) return;
    
    LIBBASE->sdu_SerRx->IOSer.io_Command = SDCMD_QUERY;
    DoIO((struct IORequest *)LIBBASE->sdu_SerRx);
   
    if( LIBBASE->sdu_SerRx->IOSer.io_Error )
    {
       
       bug("OOOOPS  We've lost carrier.! ");
       CloseSerial(LIBBASE);
       return;
    }
         
    LIBBASE->sdu_SerRx->IOSer.io_Command = CMD_READ;
    LIBBASE->sdu_SerRx->IOSer.io_Data = LIBBASE->sdu_RxBuff;
    LIBBASE->sdu_SerRx->IOSer.io_Length = LIBBASE->sdu_SerRx->IOSer.io_Actual ;
    
    if( LIBBASE->sdu_SerRx->IOSer.io_Length > PPP_MAXBUFF ) 
                   LIBBASE->sdu_SerRx->IOSer.io_Length = PPP_MAXBUFF;
    /* If the number of bytes available is zero, queue a request
       for one byte. */
    if(!LIBBASE->sdu_SerRx->IOSer.io_Length)
        LIBBASE->sdu_SerRx->IOSer.io_Length = 1;
    
    SendIO((struct IORequest *)LIBBASE->sdu_SerRx);
    
}







