
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

double t = 0.0;

// magnitude helper
static double mag(double x, double y) {
    return sqrt(x * x + y * y);
}

static void point(struct RastPort *rp, double x, double y) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
		Move(rp, (LONG)x, (LONG)y);
		Draw(rp, (LONG)x, (LONG)y);
	}
}

static void a(struct RastPort *rp, double y, double i) {
    double k, e, d, q, c;

    k = (y < 11
         ? 6 + sin((double)(((int)y) ^ 8)) * 6
         : y / 5.0 + cos(y / 2.0))
        * cos(i - t / 4.0);

    e = y / 7.0 - 13.0;
    d = mag(k, e) + sin(e / 4.0 + t) / 2.0;
    q = y * k / d * (3 + sin(d * 2 + y / 2 - t * 4));
    c = d / 2.0 + 1 - t / 2.0;

    point(rp, q + 60 * cos(c) + 200, q * sin(c) + d * 29 - 170);
}

static void draw(struct RastPort *rp) {
    t += M_PI / 120.0;
    for (int i = 10000; i--; ) {
        a(rp, i / 345.0, i);  // pass i too (needed in formula)
    }
}

int main() {
    if ((win = OpenWindowTags(NULL,
        WA_Left,   100,
        WA_Top,    50,
		WA_Width,  WIDTH,
		WA_Height, HEIGHT,
        WA_Title,  (IPTR)"Jellyfish Demo Port",
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
