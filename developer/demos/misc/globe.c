
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

double x=0, u=0, v=0, f=0;
int n = 200;

static void draw(struct RastPort *rp) {
    double r = 2.0 * M_PI / n;
    int W = WIDTH; // 400

    // clear background
    SetAPen(rp, 0);
    RectFill(rp, 0, 0, WIDTH, HEIGHT);

    for (int i = 0; i < n; i++) {
        for (double j = 0; j < 55; j += 0.1) {
            double R = r * i + x - 99.0 * f;
            u = sin(i + v + f) - sin(R);
            v = cos(i + v + f) - cos(R);
            x = u + f;

            double X = n + 99.0 * u;
            double Y = n + 99.0 * v;

            // set pen depending on j
            SetAPen(rp, (int)(2*j + 66) & 255);

            Move(rp, (LONG)X, (LONG)Y);
            Draw(rp, (LONG)X+1, (LONG)Y+1);
        }
    }

    f += 0.00005;
}

int main() {
    if ((win = OpenWindowTags(NULL,
        WA_Left,   100,
        WA_Top,    50,
		WA_Width,  WIDTH,
		WA_Height, HEIGHT,
        WA_Title,  (IPTR)"Demo",
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
