#include <stdio.h>
#include <string.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <devices/serial.h>

void doall(void);

struct IOExtSer * IORequests[10];
struct MsgPort * SerPort;

int main(int argc, char **argv)
{
  int i;
  while (i < 10)
  {
    IORequests[i++] = NULL;
  }  
  SerPort = CreatePort("mySerial",0);
  doall();
  DeletePort(SerPort);
  return 1;
}


int getFreeIORequest()
{
  int i = 0;
  while (i <= 9)
  {
    if (NULL == IORequests[i])
      return i;
    i++;
  }
  return -1;
}

void closedevices(void)
{
  int i = 0;
  while (i <= 9)
  { 
    if (NULL != IORequests[i])
      CloseDevice((struct IORequest *)IORequests[i]);
    i++;
  }  
}

void opendevice(void)
{
  int unitnum;
  char sevenwire, shared;
  int index = getFreeIORequest();
  int flags;
  int err;
  
  if (-1 == index)
  {
    printf("No more device to open.\n");
    return;
  }
  
  IORequests[index] = (struct IOExtSer *) 
     CreateExtIO(SerPort, sizeof(struct IOExtSer));
  
  printf("Open serial device.\n");
  printf("Unitnumber: ");
  scanf("%d", &unitnum);
  printf("shared access (y/n):");
  scanf("%c", &shared);
  printf("seven wire (y/n):");
  scanf("%c", &sevenwire);
  
  if (shared == 'y' ||
      shared == 'Y')
    IORequests[index] -> io_SerFlags |= SERF_SHARED;
    
  if (sevenwire == 'y' ||
      sevenwire == 'Y')
    IORequests[index] -> io_SerFlags |= SERF_7WIRE;
    
  err = OpenDevice("serial.device",unitnum,(struct IORequest *)IORequests[index],flags);
  
  if (0 != err)
  {
    printf("Failed to open unit %i of serial device.\n",unitnum);
    DeleteExtIO((struct IORequest *)IORequests[index]);
    IORequests[index]=NULL;
  }   
  else
  {
    printf("Created unit %i of serial device. Refer to it with number %i\n",unitnum,index);
  }
      
}

void doall(void)
{
  char buf[80];

  for (;;) 
  {
    printf("> ");
    fflush(stdout);
    scanf("%s", buf);
  
    if (!strcmp(buf,"quit")) 
    {
      closedevices();
      return;
    }
    else if (!strcmp(buf, "help"))
    {
      printf("quit help opendevice [od] \n");
    }
    else if (!strcmp(buf, "opendevice") || !strcmp(buf, "od"))
    {
      opendevice(); 
    }
  }

}