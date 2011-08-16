#define cmd_Nak	       -1
#define cmd_Query	1
#define cmd_Show	2
#define cmd_Update	3
#define cmd_Scroll	4

#define STATUS_ACK	0
#define STATUS_NAK	1

/*
 * Requests are generally linear data blocks consisting of ULONGs.
 * This makes it simpler to parse them in Java code.
 * Data starting from 'cmd' is sent to the server. The reply will be
 * stored directly after sent portion.
 * Reply length is determined by command value using hardcoded table.
 *
 * Yes, this is not 64-bit-safe. But:
 * a) Android isn't going to move to 64-bit machines for now.
 * b) Java doesn't have "pointer-sized" data type, it has either 32 bits (int)
 *    or 64 bits (long). And we don't want to cope with UQUADs here.
 * If support for 64 bits is ever needed, perhaps the best way is to make 64-bit
 * versions of some commands (which require a pointer), which use two ULONGs for it.
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

/* Query display size */
struct QueryRequest
{
    struct Request req;		/* cmd_Query, 1				*/
    /* Request parameters */
    ULONG	   id;		/* Display ID, currently always zero	*/
    /* Response data (8 bytes) */
    ULONG	   width;
    ULONG	   height;
};

/* Show a single bitmap on display */
struct ShowRequest
{
    struct Request req;		/* cmd_Show, 7				*/
    /* Request */
    ULONG	   displayid;	/* DisplayID, currently always zero	*/
    ULONG	   left;	/* Offset				*/
    ULONG	   top;
    ULONG	   width;	/* Size					*/
    ULONG	   height;
    ULONG	   mod;		/* Bytes per line			*/
    ULONG	   addr;	/* Start address			*/
    /* Response has no data, only echo */
};

/* Update screen region */
struct UpdateRequest
{
    struct Request req;		/* cmd_Update, 5			*/
    /* Request */
    ULONG	   id;		/* Bitmap ID, reserved			*/
    ULONG	   left;	/* Rectangle to update			*/
    ULONG	   top;
    ULONG	   right;
    ULONG	   bottom;
    /* No response needed */
};

struct ScrollRequest
{
    struct Request req;		/* cmd_Scroll, 3			*/
    /* Request */
    ULONG	   id;		/* Bitmap ID, reserved			*/
    ULONG	   left;	/* New offset				*/
    ULONG	   top;
    /* No response needed */
};

void agfxInt(int pipe, int mode, void *data);
void DoRequest(struct Request *req, struct agfx_staticdata *xsd);
