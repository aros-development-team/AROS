#include <stdio.h>
#include <string.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <devices/serial.h>
#include <exec/execbase.h>

#include <stdlib.h>

void doall(void);
VOID do_auto(struct MsgPort * prt, ULONG unitnum, ULONG baudrate, ULONG delay);

struct IOExtSer * IORequests[10];
struct MsgPort * SerPort;


#define ARG_TEMPLATE "AUTO/S,BAUD/K,UNIT/K,PAUSE/K"

enum
{
	ARG_AUTO = 0,
	ARG_BAUD,
	ARG_UNIT,
	ARG_PAUSE,
	NOOFARGS
};

int main(int argc, char **argv)
{
	int i = 0;
	IPTR args[NOOFARGS] = {(IPTR)FALSE,
	                       (IPTR)NULL,
	                       (IPTR)NULL,
	                       (IPTR)NULL};
	struct RDArgs * rda;
	rda = ReadArgs(ARG_TEMPLATE, args, NULL);
	
	if (NULL != rda) {
		while (i < 10) {
			IORequests[i++] = NULL;
		}	
		SerPort = CreatePort("mySerial",0);
		if (TRUE == args[ARG_AUTO]) {
			ULONG unitnum = 0;
			ULONG baudrate = 9600;
			ULONG delay = 5;
			if (NULL != (APTR) args[ARG_UNIT])
				unitnum = atoi((CONST_STRPTR)args[ARG_UNIT]);
			if (NULL != (APTR) args[ARG_BAUD])
				baudrate = atoi((CONST_STRPTR)args[ARG_BAUD]);
			if (NULL != (APTR) args[ARG_PAUSE])
				delay = atoi((CONST_STRPTR)args[ARG_PAUSE]);
			do_auto(SerPort, 
			        unitnum, 
			        baudrate,
			        delay);
		} else {
			doall();
		}
		DeletePort(SerPort);
		FreeArgs(rda);
	}	
	return 1;
}

VOID do_auto(struct MsgPort * prt, ULONG unitnum, ULONG baudrate, ULONG delay) 
{
	ULONG err;
	IORequests[0] = (struct IOExtSer *) 
		 CreateExtIO(SerPort, sizeof(struct IOExtSer));
	
	printf("Opening unit %ld. Using baudrate %ld baud.\n",
	       unitnum,
	       baudrate);
	
	err = OpenDevice("serial.device",unitnum,(struct IORequest *)IORequests[0],0);
	
	if (0 != err) {
		printf("Failed to open unit %ld of serial device.\n",unitnum);
		DeleteExtIO((struct IORequest *)IORequests[0]);
		IORequests[0]=NULL;
	} else {
		printf("Opened device. Now waiting for %ld seconds.\n",delay);
		if (0 != delay) {
			Delay(50*delay);
		}
		IORequests[0]->IOSer.io_Command = SDCMD_SETPARAMS;

		IORequests[0]->io_Baud = baudrate;

		DoIO((struct IORequest *)IORequests[0]);
		if (0 != ((struct IORequest *)IORequests[0])->io_Error) {
			printf("An error occured while setting the baudrate!\n");
		} else {
			ULONG len;
			char buffer[] = "Hello, this is AROS's serial device.\n";
			printf("Writing to serial device.\n");
			IORequests[0]->IOSer.io_Command = CMD_WRITE;
			IORequests[0]->IOSer.io_Flags = 0;
			IORequests[0]->IOSer.io_Length = -1;
			IORequests[0]->IOSer.io_Data = buffer;

			DoIO((struct IORequest *)IORequests[0]);
			printf("Now please enter something! Waiting for %ld seconds!\n",delay);
			if (0 != delay) {
				Delay(50 * delay);
			}

			IORequests[0]->IOSer.io_Command = SDCMD_QUERY;

			DoIO((struct IORequest *)IORequests[0]);
			printf("Status bits: 0x%x\n",IORequests[0]->io_Status);
			printf("Number of bytes in buffer: %d\n",(int)IORequests[0]->IOSer.io_Actual);

			if (0 != (len = (int)IORequests[0]->IOSer.io_Actual)) {
				len = (len < sizeof(buffer) - 1) 
				      ? len 
				      : sizeof(buffer)-1;
				IORequests[0]->IOSer.io_Command = CMD_READ;
				IORequests[0]->IOSer.io_Flags = IOF_QUICK;
				IORequests[0]->IOSer.io_Length = len;
				IORequests[0]->IOSer.io_Data = buffer;
				
				DoIO((struct IORequest *)IORequests[0]);
				buffer[len] = 0;
				printf("Received the following string: %s\n",buffer);
			}
			printf("Ending now.\n");
		}
		CloseDevice((struct IORequest *)IORequests[0]);
	}	
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
	scanf("%ld", &unitnum);
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
		printf("Failed to open unit %ld of serial device.\n",unitnum);
		DeleteExtIO((struct IORequest *)IORequests[index]);
		IORequests[index]=NULL;
	}	 
	else
	{
		printf("Created unit %ld of serial device. Refer to it with number %d\n",unitnum,index);
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
