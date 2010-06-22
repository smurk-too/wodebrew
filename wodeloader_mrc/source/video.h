#ifndef _VIDEO_H_
#define _VIDEO_H_

#include "libpng/pngu/pngu.h"

/* Prototypes */
void Con_Init(u32, u32, u32, u32);
void Con_InitTr(u32, u32, u32, u32);
void Con_Clear(void);
void Con_ClearLine(void);

void Video_Configure(GXRModeObj *);
void Video_SetMode(void);
void Video_Clear(s32);
void Video_DrawPng(IMGCTX, PNGUPROP, u16, u16);
void Video_DrawPngTrans(IMGCTX, PNGUPROP, u16, u16);
void Video_DrawPngResize(IMGCTX, PNGUPROP, u16, u16,u16,u16);

void Video_Oscurecer(int,int,int,int);

#endif
