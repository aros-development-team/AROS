/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>
#include <dos/var.h>

/*****************************************************************************

    NAME */

      AROS_LH3(STRPTR, Getenv,

/*  SYNOPSIS */ 
      AROS_LHA(char *, string , A0),
      AROS_LHA(char *, buffer , A1),
      AROS_LHA(ULONG , size   , D0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 47, Arp)

/*  NAME
        GetEnv -- Get the value of an environment variable
 
    FUNCTION
        This function provides an environment variable mechanism
        compatible with MANX.  Note that this call is more efficient
        than using the manx getenv() function call when arplibrary is
        installed.  
 
    INPUTS
        string -- pointer to an environment variable name.
        buffer -- a user allocated area which will be used to store
                  the value associated with the environment variable.
                  This may be NULL.
        size -- size of buffer region in bytes.
 
    RESULT
        If found, a pointer to the start of the value string for that
        variable will be returned.
 
        If buffer is non-null, then as much of the value string as
        allowed by size will be copied into the user buffer.
 
 
 
    ADDTIONAL CONSIDERATIONS
        MANX was the first to implement environment variables on the
        Amiga.  As a result, we are trying to be compatible with their
        implementation.  Note that this function is NOT equivalent to
        MANX getenv() call, but can easily be constructed using this
        this function.  Note also that all calls by MANX will be handled
        by arplibrary when it is installed.
 
        You would be wise to run as Forbidden all code which examines the
        environment space of the arp.library (for example, using the
        return value of this function).  As an example, the following code
        performs identically to the manx getenv() function:
 
            char getenv(variable)
                 char *variable;
                 {
                        char *temp;
                        static char *buffer;
 
                        free(buffer);   // Free memory from last call 
                        Forbid();
                        temp = Getenv(name, 0L, 0L);
                        if (!temp)
                            Permit(), return(0);
                        buffer = malloc(strlen(temp));
                        strcpy(buffer, temp);
                        Permit();
                        return (buffer);
                   }
 
    BUGS
        If size is not large enough, the complete value of that
        environment variable will not be copied to the user buffer.
 

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  /* Did the user pass me a buffer??*/
  if (NULL != buffer)
  {
    /* look for it as a GLOBAL = environment variable */
    GetVar(string, buffer, size, GVF_GLOBAL_ONLY);
  }
  else /* buffer == NULL */
  { /* What are we going to do if he didn't pass me a buffer?? */
    /* I should definitely not look for the variable myself in the 
       dos structures and should not establish my own collection 
       of variables. I could allocate memory, let dos look for the 
       variable and write the code into my buffer and start 
       collection those buffers. But that's not good either! */
    
  }
  
  return buffer;  
    
  AROS_LIBFUNC_EXIT
} /* Getenv */
