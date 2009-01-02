/* $Id$
 *
 *      lineread.c - functions to read lines from sockets effectively
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 *      Copyright © 2005 Pavel Fedin
 */

/*
 * since charRead is a macro package and having no `.c' -file, it's
 * documentation is added here
 */

/****** net.lib/charRead ****************************************************
 
    NAME
	charRead -- read characters from socket one by one.
 
    SYNOPSIS
	initCharRead(rc, fd)
 
	void initCharRead(struct CharRead *, int);
 
 
	character = charRead(rc)
 
	int charRead(struct CharRead *);
 
 
    DESCRIPTION
	charRead is a macro package which return characters one by one 
	from given socket input stream. The socket where data is to be read
	is set by calling initCharRead(): rc is the pointer to charread
	structure previously allocated. fd is the (socket) descriptor where
	reading is to be done.
 
	charRead() returns the next character from input stream or one of
	the following:
 
	RC_DO_SELECT    (-3)    - read input buffer is returned. Do select
				  before next call if you don't want charread
				  to block.
 
	RC_EOF          (-2)    - end-of-file condition has occurred.
 
	RC_ERROR        (-1)    - there has been an error while filling new
				  charread buffer. Check the value of Errno()
 
    NOTE
	Always use variable of type int to store return value from charRead()
	since the numeric value of characters returned may vary between
	0 -255 (or even greater). As you may know, -3 equals 253 if of type
	unsigned char.
 
    EXAMPLE
	\*
	 * This piece of code shows how to use charread with select()
	 *\
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <charread.h>
 
	main_loop(int sock)
	{
	  struct CharRead rc;
	  fd_set readfds;
	  int c;
 
	  initCharRead(&rc, sock);
 
	  FD_ZERO(&readfds);
 
	  while(1) {
	    FD_SET(sock, &readfds);     
 
	    if (select(sock + 1. &readfds, NULL, NULL, NULL)) < 0) {
	      perror("select");
	      break;
	    }
	    if (FD_ISSET(sock, &readfds)) {
	      while((c = charRead(&rc)) >= 0)
		handle_next_input_character(c);
	      if (c == RC_EOF)
		break;
	      if (c == RC_ERROR) {
		perror("charRead");
		break;
	      }
	    }
	  }
	}
 
     PORTABILITY
	The source file charread.h should be able to be used in 
	UNIX programs as is.
 
     SEE ALSO
	lineRead(), bsdsocket.library/recv()
*****************************************************************************
*
*/


/****** net.lib/lineRead *****************************************************

    NAME
        lineRead -- read newline terminated strings from socket

    SYNOPSIS
        initLineRead(rl, fd, lftype, bufsize)

        void initLineRead(struct LineRead *, int, int, int);


        length = lineRead(rl)

        int lineread(struct LineRead *);


    FUNCTION
        lineRead() reads newline terminated strings from given descriptor
        very efficiently. All the options needed are set by calling
        initLineRead(): rl is the pointer to lineread structure previously
        allocated. fd is the (socket) descriptor where reading is to be
        done. lftype can have following 3 values:

            RL_LFNOTREQ - Newline terminated strings are returned unless
                          there is no newlines left in currently buffered
                          input. In this case remaining buffer is returned.

            RL_LFREQLF  - If there is no newlines left in currently buffered
                          input the remaining input data is copied at the
                          start of buffer. Caller is informed that next
                          call will fill the buffer (and it may block).
                          Lines are always returned with newline at the end
                          unless the string is longer than whole buffer.

            RL_LFREQNUL  - Like LF_REQLF, but remaining newline is removed.
                          Note here that lenght is one longer that actual
                          string length since line that has only one
                          newline at the end would return length as 0
                          which indigate string incomplete condition.

        bufsize is used to tell lineread how big the receive buffer is.
        always put RL_BUFSIZE here since that value is used to determine
        the memory allocated for the buffer. This option is given to you
        so you may decide to use different buffer size than the default
        1024.

        lineRead() returns the newline terminated string in rl_line field
        of lineread structure. Return values of lineRead() are:

             1 - RL_BUFSIZE     - normal length of returned string.

             0                  - If zero is returned just after select(),
                                  end-of-file condition has occurred.
                                  Otherwise string is not completed yet.
                                  Make sure you call select() (or use non-
                                  blocking IO) if you don't want next call
                                  to block.

            -1                  - if rl_Line field of lineread structure
                                  is NULL, it indicates error condition.
                                  If rl_Line points to start of string
                                  buffer, input string has been longer
                                  than buffer. In this case rl_Line points
                                  to zero terminated string of length
                                  RL_BUFSIZE.

        You may modify the zero terminated string returned by lineRead() in
        any way, but memory around the string is private lineread memory.

    EXAMPLE
        \*
         * The following code shows how to use lineread with select()
         *\
        #ifdef USE_LOW_MEMORY_BUFFER
        #define RL_BUFSIZE 256
        #endif

        #include <sys/types.h>
        #ifdef AMIGA
        #include <bsdsocket.h>
        #endif
        #include <lineread.h>

        #define NULL 0

        ...

        main_loop(int sock)
        {
          struct LineRead * rl;
          int length;
          fd_set reafdfs;

          if (rl = (struct LineRead *)AllocMem(sizeof (*rl), 0)) {

            initLineRead(rl, sock, LF_REQLF, RL_BUFSIZE);

            FD_ZERO(&readfds);

            while(1) {
              FD_SET(sock, &readfds);

              if (select(sock + 1, &readfds, NULL, NULL, NULL)) < 0) {
                perror("select");
                break;
              }
              if (FD_ISSET(sock, &readfds))
                if ((length = lineRead(rl)) == 0) \* EOF *\
                  break;
                do {
                  if (length > 0)
                    write(1, rl->rl_Line, length); \* stdout. write() for *\
                                                   \* speed demonstration *\
                  else { \* length == -1 *\
                    if (rl->rl_Line == NULL); {
                      perror("lineRead");
                      break;
                    }
                    else {
                      fprintf(stderr, "lineread input buffer overflow!\n");
                      write(1, rl->rl_Line, RL_BUFSIZE);
                      write(1, "\n", 1);
                    }
                  }
                } while ((length = lineRead(rl)) != 0); \* 0 -> do select() *\
            }
          FreeMem(rl, sizeof (*rl);
          }
          else
            fprintf("AllocMem: Out Of memory\n");
        }

     PORTABILITY
        The source modules lineread.c and lineread.h should compile
        in UNIX machines as is.

     SEE ALSO
        charRead(), bsdsocket.library/recv()

******************************************************************************
*/

#include "lineread.h"

#ifdef AMIGA
extern struct Library * SocketBase;
#define READ(a, b, c) recv(a, b, c, 0)
#include <proto/socket.h>
#else /* not AMIGA */
#define READ(a, b, c) read(a, b, c)
#endif /* AMIGA */

#ifndef NULL
#define NULL 0
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef __CONCAT
#if defined (__STDC__) || defined (__cplusplus)
#define __CONCAT(x,y) x ## y
#else
#define __CONCAT(x,y) x/**/y
#endif
#endif /* __CONCAT not defined */  
  
#define RLP(field) __CONCAT(rl->rl_Private.rlp_,field)


#if defined (__STDC__) || defined (__cplusplus)

int lineRead(struct LineRead * rl)

#else

int lineRead(rl)
     struct LineRead * rl;

#endif
{
  int i;
  
  if (RLP(Bufpointer) == RLP(Howlong))

    if (RLP(Selected)) {

      if (RLP(Line_completed))
	RLP(Startp) = RLP(Bufpointer) = 0;

      if ((i = READ(rl->rl_Fd,
		    RLP(Buffer) + RLP(Bufpointer),
		    RLP(Buffersize) - RLP(Bufpointer))) <= 0) {
	/*
	 * here if end-of-file or on error. set Howlong == Bufpointer
	 * so if non-blocking I/O is in use next call will go to READ()
	 */
	RLP(Howlong) = RLP(Bufpointer);
	rl->rl_Line = NULL;			
	return i;
      }
      else
	RLP(Howlong) = RLP(Bufpointer) + i;
    }
    else /* Inform user that next call may block (unless select()ed) */ 
      {
	RLP(Selected) = TRUE;
	return 0;
      }
  else /* Bufpointer has not reached Howlong yet. */
    {
      RLP(Buffer)[RLP(Bufpointer)] = RLP(Saved);
      RLP(Startp) = RLP(Bufpointer);
    }

  /*
   * Scan read string for next newline.
   */
  while (RLP(Bufpointer) < RLP(Howlong))
    if (RLP(Buffer)[RLP(Bufpointer)++] == '\n')
      goto Skip;

  /*
   * Here if Bufpointer == Howlong.
   */
  if (rl->rl_Lftype != RL_LFNOTREQ) {
    RLP(Selected) = TRUE;

    if (RLP(Bufpointer) == RLP(Buffersize)) {
      /*
       * Here if Bufpointer reaches end-of-buffer.
       */
      if (RLP(Startp) == 0) { /* (buffer too short for whole string) */
	RLP(Line_completed) = TRUE;
	rl->rl_Line = RLP(Buffer);
	RLP(Buffer)[RLP(Bufpointer)] = '\0';
	return -1;
      }
      /*
       * Copy partial string to start-of-buffer and make control ready for
       * filling rest of buffer when next call to lineRead() is made
       * (perhaps after select()).
       */
      for (i = 0; i < RLP(Buffersize) - RLP(Startp); i++)
	RLP(Buffer)[i] = RLP(Buffer)[RLP(Startp) + i];
      RLP(Howlong)-= RLP(Startp);
      RLP(Bufpointer) = RLP(Howlong);
      RLP(Startp) = 0;
    }
    
    RLP(Line_completed) = FALSE;
    return 0;
  }

 Skip:
  RLP(Line_completed) = TRUE;
  if (rl->rl_Lftype == RL_LFREQNUL)
    RLP(Buffer)[RLP(Bufpointer) - 1] = '\0';
  RLP(Saved) = RLP(Buffer)[RLP(Bufpointer)];
  RLP(Buffer)[RLP(Bufpointer)] = '\0';
  RLP(Selected) = FALSE;
  rl->rl_Line = RLP(Buffer) + RLP(Startp);

  return (RLP(Bufpointer) - RLP(Startp));
}

#undef READ	  

#if defined (__STDC__) || defined (__cplusplus)
	  
void initLineRead(struct LineRead * rl, int fd, int lftype, int buffersize)
	  
#else

int initLineRead(rl, fd, lftype, buffersize)
      struct LineRead * rl;
      int fd;
      int lftype;
      int buffersize;
  
#endif
{	  
  rl->rl_Fd     = fd;
  rl->rl_Lftype = lftype;

  RLP(Bufpointer) = RLP(Howlong) = 0;
  RLP(Selected) = RLP(Line_completed) = TRUE;

  RLP(Buffersize) = buffersize;
}
