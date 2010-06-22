#include <stdio.h>
#include <ogcsys.h>

#include "sys.h"
#include "video.h"

/* Video variables */
static u32 *framebuffer = NULL;
static GXRModeObj *vmode = NULL;

extern s32 CON_InitTr(GXRModeObj *rmode, s32 conXOrigin,s32 conYOrigin,s32 conWidth,s32 conHeight, s32 bgColor);

void Con_Init(u32 x, u32 y, u32 w, u32 h)
{
	/* Create console in the framebuffer */
	CON_InitEx(vmode, x, y, w, h);
}

void Con_InitTr(u32 x, u32 y, u32 w, u32 h)
{
	/* Create console in the framebuffer */
	CON_InitTr(vmode, x, y, w, h, 4);
}

void Con_Clear(void)
{
	/* Clear console */
	printf("\x1b[2J");
	fflush(stdout);
}

void Con_ClearLine(void)
{
	s32 cols, rows;
	u32 cnt;

	printf("\r");
	fflush(stdout);

	/* Get console metrics */
	CON_GetMetrics(&cols, &rows);

	/* Erase line */
	for (cnt = 1; cnt < cols; cnt++) {
		printf(" ");
		fflush(stdout);
	}

	printf("\r");
	fflush(stdout);
}

void Video_Configure(GXRModeObj *rmode)
{
	/* Configure the video subsystem */
	VIDEO_Configure(rmode);

	/* Setup video */
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if (rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
}

void Video_SetMode(void)
{
	/* Select preferred video mode */
	vmode = VIDEO_GetPreferredMode(NULL);

	/* Allocate memory for the framebuffer */
	framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));

	/* Configure the video subsystem */
	VIDEO_Configure(vmode);

	/* Setup video */
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if (vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	/* Clear the screen */
	Video_Clear(COLOR_BLACK);
}

void Video_Clear(s32 color)
{
	VIDEO_ClearFrameBuffer(vmode, framebuffer, color);
}

void Video_DrawPng(IMGCTX ctx, PNGUPROP imgProp, u16 x, u16 y){
	PNGU_DECODE_TO_COORDS_YCbYCr(ctx, x, y, imgProp.imgWidth, imgProp.imgHeight, vmode->fbWidth, vmode->xfbHeight, framebuffer);
}
void Video_DrawPngTrans(IMGCTX ctx, PNGUPROP imgProp, u16 x, u16 y){
	PNGU_DECODE_TO_COORDS_YCbYCrTrans(ctx, x, y, imgProp.imgWidth, imgProp.imgHeight, vmode->fbWidth, vmode->xfbHeight, framebuffer);
}
void Video_DrawPngResize(IMGCTX ctx, PNGUPROP imgProp, u16 x, u16 y,u16 width,u16 height){
	PNGU_DECODE_TO_COORDS_YCbYCr2(ctx, x, y, imgProp.imgWidth, imgProp.imgHeight, vmode->fbWidth, vmode->xfbHeight, framebuffer, width, height);
}


u8 GRRLIB_ClampVar8(float Value){
    /* Using float to increase the precision.
    This makes a full spectrum (0 - 255) possible. */
    //Value = roundf(Value);
    if (Value < 0) {
        Value = 0;
    } else if (Value > 255) {
        Value = 255;
    }
    return (u8)Value;
}

void GRRLIB_GetPixelFromFB(int x, int y, u8 *R1, u8 *G1, u8 *B1, u8* R2, u8 *G2, u8 *B2) {
	u32 Buffer = (((u32 *)framebuffer)[y*(vmode->fbWidth/2)+x]);
	u8 *Colors = (u8 *) &Buffer;

	/* Color channel:
	Colors[0] = Y1
	Colors[1] = Cb
	Colors[2] = Y2
	Colors[3] = Cr*/

	//oscurecer colores
	Colors[0]=(u8)GRRLIB_ClampVar8(Colors[0]-(Colors[0]/2.8));
	Colors[2]=(u8)GRRLIB_ClampVar8(Colors[2]-(Colors[2]/2.8));
	
	//para blanco y negro =128
	Colors[1]=136;
	Colors[3]=120;

	framebuffer[y*(vmode->fbWidth/2)+x]=Buffer;
	/*
	codigo original (clona el color casi perfectamente)
	
	*R1 = GRRLIB_ClampVar8(1.164 * (Colors[0] - 16) + 1.596 * (Colors[3] - 128));
	*G1 = GRRLIB_ClampVar8(1.164 * (Colors[0] - 16) - 0.813 * (Colors[3] - 128) - 0.392 * (Colors[1] - 128));
	*B1 = GRRLIB_ClampVar8(1.164 * (Colors[0] - 16) + 2.017 * (Colors[1] - 128) );

	*R2 = GRRLIB_ClampVar8(1.164 * (Colors[2] - 16) + 1.596 * (Colors[3] - 128) );
	*G2 = GRRLIB_ClampVar8(1.164 * (Colors[2] - 16) - 0.813 * (Colors[3] - 128) - 0.392 * (Colors[1] - 128));
	*B2 = GRRLIB_ClampVar8(1.164 * (Colors[2] - 16) + 2.017 * (Colors[1] - 128) );
	*/
}

u32 CvtRGB (u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2)
{
  int y1, cb1, cr1, y2, cb2, cr2, cb, cr;
 
  y1 = (299 * r1 + 587 * g1 + 114 * b1) / 1000;
  cb1 = (-16874 * r1 - 33126 * g1 + 50000 * b1 + 12800000) / 100000;
  cr1 = (50000 * r1 - 41869 * g1 - 8131 * b1 + 12800000) / 100000;
 
  y2 = (299 * r2 + 587 * g2 + 114 * b2) / 1000;
  cb2 = (-16874 * r2 - 33126 * g2 + 50000 * b2 + 12800000) / 100000;
  cr2 = (50000 * r2 - 41869 * g2 - 8131 * b2 + 12800000) / 100000;
 
  cb = (cb1 + cb2) >> 1;
  cr = (cr1 + cr2) >> 1;
 
  return (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
}



void Video_Oscurecer(int x, int y, int x2, int y2) {
	int i,j;
	x2/=2;
	x/=2;

	for(j=y;j<y2;j++){
		//int y=j*320;
		for(i=x;i<x2;i++){
			u32 Buffer = ((u32 *)framebuffer)[j*320+i];
			u8 *Colors = (u8 *) &Buffer;

			/* Color channel:
			Colors[0] = Y1
			Colors[1] = Cb
			Colors[2] = Y2
			Colors[3] = Cr*/

			//oscurecer colores
			Colors[0]=GRRLIB_ClampVar8(Colors[0]-(Colors[0]/3.75));
			Colors[2]=GRRLIB_ClampVar8(Colors[2]-(Colors[2]/3.75));
	
			//azular,para blanco y negro =128
			Colors[1]=136;
			Colors[3]=120;

			framebuffer[j*320+i]=Buffer;
		}
	}
}
