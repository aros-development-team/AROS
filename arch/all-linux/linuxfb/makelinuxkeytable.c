/* $XConsortium: xev.c,v 1.15 94/04/17 20:45:20 keith Exp $ */
/*

Copyright (c) 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

/*
 * Author:  Jim Fulton, MIT X Consortium
 */

/***************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <limits.h>

#define FALSE 0
#define TRUE 1

void cleanup(int);
void cleanup_sighandling();
void setup_sighandling();
void cleanup_kbd();
int init_kbd();
void cleanup(int);

				
/***************************************************************************/


static char *ProgramName;
static FILE *fh;

static unsigned char table[256];

/***************************************************************************/

#define CODE_NONE	0xFF
#define CODE_ESCAPE	0x01
static struct _asktable
{
    char *title;
    int rawkeystart;
    int numkeys;
    unsigned char nokey_code;
} asktable [] =
{
    {"1st row: Press ESCAPE Key", 0x45, 1, },
    {"1st row: Press F1 .. F10", 0x50, 10, CODE_NONE},
    {"1st row: Press F11 (or ESCAPE if key doesn't exist)", 0x4B, 1, CODE_ESCAPE},
    {"1st row: Press F12 (or ESCAPE if key doesn't exist)", 0x6F, 1, CODE_ESCAPE},

    {"2nd row: Press the key left of 1, or ESCAPE if there's no key left of 1", 0x00, 1, CODE_ESCAPE},
    {"2nd row: Press 1 .. 0", 0x1, 10, CODE_NONE},
    {"2nd row: Press the three normal (!!) keys right of 0. Press ESCAPE instead of third key if you only have two keys right of 0", 0X0B, 3, CODE_ESCAPE},
    {"2nd row: Press BACKSPACE", 0x41, 1, CODE_NONE},

    {"3rd row: Press TAB", 0x42, 1, CODE_NONE},
    {"3rd row: Press the twelve keys right of TAB (= upto and excl. RETURN)", 0x10, 12, CODE_NONE},
    {"3rd row: Press RETURN", 0x44, 1, CODE_NONE},

    {"4th row: Press the (left) CONTROL key which might also be in 6th row", 0x63, 1, CODE_NONE},
    {"4th row: Press CAPS LOCK", 0x62, 1, CODE_NONE},
    {"4th row: Press the twelve keys right of CAPS LOCK. Press ESCAPE for last if you only have eleven keys there", 0x20, 12, CODE_ESCAPE},

    {"5th row: Press LEFT SHIFT", 0x60, 1, CODE_NONE},
    {"5th row: If in this row you have eleven normal keys, press the key right of LEFT SHIFT, otherwise ESCAPE", 0x30, 1, CODE_ESCAPE},
    {"5th row: Press the next ten keys in this row (= upto and excl. RIGHT SHIFT)", 0x31, 10, CODE_NONE},
    {"5th row: Press RIGHT SHIFT", 0x61, 1, CODE_NONE},

    {"6th row: Press left ALT", 0x64, 1, CODE_NONE},
    {"6th row: Press left AMIGA", 0x66, 1, CODE_NONE},
    {"6th row: Press SPACE", 0x40, 1, CODE_NONE},
    {"6th row: Press right AMIGA", 0x67, 1, CODE_NONE},
    {"6th row: Press right ALT", 0x65, 1, CODE_NONE},
    {"6th row: Press right CONTROL (or ESCAPE if key doesn't exist)", 0x63, 1, CODE_NONE},

    {"Above cursor keys: Press INSERT (or ESCAPE if key doesn't exist)", 0x47, 1, CODE_ESCAPE},
    {"Above cursor keys: Press HOME (or ESCAPE if key doesn't exist)", 0x70, 1, CODE_ESCAPE},
    {"Above cursor keys: Press PAGE UP (or ESCAPE if key doesn't exist)", 0x48, 1, CODE_ESCAPE},
    {"Above cursor keys: Press DELETE", 0x46, 1, CODE_NONE},
    {"Above cursor keys: Press END (or ESCAPE if key doesn't exist)", 0x71, 1, CODE_ESCAPE},
    {"Above cursor keys: Press PAGE DOWN (or ESCAPE if key doesn't exist)", 0x49, 1, CODE_ESCAPE},
    {"Above cursor keys: Press HELP (or key you want to use for HELP if it doesn't exist)", 0x5F, 1, CODE_NONE},
    
    {"Cursor keys: Press CURSOR UP", 0x4C, 1, CODE_NONE},
    {"Cursor keys: Press CURSOR LEFT", 0x4F, 1, CODE_NONE},
    {"Cursor keys: Press CURSOR DOWN", 0x4D, 1, CODE_NONE},
    {"Cursor keys: Press CURSOR RIGHT", 0x4E, 1, CODE_NONE},

    {"Numeric pad 1st row: Press the four keys above (!!!!) 7 8 9 (Or ESCAPE if keys do not exist)", 0x5A, 4, CODE_ESCAPE},
    {"Numeric pad 2nd row: Press 7 8 9 (Or ESCAPE if keys do not exist)", 0x3D, 3, CODE_ESCAPE},
    {"Numeric pad 2nd row: If key right of 9 is a normal size key press it, otherwise ESCAPE", 0x4A, 1, CODE_ESCAPE},
    {"Numeric pad 3rd row: Press 4 5 6 (Or ESCAPE if keys do not exist)", 0x2D, 3, CODE_ESCAPE},
    {"Numeric pad 3rd row: Press + (Or ESCAPE if key does not exist)", 0x5e, 1, CODE_ESCAPE},
    {"Numeric pad 4th row: Press 1 2 3 (Or ESCAPE if keys do not exist)", 0x1D, 3, CODE_ESCAPE},
    {"Numeric pad 5th row: Press 0 (Or ESCAPE if key does not exist)", 0x0F, 1, CODE_ESCAPE},
    {"Numeric pad 5th row: Press COMMA (Or ESCAPE if key does not exist)", 0x3C, 1, CODE_ESCAPE},
    {"Numeric pad 5th row: Press ENTER (Or ESCAPE if key does not exist)", 0x43, 1, CODE_ESCAPE},
    {0, 0, 0, CODE_NONE}
};

/***************************************************************************/

void doaskuser(char *title,int rawkeystart, int numkeys, unsigned char sc);

/***************************************************************************/

void usage ()
{
    static char *msg[] = {
"     -o <filename>                      Table output file name",
"",
NULL};
    char **cpp;

    fprintf (stderr, "usage:  %s [-options ...]\n", ProgramName);
    fprintf (stderr, "where options include:\n");

    for (cpp = msg; *cpp; cpp++) {
	fprintf (stderr, "%s\n", *cpp);
    }

    cleanup(1);
}


/***************************************************************************/

int main (int argc, char **argv)
{
    char *tablefilename = NULL;
    int i;
    char *name = "Linux Scancode to Rawkey table generation tool";
    struct _asktable *ask;

    ProgramName = argv[0];
    for (i = 1; i < argc; i++) {
	char *arg = argv[i];

	if (arg[0] == '-') {
	    switch (arg[1]) {	
	      case 'o':			/* table file name */
	        if (++i >= argc) usage ();
		tablefilename = argv[i];
		continue;
		
	      default:
		usage ();
	    }				/* end switch on - */
	} else 
	  usage ();
    }					/* end for over argc */

    if (!tablefilename)
    {
        fprintf (stderr, "%s: output filename missing\n",
		 ProgramName);
	cleanup(1);
    }

    printf("%s\n", name);    
    
    memset(table, 0xFF, 256);
    
    if (!init_kbd()) {
	fprintf(stderr, "Could not init keyboard\n");
	cleanup(1);
    }
    
        
    for(ask = asktable; ask->title; ask++)
    {
        doaskuser(ask->title, ask->rawkeystart, ask->numkeys, ask->nokey_code);
    }
      
    fh = fopen(tablefilename,"wb");
    if (!fh)
    {
	fprintf(stderr, "%s:  unable to open \"%s\" in write mode\n",
		 ProgramName, tablefilename);
		 
	cleanup(1);
    }
    
    i = fwrite(table, 1, 256, fh);
    if (i != 256)
    {
	fprintf (stderr, "%s:  writing to \"%s\" failed\n",
		 ProgramName, tablefilename);
	cleanup(1);
    }
    
    fclose(fh);
    fh = 0;

    cleanup(0);
    
    return 0;
}

/***************************************************************************/

static int kbdfd;
static unsigned char lastcode = 0xFF;

void doaskuser(char *title,int rawkeystart, int numkeys, unsigned char actcode)
{
    int i;
    
    printf("%s\n", title);
    

    for(i = 0;i < numkeys;)
    {
/*	printf("IN OUTER LOOP, i=%d, numkeys=%d, lastcode=0x%x\n", i, numkeys, lastcode);
*/	for (;;) {
	    int bytesread;
	    unsigned char code;
	    int exitloop = FALSE;
	    
	    
	    bytesread = read(kbdfd, &code, 1);
	    if (code != lastcode && code < 0x80) {
/*		printf("Got key down: 0x%x, actcode=0x%x, lastocode: 0x%x\n"
			, code, actcode, lastcode);
*/		if (actcode == CODE_NONE || actcode != code) {
		    printf("SETTING IN TABLE\n");
		    table[code] = rawkeystart + i;
		    exitloop = TRUE;
		}
		if (actcode == CODE_ESCAPE && actcode == code)
		    exitloop = TRUE;
		
	    }
/*	    if (code >= 0x80)
		printf("Got key up: 0x%x\n", code);
*/	    
	    lastcode = code;
/*	    printf("SETING LASTCODE 0x%x AND EXITING LOOP\n", lastcode);
*/	    if (exitloop)
		break;
	}
	i ++;
	
    } /* for(i = 0;i < numkeys;) */
}



int set_kbd_mode(int fd, int mode, int *oldmode)
{
    /* Get and preserve the old kbd mode */
    if (NULL != oldmode) {
	if (-1 == ioctl(fd, KDGKBMODE, oldmode)) {
	    fprintf(stderr, "Unable to get old kbd mode: %s\n", strerror(errno));
	    return 0;
	}
    }
    
    /* Set the new mode */
    if (-1 == ioctl(fd, KDSKBMODE, mode)) {
	fprintf(stderr, "Unable to set new kbd mode: %s\n", strerror(errno));
	return 0;
    }
    
    return 1;
}


static int oldkbdmode;
static struct termios oldtio;
static struct kbd_repeat repeat;

int   mode_done	   = FALSE
    , fd_done	   = FALSE
    , termios_done = FALSE
    , repeat_done  = FALSE;
    
#define KBD_DEVNAME "/dev/console"

int init_kbd()
{
    int ret = TRUE;
printf("INIT_KBD\n");
    
    kbdfd = open(KBD_DEVNAME, O_RDONLY);
    if (-1 == kbdfd) {
	fprintf(stderr, "!!! Could not open keyboard device: %s\n", strerror(errno));
	ret = FALSE;
    } else {
	/* Try to read some data from the keyboard */
	struct termios newtio;
	
	fd_done = TRUE;
	

	setup_sighandling();

printf("SIGNALS SETUP\n");	
	if ( (-1 == tcgetattr(kbdfd, &oldtio)) || (-1 == tcgetattr(kbdfd, &newtio))) {
	    fprintf(stderr, "!!! Could not get old termios attrs: %s\n", strerror(errno));
	    ret = FALSE;
	} else {
	    /* Set some new attrs */
	    newtio.c_lflag = ~(ICANON | ECHO | ISIG);
	    newtio.c_iflag = 0;
	    newtio.c_cc[VMIN] = 1;
	    newtio.c_cc[VTIME] = 0;
	    
	    if (-1 == tcsetattr(kbdfd, TCSAFLUSH, &newtio)) {
		fprintf(stderr, "!!! Could not set new termio: %s\n", strerror(errno));
		ret = FALSE;
	    } else {
	    	termios_done = TRUE;
printf("SET TERMIO ATTRS\n");
		if (!set_kbd_mode(kbdfd, K_MEDIUMRAW, &oldkbdmode)) {
		    fprintf(stderr, "!!! Could not set kbdmode\n");
		    ret = FALSE;
		} else {
printf("KBD MODE SET\n");
		    mode_done = TRUE;
		    /* Set keyboard repeat */
		    repeat.delay = 100000;
		    repeat.rate  = 100000;
		    if (-1 == ioctl(kbdfd, KDKBDREP, &repeat)) {
			fprintf(stderr, "!!!! Could not set keyboard repeat: %s\n", strerror(errno));
		    } else {
			repeat_done = TRUE;
			ret = TRUE;
		    }
		}

	    } /* if (termios attrs set) */
	} /*  if (got old termios attrs) */
    }
    
    if (!ret) {
    	cleanup_kbd();
    }

    return ret;
    
}

void cleanup_kbd()
{
    if (repeat_done)
	ioctl(kbdfd, KDKBDREP, &repeat);
    /* Reset the kbd mode */
    if (mode_done)
	set_kbd_mode(kbdfd, oldkbdmode, NULL);

    if (termios_done)
	tcsetattr(kbdfd, TCSAFLUSH, &oldtio);
   
    if (fd_done)
    	close(kbdfd);
	   
    cleanup_sighandling();
    
    return;
}

const int signals[] = {
	SIGHUP, SIGINT,	SIGQUIT, SIGILL,
	SIGTRAP, SIGBUS, SIGFPE, SIGKILL,
	SIGALRM, SIGSEGV , SIGTERM
};


void exit_sighandler(int sig)
{
    printf("PARENT EXITING VIA SIGHANDLER\n");
    cleanup(0);
}

void kbdsighandler(int sig)
{
    cleanup_kbd();
}

/* Avoid that some signal kills us without resetting the keyboard */
void setup_sighandling(void)
{
    unsigned int i;
    
    for (i = 0; i < sizeof (signals); i ++) {
    	signal(signals[i], kbdsighandler);
    }
    
/*    signal(SIGALRM, exit_sighandler);
    alarm(200);
*/    
}

void cleanup_sighandling()
{
    unsigned int i;
    for (i = 0; i < sizeof (signals); i ++) {
	signal(signals[i], SIG_DFL);
    }	
}

void cleanup(int exitcode)
{
    cleanup_kbd();
    exit (0);
}
