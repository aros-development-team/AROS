#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define TTYNAME "/dev/cua0"

/* Amiga specific values */
#define TRUE 1
#define FALSE 0
#define BOOL int

/*
  This HIDD should also set up the Unix input device in such a fashion,
  that whenever a certain amount of bytes have come in, an interrupt is
  executed and the Amiga Interrupt handler will call a function to
  handle the incoming data. 
*/


/*
  tty_init()
  Tries to open "/dev/cua0" for I/O. 
  
  A PC has more than one available serial port. So the port to be opened
  should be an argument to this function.
  On failure this function will return 0. 
*/

int tty_open()
{
  int fd; 
  speed_t         speed;
  struct termios  tty_new;

  /* open the serial port */
  if ((fd = open(TTYNAME, O_RDWR)) < 0) 
    return 0;
  return fd;
}

/* 
  Set the parameters on the Unix serial port.
  Inputs:
    fd           filehandle
    speed        Baudrate
    DataLength   5,6,7 or 8
    StopBits     16 (for 1 stopbit), 32 (for 2 stopbits)
*/

BOOL set_serial_parameters(int fd, 
                           speed_t speed,
                           int DataLength,
                           int StopBits)
{
  struct termios  tty;
  int cflag = 0;

  /* character size mask */
  switch (DataLength)
  {
    case 5: cflag = CS5; 
      break;
    case 6: cflag = CS6;
      break;
    case 7: cflag = CS7;
      break;
    case 9: cflag = CS8;
      break;
    default:
      /* anything else is simply wrong. */
      return FALSE;
  }
  
  /* check StopBits */
  switch(StopBits)
  {
    case 16: 
      break;
    case 32: cflag |= CSTOPB;
      break;
    default:
      return FALSE;
  }
  
  /* get current parameters */
  tcgetattr(fd, &tty);
  
  /* set a few standard values */
  cfmakeraw(&tty);
  tty.c_lflag |= ISIG;
  
  /* set the speed */
  cfsetspeed(&tty, speed);

  /* Set DataLength and  StopBits */
  tty.c_cflag |= cflag;

  /* make the changes effective immediately */
  if (tcsetattr(fd, TCSANOW, &tty) < 0)
    return FALSE;
        
  return TRUE;
}

/*
  Send data to the serial port.
  Inputs:
      fd          filehandle
      void * buf  pointer to buffer
      count       number of bytes to send
      
  Returns TRUE if everything went alright,
          FALSE otherwise 
*/

BOOL send_data(int fd,
               const void *buf,
               size_t count)
{
  int success = write(fd, buf, count);
  if (-1 != success)
    return TRUE;
  else
    return FALSE;
}

/*
  Read data from the serial port.
  Inputs:
      fd          filehandle
      void * buf  pointer to buffer
      count       maximum number of bytes to read
      

  Returns a number >= 0 for the actual number of bytes that were read.
  If this number is -1, however, an error occurred reading from the
  serial port.
*/


int read_data(int fd,
              void *buf,
              size_t count)
{
  return read(fd, buf, count);
}