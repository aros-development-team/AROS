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

void open_device(void)
{
  ULONG unitnum;
  char sevenwire, shared;
  int index = getFreeIORequest();
  ULONG flags = 0;
  BYTE err;
  
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


void close_device(void)
{
  int index;
  printf("Close a serial device.\n");
  printf("Referencenumber: ");
  scanf("%d", &index);
  
  if (index >= 0 && index <= 9 && NULL != IORequests[index])
  {
    printf("Closing device!\n");
    CloseDevice((struct IORequest *)IORequests[index]);
    DeleteExtIO((struct IORequest *)IORequests[index]);
    IORequests[index]=NULL;
  }
  else
  {
    printf("No such refence.\n");
  }
}

void write_to_device(void)
{
  int index;
  printf("Write to serial device.\n");
  printf("Referncenumber: ");
  scanf("%d", &index);
  
  if (index >= 0 && index <= 9 && NULL != IORequests[index])
  {
    char buffer[100];
    memset(buffer,0,100);
    IORequests[index]->IOSer.io_Command = CMD_WRITE;
    IORequests[index]->IOSer.io_Flags = 0;
    IORequests[index]->IOSer.io_Length = -1;
    IORequests[index]->IOSer.io_Data = buffer;

    printf("Enter string to transmit!\n");
    scanf("%s",buffer);
    DoIO((struct IORequest *)IORequests[index]);
  }
  else
  {
    printf("No such refence.\n");
  }
}

void read_from_device(void)
{
  int index;
  printf("Read from serial device.\n");
  printf("Referncenumber: ");
  scanf("%d", &index);

  if (index >= 0 && index <= 9 && NULL != IORequests[index])
  {
    int len;
    char buffer[100];
    memset(buffer,0,100);
    printf("Read how many bytes [1-99]: ");
    scanf("%d",&len);
    if (len <= 0) len = 1;
    if (len > 99) len = 99;
    printf("Reading %d bytes from device!\n",len);    
    IORequests[index]->IOSer.io_Command = CMD_READ;
    IORequests[index]->IOSer.io_Flags = IOF_QUICK;
    IORequests[index]->IOSer.io_Length = len;
    IORequests[index]->IOSer.io_Data = buffer;

    DoIO((struct IORequest *)IORequests[index]);
    printf("Received the following string: %s\n",buffer);
  }
  else
  {
    printf("No such refence.\n");
  }
}


void query(void)
{
  int index;
  printf("Query a serial device.\n");
  printf("Referncenumber: ");
  scanf("%d", &index);
  
  if (index >= 0 && index <= 9 && NULL != IORequests[index])
  {
    IORequests[index]->IOSer.io_Command = SDCMD_QUERY;

    DoIO((struct IORequest *)IORequests[index]);
    printf("Status bits: 0x%x\n",IORequests[index]->io_Status);
    printf("Number of bytes in buffer: %d\n",(int)IORequests[index]->IOSer.io_Actual);
  }
  else
  {
    printf("No such refence.\n");
  }
}

void stop(void)
{
  int index;
  printf("Stop/pause IO on a serial device.\n");
  printf("Referncenumber: ");
  scanf("%d", &index);
  
  if (index >= 0 && index <= 9 && NULL != IORequests[index])
  {
    IORequests[index]->IOSer.io_Command = CMD_STOP;

    DoIO((struct IORequest *)IORequests[index]);
    printf("IO has been stopped!\n");
  }
  else
  {
    printf("No such refence.\n");
  }
}

void start(void)
{
  int index;
  printf("Start/resume IO on a serial device.\n");
  printf("Referncenumber: ");
  scanf("%d", &index);
  
  if (index >= 0 && index <= 9 && NULL != IORequests[index])
  {
    IORequests[index]->IOSer.io_Command = CMD_START;

    DoIO((struct IORequest *)IORequests[index]);
    printf("IO has started!\n");
  }
  else
  {
    printf("No such refence.\n");
  }
}


void set_parameters(void)
{
  int index;
  printf("Set parameters on a serial device.\n");
  printf("Referncenumber: ");
  scanf("%d", &index);
  
  if (index >= 0 && index <= 9 && NULL != IORequests[index])
  {
    int baudrate;
    IORequests[index]->IOSer.io_Command = SDCMD_SETPARAMS;

    printf("New baudrate: ");
    scanf("%d",&baudrate);
    IORequests[index]->io_Baud = baudrate;

    DoIO((struct IORequest *)IORequests[index]);
    if (((struct IORequest *)IORequests[index])->io_Error != 0)
    {
      printf("An error occured while setting the parameters!\n");
    }
  }
  else
  {
    printf("No such refence.\n");
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
      printf("quit help open_device [od] close_device [cd] write_to_device [wd]\n");
      printf("read_from_device [rd] query [q] set_parameters [sp] stop [pa]\nstart [st]\n");
    }
    else if (!strcmp(buf, "open_device") || !strcmp(buf, "od"))
    {
      open_device(); 
    }
    else if (!strcmp(buf, "close_device") || !strcmp(buf, "cd"))
    {
      close_device(); 
    }
    else if (!strcmp(buf, "write_to_device") || !strcmp(buf, "wd"))
    {
      write_to_device();
    }
    else if (!strcmp(buf, "read_from_device") || !strcmp(buf, "rd"))
    {
      read_from_device();
    }
    else if (!strcmp(buf, "query") || !strcmp(buf, "q"))
    {
      query();
    }
    else if (!strcmp(buf, "set_parameters") || !strcmp(buf, "sp"))
    {
      set_parameters();
    }
    else if (!strcmp(buf, "stop") || !strcmp(buf, "pa"))
    {
      stop();
    }
    else if (!strcmp(buf, "start") || !strcmp(buf, "st"))
    {
      start();
    }
  }
}
