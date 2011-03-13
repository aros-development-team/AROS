#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <config.h>
#include "pixman-private.h"	/* For image->bits.format
				 * FIXME: there should probably be public API for this
				 */
#include "gtk-utils.h"

void
show_image (pixman_image_t *image)
{
    struct Window *window;
    int width, height, stride;
    gboolean has_alpha;
    pixman_format_code_t format;
    struct IntuiMessage *imsg;
    struct MsgPort *port;
    BOOL terminated = FALSE;

    width = pixman_image_get_width (image);
    height = pixman_image_get_height (image);
    stride = pixman_image_get_stride (image);

    window = OpenWindowTags(NULL,
	WA_Title, "pixman-test-program",
	WA_InnerWidth,    width,
	WA_InnerHeight,   height,
	WA_Activate,      TRUE,
	WA_SmartRefresh,  TRUE,
	WA_GimmeZeroZero, TRUE,
	WA_CloseGadget,   TRUE,
	WA_DragBar,       TRUE,
	WA_DepthGadget,   TRUE,
	WA_IDCMP,         IDCMP_CLOSEWINDOW,
    TAG_END);

    if (!window)
	printf("Can't open window\n");

    format = image->bits.format;
    
    if (format == PIXMAN_a8r8g8b8)
	has_alpha = TRUE;
    else if (format == PIXMAN_x8r8g8b8)
	has_alpha = FALSE;
    else
	printf("Can't deal with this format: %x\n", format);

    WritePixelArray(pixman_image_get_data(image), 0, 0, stride, window->RPort, 0, 0, width, height, RECTFMT_ARGB);

    port = window->UserPort;
    while (!terminated)
    {
	Wait(1L << port->mp_SigBit);
	if ((imsg = (struct IntuiMessage *)GetMsg(port)) != NULL)
	{
	    switch (imsg->Class)
	    {
		case IDCMP_CLOSEWINDOW:
		    terminated = TRUE;
		    break;
	    }
	    ReplyMsg((struct Message *)imsg);
	}
    }

    CloseWindow(window);
}
