
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <math.h>
#include <stdlib.h>

#define WIDTH  400
#define HEIGHT 400
#define DEPTH  8   // 8-bit chunky (256 colors)

struct Window *win;
struct RastPort *rp;
struct BitMap *offbm;
struct RastPort offrp;

static int f = 0;

static void draw(struct RastPort *rp) {
    f++;

    for (int x = 0; x <= WIDTH; x += 20) {
        for (int y = 0; y <= HEIGHT; y += 20) {
            double R = 10.0;

            for (int n = 0; n < 2; n++) {
                double k = (n == 0) ? 0.0 : M_PI/2.0;

                double X = x - 250 + 200 * sin(f/50.0 + k);
                double Y = y - 250 + 200 * sin(f/67.0 + k);

                double s = sin(f/50.0 + k);
                double c = cos(f/50.0 + k);

                double v1 = fabs(s*X - c*Y) / 3.0;
                double v2 = (X*X + Y*Y) / 4000.0 + 3.0;

                // clamp
                if (v1 < 0) v1 = 0;

                if (v1 < R) R = v1;
                if (v2 < R) R = v2;
            }

            int Rint = (int)R;
            RectFill(rp, x - Rint, y - Rint,
                          x + Rint, y + Rint);
        }
    }
}

int main() {
    if ((win = OpenWindowTags(NULL,
        WA_Left,   100,
        WA_Top,    50,
		WA_Width,  WIDTH,
		WA_Height, HEIGHT,
        WA_Title,  (IPTR)"Rectangles Demo Port",
        WA_Flags,  WFLG_SIMPLE_REFRESH | WFLG_SMART_REFRESH | WFLG_ACTIVATE | WFLG_CLOSEGADGET,
        TAG_END))) {

        rp = win->RPort;

		// Allocate an off-screen bitmap
		offbm = AllocBitMap(WIDTH, HEIGHT, DEPTH, BMF_CLEAR, win->RPort->BitMap);
		if (!offbm) {
			CloseWindow(win);
			return 20;
		}

		// Init a RastPort that uses our bitmap
		InitRastPort(&offrp);
		offrp.BitMap = offbm;

        BOOL running = TRUE;
        struct IntuiMessage *msg;

        while (running) {
            // clear background (set pen 0 and erase)
            SetAPen(&offrp, 0);
            RectFill(&offrp, 0, 0, win->Width, win->Height);

            // set pen color (pen 1)
            SetAPen(&offrp, 2);

            draw(&offrp);
			ClipBlit(&offrp, 0, 0, rp, 0, 0, WIDTH, HEIGHT, 0xC0); // COPY
#if (0)
			// Wait for something to happen on the window’s port
			WaitPort(win->UserPort);
			
            // Handle events
            while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) {
                if (msg->Class == IDCMP_CLOSEWINDOW) {
                    running = FALSE;
                }
                ReplyMsg((struct Message *)msg);
            }
#endif
        }

        CloseWindow(win);
    }

    return 0;
}
