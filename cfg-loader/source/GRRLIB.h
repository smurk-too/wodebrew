/*===========================================
        GRRLIB 4.0.0
        Code     : NoNameNo
        Additional Code : Crayon
        GX hints : RedShade

Download and Help Forum : http://grrlib.santo.fr
===========================================*/

// Modified by oggzee


#ifndef __GXHDR__
#define __GXHDR__

/**
 * @file GRRLIB.h
 * GRRLIB library.
 */

#include <gccore.h>
#include <ogc/conf.h>
#include "util.h"

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

//	u32 mask = 0x
#define R(c)  (((c) >>24) &0xFF)  /**< Exract Red   component of colour. */
#define G(c)  (((c) >>16) &0xFF)  /**< Exract Green component of colour. */
#define B(c)  (((c) >> 8) &0xFF)  /**< Exract Blue  component of colour. */
#define A(c)  ( (c)       &0xFF)  /**< Exract Alpha component of colour. */

/**
 * Structure to hold the texture informations.
 */
typedef struct GRRLIB_texImg{
    unsigned int w;         /**< width of the texture. */
    unsigned int h;         /**< height of the texture. */
    unsigned int tilew;     /**< widht of a tile. */
    unsigned int tileh;     /**< height of a tile. */
    unsigned int nbtilew;   /**< number of tiles for the x axis. */
    unsigned int nbtileh;   /**< number of tiles for the y axis. */
    unsigned int tilestart; /**< offset for starting position. */
    void *data;             /**< pointer to the texture data. */
} GRRLIB_texImg;

/**
 * Structure to hold the bytemap character informations.
 */
typedef struct GRRLIB_bytemapChar{
    u8 character;    /**< Which character. */
    u8 width;        /**< Character width. */
    u8 height;       /**< Character height. */
    s8 relx;         /**< Horizontal offset according to cursor (-128..127). */
    s8 rely;         /**< Vertical offset according to cursor (-128..127). */
    u8 shift;        /**< Horizontal cursor shift after drawing the character. */
    u8 *data;        /**< Character data itself (uncompressed, 8 bits per pixel). */
} GRRLIB_bytemapChar;

/**
 * Structure to hold the bytemap font informations.
 */
typedef struct GRRLIB_bytemapFont{
    u8 version;                     /**< Version . */
    s8 addSpace;                    /**< Add-space after each char (-128..127). */
    u32 *palette;                   /**< Font palette. */
    char *name;                     /**< Font name. */
    u16 nbChar;                     /**< Number of characters in font. */
    GRRLIB_bytemapChar *charDef;    /**< List of bitmap character definitions. */
} GRRLIB_bytemapFont;


extern Mtx GXmodelView2D;

inline void GRRLIB_FillScreen(u32 color);

inline void GRRLIB_Plot(f32 x, f32 y, u32 color);
void GRRLIB_NPlot(guVector v[], u32 color, long n);

inline void GRRLIB_Line(f32 x1, f32 y1, f32 x2, f32 y2, u32 color);

inline void GRRLIB_Rectangle(f32 x, f32 y, f32 width, f32 height, u32 color, u8 filled);
void GRRLIB_NGone(guVector v[], u32 color, long n);
void GRRLIB_NGoneFilled(guVector v[], u32 color, long n);

GRRLIB_texImg GRRLIB_CreateEmptyTexture(unsigned int, unsigned int);
GRRLIB_texImg GRRLIB_LoadTexture(const unsigned char my_img[]);

GRRLIB_bytemapFont GRRLIB_LoadBMF(const unsigned char my_bmf[]);
void GRRLIB_FreeBMF(GRRLIB_bytemapFont bmf);

void GRRLIB_InitTileSet(struct GRRLIB_texImg *tex, unsigned int tilew, unsigned int tileh, unsigned int tilestart);

inline void GRRLIB_DrawImg(f32 xpos, f32 ypos, GRRLIB_texImg tex, float degrees, float scaleX, f32 scaleY, u32 color );
extern void GRRLIB_DrawImgQuad(guVector pos[4], struct GRRLIB_texImg *tex, u32 color);
inline void GRRLIB_DrawTile(f32 xpos, f32 ypos, GRRLIB_texImg tex, float degrees, float scaleX, f32 scaleY, u32 color, int frame);
extern void GRRLIB_DrawTileQuad(guVector pos[4], struct GRRLIB_texImg *tex, u32 color,int frame);

void GRRLIB_Printf(f32 xpos, f32 ypos, GRRLIB_texImg tex, u32 color, f32 zoom, const char *text, ...);
void GRRLIB_PrintBMF(f32 xpos, f32 ypos, GRRLIB_bytemapFont bmf, f32 zoom, const char *text, ...);

bool GRRLIB_PtInRect(int hotx, int hoty, int hotw, int hoth, int wpadx, int wpady);
bool GRRLIB_RectInRect(int rect1x, int rect1y, int rect1w, int rect1h, int rect2x, int rect2y, int rect2w, int rect2h);
bool GRRLIB_RectOnRect(int rect1x, int rect1y, int rect1w, int rect1h, int rect2x, int rect2y, int rect2w, int rect2h);

u32 GRRLIB_GetPixelFromtexImg(int x, int y, GRRLIB_texImg tex);
void GRRLIB_SetPixelTotexImg(int x, int y, GRRLIB_texImg tex, u32 color);

void GRRLIB_FlushTex(GRRLIB_texImg tex);

void GRRLIB_BMFX_Grayscale(GRRLIB_texImg texsrc, GRRLIB_texImg texdest);
void GRRLIB_BMFX_Invert(GRRLIB_texImg texsrc, GRRLIB_texImg texdest);
void GRRLIB_BMFX_FlipH(GRRLIB_texImg texsrc, GRRLIB_texImg texdest);
void GRRLIB_BMFX_FlipV(GRRLIB_texImg texsrc, GRRLIB_texImg texdest);
void GRRLIB_BMFX_Blur(GRRLIB_texImg texsrc, GRRLIB_texImg texdest, int factor);
void GRRLIB_BMFX_Scatter(GRRLIB_texImg texsrc, GRRLIB_texImg texdest, int factor);
void GRRLIB_BMFX_Pixelate(GRRLIB_texImg texsrc, GRRLIB_texImg texdest, int factor);

void GRRLIB_GXEngine(guVector v[], u32 color, long count, u8 fmt);

void GRRLIB_FreeTexture(struct GRRLIB_texImg *tex);
GRRLIB_texImg GRRLIB_Screen2Texture();
void GRRLIB_Screen2Texture_buf(GRRLIB_texImg *tx);

GRRLIB_texImg GRRLIB_AAScreen2Texture();
void GRRLIB_AAScreen2Texture_buf(GRRLIB_texImg *tx);
void GRRLIB_prepareAAPass(int, int);
void GRRLIB_drawAAScene(int, GRRLIB_texImg *texAABuffer);
void GRRLIB_ResetVideo(void);

void GRRLIB_Init();

void GRRLIB_Render();

void GRRLIB_Exit();

void GRRLIB_GetPixelFromFB(int x, int y, u8 *R1, u8 *G1, u8 *B1, u8 *R2, u8 *G2, u8 *B2 );
u8 GRRLIB_ClampVar8(float Value);

u32 GRRLIB_GetColor(u8 r, u8 g, u8 b, u8 a);

bool GRRLIB_ScrShot(const char*);

//
void GRRLIB_Print2(f32 xpos, f32 ypos, struct GRRLIB_texImg tex, u32 color, u32 outline, u32 shadow, const char *text);
void GRRLIB_Print(f32 xpos, f32 ypos, struct GRRLIB_texImg tex, u32 color, const char *text);

extern int GRRLIB_Init_VMode(GXRModeObj *a_rmode, void *fb0, void *fb1);
extern void GRRLIB_DrawSlice2(f32 xpos, f32 ypos, GRRLIB_texImg tex,
		float degrees, float scaleX, f32 scaleY, u32 color1, u32 color2,
		float x, float y, float w, float h);
extern void GRRLIB_DrawSlice(f32 xpos, f32 ypos, GRRLIB_texImg tex,
		float degrees, float scaleX, f32 scaleY, u32 color,
		float x, float y, float w, float h);
extern void** _GRRLIB_GetXFB(int *cur_fb);
extern void _GRRLIB_SetFB(int cur_fb);
extern void _GRRLIB_Render();
extern void _GRRLIB_VSync();
extern void _GRRLIB_Exit();


#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif

/**
 * @mainpage GRRLIB Documentation
 * @image html grrlib_logo.png
 * Welcome to the GRRLIB documentation.
 */
