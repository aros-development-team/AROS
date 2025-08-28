/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <clib/alib_protos.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <stdio.h>

struct BitMap *LoadPictureAsBitMap(CONST_STRPTR filename, struct BitMap **testbm)
{
    Object *dto = NULL;
    struct BitMap *bm = NULL;

    dto = NewDTObject((STRPTR)filename,
						DTA_SourceType, 		DTST_FILE,
						DTA_GroupID, 			GID_PICTURE,
						OBP_Precision, 			PRECISION_EXACT,
						PDTA_FreeSourceBitMap, 	FALSE,
						PDTA_Remap, 			FALSE,
						PDTA_DestMode,   		PMODE_V43,
						TAG_END);

    if (dto)
    {
        struct BitMapHeader *bmhd;
        GetAttr(PDTA_BitMapHeader, dto, (APTR)&bmhd);
        if(bmhd)
        {
			ULONG width = bmhd->bmh_Width, height = bmhd->bmh_Height, depth = bmhd->bmh_Depth;
			struct FrameInfo fri = {0};
			DoMethod(dto, DTM_FRAMEBOX, NULL, (IPTR)&fri, (IPTR)&fri, sizeof(struct FrameInfo),0);
			if (fri.fri_Dimensions.Depth > 0)
			{
				if (DoMethod(dto, DTM_PROCLAYOUT, NULL, TRUE))
				{
					GetAttr(PDTA_DestBitMap, dto, (APTR)&bm);
					if (!bm)
						GetAttr(PDTA_BitMap, dto, (APTR)&bm);
					if (bm)
					{
						struct Screen *pubScreen = LockPubScreen(NULL);

						struct BitMap *clone2, *clone = AllocBitMap(
							width, height, depth,
							0,
							(pubScreen) ? pubScreen->RastPort.BitMap : 0);

						clone2 = AllocBitMap(
							width, height, depth,
							0,
							(pubScreen) ? pubScreen->RastPort.BitMap : 0);
						
						if (pubScreen)
							UnlockPubScreen(NULL, pubScreen);

						if (clone)
						{
							struct RastPort rp_dst;
							InitRastPort(&rp_dst);
							rp_dst.BitMap = clone;
							if (clone2) {
								BltBitMapRastPort(bm, 0, 0, &rp_dst, 0, 0, bm->BytesPerRow * 8, bm->Rows, 0xC0);
								*testbm = clone2;
							}
							else {
								FreeBitMap(clone);
								clone = NULL;
							}
						}
						else {
							if (clone2)
								FreeBitMap(clone2);
						}

						bm = clone;
					}
				}
			}
		}

        DisposeDTObject(dto);
    }

    return bm;
}

int main(void)
{
    struct BitMap *bm = NULL, *testbm = NULL;
    struct Window *win = NULL;

    /* Load the bitmap from file */
    bm = LoadPictureAsBitMap("SYS:Developer/Debug/Tests/Datatypes/png/SamplePNGImage_1mb.png", &testbm);
    if (!bm)
    {
        printf("Failed to load image\n");
        return 20;
    }

    /* Open a simple window */
    win = OpenWindowTags(NULL,
                         WA_Left,        100,
                         WA_Top,         100,
                         WA_Width,       GetBitMapAttr(bm, BMA_WIDTH),
                         WA_Height,      GetBitMapAttr(bm, BMA_HEIGHT),
                         WA_Title,       (IPTR)"Image Viewer",
                         WA_Flags,       WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_SMART_REFRESH | WFLG_ACTIVATE,
                         WA_IDCMP,       IDCMP_CLOSEWINDOW,
                         TAG_END);

    if (!win)
    {
        printf("Failed to open window\n");
        FreeBitMap(bm);
        return 20;
    }

    {
		ULONG 	width = GetBitMapAttr(bm, BMA_WIDTH),
				height = GetBitMapAttr(bm, BMA_HEIGHT);
		ULONG endpos, value;
		struct timeval tv_start, tv_end;
        struct RastPort rp_src;
        InitRastPort(&rp_src);
        rp_src.BitMap = bm;
		LONG dur;
		int i, t;

        BltBitMapRastPort(bm,
                          0, 0,
                          win->RPort,
                          0, 0,
                          width,
                          height,
                          0xC0);

		ULONG tiles = width / 138, step = (256/tiles);
		printf("Testing Brighten\n");
		for (t = i = 0; i < 256; t++, i = i + step) {
			Delay(25);
			ProcessPixelArray(&rp_src, 10 + (t * 138), 10, 128, 128, POP_BRIGHTEN, i, NULL);
			BltBitMapRastPort(bm,
				  0, 0,
				  win->RPort,
				  0, 0,
				  width,
				  height,
				  0xC0);   
		}
		rp_src.BitMap = testbm;
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		endpos = 10 + (tiles - 1 * 138);
		value = (tiles - 1) * step;
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, 0, 0, 512, 128, POP_BRIGHTEN, value, NULL);
		}
		Permit();
		printf("\n Aligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 512x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, endpos, 10, 128, 128, POP_BRIGHTEN, value, NULL);
		}
		Permit();
		printf(" Missaligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 128x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);

        rp_src.BitMap = bm;		
		printf("\nTesting Darken\n");
		for (t = i = 0; i < 256; t++, i = i + step) {
			Delay(25);
			ProcessPixelArray(&rp_src, 10 + (t * 138), 148, 128, 128, POP_DARKEN, i, NULL);
			BltBitMapRastPort(bm,
				  0, 0,
				  win->RPort,
				  0, 0,
				  width,
				  height,
				  0xC0);   
		}
		rp_src.BitMap = testbm;
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		value = (tiles - 1) * step;
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, 0, 0, 512, 128, POP_DARKEN, value, NULL);
		}
		Permit();
		printf("\n Aligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 512x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, endpos, 148, 128, 128, POP_DARKEN, value, NULL);
		}
		Permit();
		printf(" Missaligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 128x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);

        rp_src.BitMap = bm;
		printf("\nTesting Tint\n");
		for (t = 0; t < tiles; t++) {
			Delay(3 * 50);
			ULONG tintval = 0xFFFF2020;
			ProcessPixelArray(&rp_src, 10 + (t * 138), 296, 128, 128, POP_TINT, (LONG)tintval, NULL);
			BltBitMapRastPort(bm,
				  0, 0,
				  win->RPort,
				  0, 0,
				  width,
				  height,
				  0xC0);   
		}
		rp_src.BitMap = testbm;
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, 0, 0, 512, 128, POP_TINT, (LONG)0xFFFF2020, NULL);
		}
		Permit();
		printf("\n Aligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 512x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, endpos, 296, 128, 128, POP_TINT, (LONG)0xFFFF2020, NULL);
		}
		Permit();
		printf(" Missaligned operation -:\n");
		printf("   Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf("   Operation count  : %d (x 128x128)\n", (int)i);
		printf("   Ops/sec          : %f\n", i * 1000000.0 / dur);


		Delay(3 * 50);
        rp_src.BitMap = bm;
		printf("\nTesting Blur\n");
		ProcessPixelArray(&rp_src, width / 3 , 10, width / 3, height - 20, POP_BLUR, 128, NULL);
		ProcessPixelArray(&rp_src, (width / 3) * 2 , 10, (width / 3) - 10, height - 20, POP_BLUR, 255, NULL);
		BltBitMapRastPort(bm,
			  0, 0,
			  win->RPort,
			  0, 0,
			  width,
			  height,
			  0xC0);
		rp_src.BitMap = testbm;
		Forbid();
		CurrentTime(&tv_start.tv_secs, &tv_start.tv_micro);
		for(i = 0; ; i++)
		{
			CurrentTime(&tv_end.tv_secs, &tv_end.tv_micro);
			dur = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_micro - tv_start.tv_micro;
			if (dur >= 4 * 1000000) break;
			ProcessPixelArray(&rp_src, 10, 10, width - 20, height - 20, POP_BLUR, 0, NULL);
		}
		Permit();
		printf("\n Elapsed time     : %d us (%f s)\n", (int)dur, (double)dur / 1000000);
		printf(" Operation count  : %d (x %ux%u)\n", (int)i, width - 20, height - 20);
		printf(" Ops/sec          : %f\n", i * 1000000.0 / dur);

    }

	printf("Testing complete\n");

    BOOL done = FALSE;
    struct IntuiMessage *msg;
    while (!done)
    {
        WaitPort(win->UserPort);
        while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
        {
            if (msg->Class == IDCMP_CLOSEWINDOW)
                done = TRUE;
            ReplyMsg((struct Message *)msg);
        }
    }

    /* Cleanup */
    CloseWindow(win);
    FreeBitMap(testbm);
    FreeBitMap(bm);

    return 0;
}
