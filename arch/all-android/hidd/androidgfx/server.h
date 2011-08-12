#define PIPE_NAME "AROS.display"

#define cmd_Query	0x00000001
#define cmd_Shutdown	0x00D1ED1E

#define CMDF_Quick	0x80000000
#define CMD_FLAGS_MASK	0xF0000000

struct Request
{
    struct Message  msg;
    struct Task	   *owner;	/* Owning task  */
    ULONG	    signal;	/* Reply signal */
    ULONG	    cmd;	/* Command	*/
};

struct QueryRequest
{
    struct Request  req;	/* cmd_Query				*/
    ULONG	    id;		/* Display ID, currently reserved	*/
    ULONG	    width;	/* Result, will be filled in		*/
    ULONG	    height;
};

void agfxTask(int pipe, struct agfx_staticdata *xsd);
void DoRequest(struct Request *req, struct agfx_staticdata *xsd);
