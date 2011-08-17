#define cmd_Nak	       -1
#define cmd_Query      0x80000001
#define cmd_Show       0x80000002
#define cmd_Update     0x00000003
#define cmd_Scroll     0x00000004

#define CMD_NEED_REPLY 0x80000000

#define STATUS_ACK	0
#define STATUS_NAK	1

/*
 * Display server data packets consist of ULONGs. This makes it simpler to parse them in Java code.
 *
 * Yes, this is not 64-bit-safe. But:
 * a) Android isn't going to move to 64-bit machines for now.
 * b) Java doesn't have "pointer-sized" data type, it has either 32 bits (int)
 *    or 64 bits (long). And we don't want to cope with UQUADs here.
 * If support for 64 bits is ever needed, perhaps the best way is to make 64-bit
 * versions of some commands (which require a pointer), which use two ULONGs for it.
 * For this purpose we store addresses at the end of packets. It will be possible to determine
 * 64-bit versions just by increased length.
 */

/*
 * This is raw packet header.
 * It is used for both requests and responses.
 */
struct Request
{
    ULONG cmd;			/* Command code		*/
    ULONG len;			/* Number of parameters	*/
    /* ULONG parameters follow */
};

/* This structure is used to query a request which needs a response.*/
struct WaitRequest
{
    struct MinNode  node;
    struct Task    *owner;
    ULONG	    signal;	/* Reply signal		*/
    ULONG	    status;	/* Delivery status	*/
    /* Encapsulated packet header */
    ULONG	    cmd;
    ULONG	    len;
};

/* Partucilar forms of command requests */

/* Query display size */
struct QueryRequest
{
    struct WaitRequest req;		/* cmd_Query, 1				*/
    /* Request parameters */
    ULONG	       id;		/* Display ID, currently always zero	*/
    /* Response data (8 bytes) */
    ULONG	       width;
    ULONG	       height;
};

/* Show a single bitmap on display */
struct ShowRequest
{
    struct WaitRequest req;		/* cmd_Show, 7				*/
    /* Request */
    ULONG	       displayid;	/* DisplayID, currently always zero	*/
    ULONG	       left;		/* Offset				*/
    ULONG	       top;
    ULONG	       width;		/* Size					*/
    ULONG	       height;
    ULONG	       mod;		/* Bytes per line			*/
    ULONG	       addr;		/* Start address			*/
    /* Response has no data, only echo */
};

/*
 * The following requests don't use WaitRequest because they don't need to be replied.
 * They are used with SendRequest() function.
 */

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
};

/* Scroll a bitmap */
struct ScrollRequest
{
    struct Request req;		/* cmd_Scroll, 3			*/
    /* Request */
    ULONG	   id;		/* Bitmap ID, reserved			*/
    ULONG	   left;	/* New offset				*/
    ULONG	   top;
};

void agfxInt(int pipe, int mode, void *data);
void SendRequest(struct Request *req, struct agfx_staticdata *xsd);
void DoRequest(struct WaitRequest *req, struct agfx_staticdata *xsd);
