/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.

    Desc: The functions to read lines from a file
*/

int fileopen(const char *); /* Open a file for read */
void fileclose(void); /* Close the opened file */
char *readline(void); /* Read a line from the opened file */
void exitfileerror(int code, const char *format, ...); /* Print exit code prefixed with filename and lineno */
