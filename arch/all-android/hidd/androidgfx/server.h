#define PIPE_NAME "AROS.display"

#define cmd_Query	0x00000001
#define cmd_Shutdown	0x80D1ED1E

/*
 * Requests are generally linear data blocks.
 * Data starting from 'cmd' is sent to the server. The reply will be
 * stored directly after sent portion.
 * Lengths are determined by command value using hardcoded tables.
 */

struct Request
{
    struct Message  msg;
    ULONG	    signal;	/* Reply signal */
    ULONG	    cmd;	/* Command	*/
};

struct QueryRequest
{
    /* Request (8 bytes, including cmd) */
    struct Request  req;	/* cmd_Query				*/
    ULONG	    id;		/* Display ID, currently reserved	*/
    /* Response (8 bytes) */
    ULONG	    width;
    ULONG	    height;
};

void agfxTask(int pipe, struct agfx_staticdata *xsd);
void DoRequest(struct Request *req, struct agfx_staticdata *xsd);
