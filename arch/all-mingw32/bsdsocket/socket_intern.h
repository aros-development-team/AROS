struct Socket
{
    struct MinNode n;	  /* Node		   */
    unsigned int   s;	  /* WinSock socket number */
    ULONG	   flags; /* Flags, see below	   */
};

#define SOF_NBIO 0x0001 /* Non-blocking mode */
