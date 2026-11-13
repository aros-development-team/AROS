#define cmd_Nak	       -1
#define cmd_Query      0x80000001
#define cmd_Show       0x80000002
#define cmd_Update     0x00000003
#define cmd_Scroll     0x00000004
#define cmd_Mouse      0x00000005
#define cmd_Touch      0x00000006
#define cmd_Key	       0x00000007
#define cmd_Flush      0x00000008
#define cmd_Hello      0x80000009

#define CMD_NEED_REPLY 0x80000000

#define STATUS_ACK	0
#define STATUS_NAK	1

#define PROTOCOL_VERSION 2	/* Current protocol version */

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
 *
 * The general conventions are:
 * - We send commands and get responses and event notifications from the server.
 * - Some of commands need responses from the server. We (and server) just know what commands these are.
 *   CMD_NEED_REPLY flag exists just for simplicity. We can't just add it to some command.
 * - We never respond somehow on event notifications.
 * - If the server doesn't recognize some command, it will always send back NAK reply. It's up to us to
 *   ignore this NAK if we don't wait for any response.
 * - The first command sent to server must be cmd_Hello. This way we verify protocol version. The server
 *   will respond with NAK if it doesn't understand us.
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

/* Query display size (v2) */
struct QueryRequest
{
    struct WaitRequest req;		/* cmd_Query, 1				*/
    /* Request parameters */
    ULONG	       id;		/* Display ID, currently always zero	*/
    /* Response data (16 bytes) */
    ULONG	       width;		/* Full display size			*/
    ULONG	       height;
    ULONG	       titlebar;	/* Android screenbar size		*/
    ULONG	       orientation;	/* Orientation in which size was taken	*/
};

/* Show a single bitmap on display (v2) */
struct ShowRequest
{
    struct WaitRequest req;		/* cmd_Show, 8				*/
    /* Request */
    ULONG	       displayid;	/* DisplayID, currently always zero	*/
    ULONG	       left;		/* Offset				*/
    ULONG	       top;
    ULONG	       width;		/* Size					*/
    ULONG	       height;
    ULONG	       mod;		/* Bytes per line			*/
    ULONG	       orientation;	/* Orientation				*/
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
    ULONG	   width;
    ULONG	   height;
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

/* Initial handshake and protocol version verification */
struct HelloRequest
{
    struct WaitRequest req;	/* cmd_Hello, 1				*/
    ULONG	       version;	/* Protocol version			*/
};

/* Mouse/touchscreen event data */
struct PointerEvent
{
    ULONG x;
    ULONG y;
    ULONG action;
};

struct KeyEvent
{
    ULONG code;
    ULONG flags;
};

void agfxInt(int pipe, int mode, void *data);
void SendRequest(struct Request *req, struct agfx_staticdata *xsd);
void DoRequest(struct WaitRequest *req, struct agfx_staticdata *xsd);
