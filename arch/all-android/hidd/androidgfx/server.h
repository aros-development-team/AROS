#define cmd_Nak	       -1
#define cmd_Query	1

#define STATUS_ACK	0
#define STATUS_NAK	1

/*
 * Requests are generally linear data blocks.
 * Data starting from 'cmd' is sent to the server. The reply will be
 * stored directly after sent portion.
 * Lengths are determined by command value using hardcoded tables.
 */

struct Request
{
    struct MinNode  node;
    struct Task    *owner;
    ULONG	    signal;	/* Reply signal		*/
    ULONG	    status;	/* Delivery status	*/
    ULONG	    cmd;	/* Command		*/
    ULONG	    len;	/* Number of parameters */
    /* ULONG parameters follow */
};

struct QueryRequest
{
    /* Request (8 bytes, including cmd) */
    struct Request  req;	/* cmd_Query, 1				*/
    ULONG	    id;		/* Display ID, currently reserved	*/
    /* Response (8 bytes) */
    ULONG	    width;
    ULONG	    height;
};

void agfxInt(int pipe, int mode, void *data);
void DoRequest(struct Request *req, struct agfx_staticdata *xsd);
