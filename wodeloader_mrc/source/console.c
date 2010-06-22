#include <stdlib.h>
#include <string.h>
#include <reent.h>
#include <errno.h>

#include "ogc/machine/asm.h"
#include "ogc/machine/processor.h"
#include "ogc/color.h"
#include "ogc/cache.h"
#include "ogc/video.h"
#include "ogc/system.h"

#include "console.h"
#include "ogc/consol.h"
#include "ogc/usbgecko.h"

#include <stdio.h>
#include <sys/iosupport.h>

//---------------------------------------------------------------------------------
const devoptab_t dotab_stdout = {
//---------------------------------------------------------------------------------
	"stdout",	// device name
	0,			// size of file structure
	NULL,		// device open
	NULL,		// device close
	__console_write,	// device write
	NULL,		// device read
	NULL,		// device seek
	NULL,		// device fstat
	NULL,		// device stat
	NULL,		// device link
	NULL,		// device unlink
	NULL,		// device chdir
	NULL,		// device rename
	NULL,		// device mkdir
	0,			// dirStateSize
	NULL,		// device diropen_r
	NULL,		// device dirreset_r
	NULL,		// device dirnext_r
	NULL,		// device dirclose_r
	NULL		// device statvfs_r
};

//color table
const unsigned int color_table[] =
{
  0x00800080,		// 30 normal black
  0x246A24BE,		// 31 normal red
  0x4856484B,		// 32 normal green
  0x6D416D8A,		// 33 normal yellow
  0x0DBE0D75,		// 34 normal blue
  0x32A932B4,		// 35 normal magenta
  0x56955641,		// 36 normal cyan
  0xC580C580,		// 37 normal white
  0x7B807B80,		// 30 bright black
  0x4C544CFF,		// 31 bright red
  0x95299512,		// 32 bright green
  0xE200E294,		// 33 bright yellow
  0x1CFF1C6B,		// 34 bright blue
  0x69D669ED,		// 35 bright magenta
  0xB2ABB200,		// 36 bright cyan
  0xFF80FF80,		// 37 bright white
};

static u32 do_xfb_copy = FALSE;
static struct _console_data_s stdcon;
static struct _console_data_s *curr_con = NULL;
static void *_console_buffer = NULL;
static void *_bg_buffer = NULL;
static unsigned int _bg_color = COLOR_BLACK;

struct _c1
{
   	char c;
   	int fg, bg;
};

static int _c_buffer_size = 0;
static struct _c1 *_c_buffer = NULL;

static s32 __gecko_status = -1;
static u32 __gecko_safe = 0;

extern u8 console_font_8x16[];

extern void *VIDEO_GetCurrrentFramebuffer();

int fb_change = 0;
int retrace_cnt = 0;

void __console_vipostcb(u32 retraceCnt)
{
	if (retrace_cnt < 5) {
		retrace_cnt++;
		return;
	}
	if (!fb_change) return;
	do_xfb_copy = TRUE;
	__console_flush(0);
	do_xfb_copy = FALSE;
}

void __console_flush(int retrace_min)
{
	u32 ycnt,xcnt, fb_stride;
	u32 *fb,*ptr;

	if (!fb_change && retrace_min >= 0) {
		if (retrace_min == 0) retrace_cnt = 0;
		return;
	}
	if (retrace_cnt < retrace_min) return;
	fb_change = 0;
	retrace_cnt = 0;

	ptr = curr_con->destbuffer;
	fb = VIDEO_GetCurrentFramebuffer()+(curr_con->target_y*curr_con->tgt_stride) + curr_con->target_x*VI_DISPLAY_PIX_SZ;
	fb_stride = curr_con->tgt_stride/4 - (curr_con->con_xres/VI_DISPLAY_PIX_SZ);

	for(ycnt=curr_con->con_yres;ycnt>0;ycnt--)
	{
		for(xcnt=curr_con->con_xres;xcnt>0;xcnt-=VI_DISPLAY_PIX_SZ)
		{
			*fb++ = *ptr++;
		}
		fb += fb_stride;
	}
}

void _bg_grab()
{
	u32 ycnt,xcnt, fb_stride;
	u32 *fb,*ptr;

	ptr = _bg_buffer;
	fb = VIDEO_GetCurrentFramebuffer()+(curr_con->target_y*curr_con->tgt_stride) + curr_con->target_x*VI_DISPLAY_PIX_SZ;
	fb_stride = curr_con->tgt_stride/4 - (curr_con->con_xres/VI_DISPLAY_PIX_SZ);

	for(ycnt=curr_con->con_yres;ycnt>0;ycnt--)
	{
		for(xcnt=curr_con->con_xres;xcnt>0;xcnt-=VI_DISPLAY_PIX_SZ)
		{
			*ptr++ = *fb++;
		}
		fb += fb_stride;
	}
}

static void _bg_console_drawc(int c)
{
	console_data_s *con;
	int ay;
	unsigned int *ptr;
	unsigned int *bg;
	unsigned char *pbits;
	unsigned char bits;
	unsigned int color;
	unsigned int fgcolor, bgcolor;
	unsigned int nextline;

	if(do_xfb_copy==TRUE) return;
	if(!curr_con) return;
	con = curr_con;

	ptr = (unsigned int*)(con->destbuffer + ( con->con_stride *  con->cursor_row * FONT_YSIZE ) + ((con->cursor_col * FONT_XSIZE / 2) * 4));
	bg = (unsigned int*)(_bg_buffer + ( con->con_stride *  con->cursor_row * FONT_YSIZE ) + ((con->cursor_col * FONT_XSIZE / 2) * 4));
	pbits = &con->font[c * FONT_YSIZE];
	nextline = con->con_stride/4 - 4;
	fgcolor = con->foreground;
	bgcolor = con->background;

	for (ay = 0; ay < FONT_YSIZE; ay++)
	{
		/* hard coded loop unrolling ! */
		/* this depends on FONT_XSIZE = 8*/
#if FONT_XSIZE == 8
		bits = *pbits++;

		/* bits 1 & 2 */
		if ( bits & 0x80)
			color = fgcolor & 0xFFFF0000;
		else
			color = *bg & 0xFFFF0000;
		if (bits & 0x40)
			color |= fgcolor  & 0x0000FFFF;
		else
			color |= *bg  & 0x0000FFFF;
		*ptr++ = color;
		bg++;

		/* bits 3 & 4 */
		if ( bits & 0x20)
			color = fgcolor & 0xFFFF0000;
		else
			color = *bg & 0xFFFF0000;
		if (bits & 0x10)
			color |= fgcolor  & 0x0000FFFF;
		else
			color |= *bg  & 0x0000FFFF;
		*ptr++ = color;
		bg++;

		/* bits 5 & 6 */
		if ( bits & 0x08)
			color = fgcolor & 0xFFFF0000;
		else
			color = *bg & 0xFFFF0000;
		if (bits & 0x04)
			color |= fgcolor  & 0x0000FFFF;
		else
			color |= *bg  & 0x0000FFFF;
		*ptr++ = color;
		bg++;

		/* bits 7 & 8 */
		if ( bits & 0x02)
			color = fgcolor & 0xFFFF0000;
		else
			color = *bg & 0xFFFF0000;
		if (bits & 0x01)
			color |= fgcolor  & 0x0000FFFF;
		else
			color |= *bg  & 0x0000FFFF;
		*ptr++ = color;
		bg++;

		/* next line */
		ptr += nextline;
		bg += nextline;
#else
#endif
	}
}

static void _nc_console_drawc(int c)
{
	console_data_s *con;
	int ay;
	unsigned int *ptr;
	unsigned char *pbits;
	unsigned char bits;
	unsigned int color;
	unsigned int fgcolor, bgcolor;
	unsigned int nextline;

	if(do_xfb_copy==TRUE) return;
	if(!curr_con) return;
	con = curr_con;

	if (_bg_buffer && con->background == _bg_color) {
		_bg_console_drawc(c);
		return;
	}

	ptr = (unsigned int*)(con->destbuffer + ( con->con_stride *  con->cursor_row * FONT_YSIZE ) + ((con->cursor_col * FONT_XSIZE / 2) * 4));
	pbits = &con->font[c * FONT_YSIZE];
	nextline = con->con_stride/4 - 4;
	fgcolor = con->foreground;
	bgcolor = con->background;

	for (ay = 0; ay < FONT_YSIZE; ay++)
	{
		/* hard coded loop unrolling ! */
		/* this depends on FONT_XSIZE = 8*/
#if FONT_XSIZE == 8
		bits = *pbits++;

		/* bits 1 & 2 */
		if ( bits & 0x80)
			color = fgcolor & 0xFFFF00FF;
		else
			color = bgcolor & 0xFFFF00FF;
		if (bits & 0x40)
			color |= fgcolor  & 0x0000FF00;
		else
			color |= bgcolor  & 0x0000FF00;
		*ptr++ = color;

		/* bits 3 & 4 */
		if ( bits & 0x20)
			color = fgcolor & 0xFFFF00FF;
		else
			color = bgcolor & 0xFFFF00FF;
		if (bits & 0x10)
			color |= fgcolor  & 0x0000FF00;
		else
			color |= bgcolor  & 0x0000FF00;
		*ptr++ = color;

		/* bits 5 & 6 */
		if ( bits & 0x08)
			color = fgcolor & 0xFFFF00FF;
		else
			color = bgcolor & 0xFFFF00FF;
		if (bits & 0x04)
			color |= fgcolor  & 0x0000FF00;
		else
			color |= bgcolor  & 0x0000FF00;
		*ptr++ = color;

		/* bits 7 & 8 */
		if ( bits & 0x02)
			color = fgcolor & 0xFFFF00FF;
		else
			color = bgcolor & 0xFFFF00FF;
		if (bits & 0x01)
			color |= fgcolor  & 0x0000FF00;
		else
			color |= bgcolor  & 0x0000FF00;
		*ptr++ = color;

		/* next line */
		ptr += nextline;
#else
#endif
	}

}

static void __console_drawc(int c)
{
	if(!curr_con) return;
	if (_bg_buffer && _c_buffer) {
		int cidx = curr_con->cursor_row * curr_con->con_cols + curr_con->cursor_col;
		_c_buffer[cidx].c = c;
		_c_buffer[cidx].fg = curr_con->foreground;
		_c_buffer[cidx].bg = curr_con->background;
	}
	_nc_console_drawc(c);
	//fb_change = 1;
}

void _bg_scroll()
{
	// clear
	unsigned int c;
	unsigned int *p;
	unsigned int *bg;
	c = (curr_con->con_xres * curr_con->con_yres)/2;
	p = (unsigned int*)curr_con->destbuffer;
	bg = _bg_buffer;
	while(c--) *p++ = *bg++;
    // scroll
	memmove(_c_buffer, _c_buffer + curr_con->con_cols,
		sizeof(struct _c1) * (curr_con->con_rows-1) * curr_con->con_cols);
	// clear last
	memset(_c_buffer + (curr_con->con_rows-1) * curr_con->con_cols, 0,
		sizeof(struct _c1) * curr_con->con_cols);
	// repaint
	int save_row = curr_con->cursor_row;
	int save_col = curr_con->cursor_col;
	int save_fg = curr_con->foreground;
	int save_bg = curr_con->background;
	int x, y, i = 0;
	for (y=0; y < curr_con->con_rows - 1; y++) {
		for (x=0; x < curr_con->con_cols; x++) {
			curr_con->cursor_col = x;
			curr_con->cursor_row = y;
			curr_con->foreground = _c_buffer[i].fg;
			curr_con->background = _c_buffer[i].bg;
			if (_c_buffer[i].c) _nc_console_drawc(_c_buffer[i].c);
			i++;
		}
	}
	curr_con->cursor_row = save_row;
	curr_con->cursor_col = save_col;
	curr_con->foreground = save_fg;
	curr_con->background = save_bg;
}


static void __console_clear(void)
{
	console_data_s *con;
	unsigned int c;
	unsigned int *p;
	unsigned int *bg;

	if(!curr_con) return;
	con = curr_con;

	c = (con->con_xres*con->con_yres)/2;
	p = (unsigned int*)con->destbuffer;
	if (_bg_buffer) {
		bg = _bg_buffer;
		while(c--)
			*p++ = *bg++;
		memset(_c_buffer, 0, _c_buffer_size);
	} else {
		while(c--)
			*p++ = con->background;
	}

	con->cursor_row = 0;
	con->cursor_col = 0;
	con->saved_row = 0;
	con->saved_col = 0;
}

void __console_init(void *framebuffer,int xstart,int ystart,int xres,int yres,int stride)
{
	unsigned int level;
	console_data_s *con = &stdcon;

	_CPU_ISR_Disable(level);

	con->destbuffer = framebuffer;
	con->con_xres = xres;
	con->con_yres = yres;
	con->con_cols = xres / FONT_XSIZE;
	con->con_rows = yres / FONT_YSIZE;
	con->con_stride = con->tgt_stride = stride;
	con->target_x = xstart;
	con->target_y = ystart;

	con->font = console_font_8x16;

	con->foreground = COLOR_WHITE;
	con->background = COLOR_BLACK;

	curr_con = con;

	__console_clear();
	fb_change = 1;

	devoptab_list[STD_OUT] = &dotab_stdout;
	devoptab_list[STD_ERR] = &dotab_stdout;
	_CPU_ISR_Restore(level);

	setvbuf(stdout, NULL , _IONBF, 0);
	setvbuf(stderr, NULL , _IONBF, 0);
}

void __console_init_ex(void *conbuffer,int tgt_xstart,int tgt_ystart,int tgt_stride,int con_xres,int con_yres,int con_stride)
{
	unsigned int level;
	console_data_s *con = &stdcon;

	_CPU_ISR_Disable(level);

	con->destbuffer = conbuffer;
	con->target_x = tgt_xstart;
	con->target_y = tgt_ystart;
	con->con_xres = con_xres;
	con->con_yres = con_yres;
	con->tgt_stride = tgt_stride;
	con->con_stride = con_stride;
	con->con_cols = con_xres / FONT_XSIZE;
	con->con_rows = con_yres / FONT_YSIZE;
	con->cursor_row = 0;
	con->cursor_col = 0;
	con->saved_row = 0;
	con->saved_col = 0;

	con->font = console_font_8x16;

	con->foreground = COLOR_WHITE;
	con->background = COLOR_BLACK;

	curr_con = con;

	if(_bg_buffer) {
		_bg_grab();
		con->background = _bg_color;
	}

	__console_clear();
	fb_change = 1;
	retrace_cnt = 0;

	devoptab_list[STD_OUT] = &dotab_stdout;
	devoptab_list[STD_ERR] = &dotab_stdout;

	VIDEO_SetPostRetraceCallback(__console_vipostcb);

	_CPU_ISR_Restore(level);

	setvbuf(stdout, NULL , _IONBF, 0);
	setvbuf(stderr, NULL , _IONBF, 0);
}

static int __console_parse_escsequence(char *pchr)
{
	char chr;
	console_data_s *con;
	int i;
	int parameters[3];
	int para;

	if(!curr_con) return -1;
	con = curr_con;

	/* set default value */
	para = 0;
	parameters[0] = 0;
	parameters[1] = 0;
	parameters[2] = 0;

	/* scan parameters */
	i = 0;
	chr = *pchr;
	while( (para < 3) && (chr >= '0') && (chr <= '9') )
	{
		while( (chr >= '0') && (chr <= '9') )
		{
			/* parse parameter */
			parameters[para] *= 10;
			parameters[para] += chr - '0';
			pchr++;
			i++;
			chr = *pchr;
		}
		para++;

		if( *pchr == ';' )
		{
		  /* skip parameter delimiter */
		  pchr++;
			i++;
		}
		chr = *pchr;
	}

	/* get final character */
	chr = *pchr++;
	i++;
	switch(chr)
	{
		/////////////////////////////////////////
		// Cursor directional movement
		/////////////////////////////////////////
		case 'A':
		{
			curr_con->cursor_row -= parameters[0];
			if(curr_con->cursor_row < 0) curr_con->cursor_row = 0;
			break;
		}
		case 'B':
		{
			curr_con->cursor_row += parameters[0];
			if(curr_con->cursor_row >= curr_con->con_rows) curr_con->cursor_row = curr_con->con_rows - 1;
			break;
		}
		case 'C':
		{
			curr_con->cursor_col += parameters[0];
			if(curr_con->cursor_col >= curr_con->con_cols) curr_con->cursor_col = curr_con->con_cols - 1;
			break;
		}
		case 'D':
		{
			curr_con->cursor_col -= parameters[0];
			if(curr_con->cursor_col < 0) curr_con->cursor_col = 0;
			break;
		}
		/////////////////////////////////////////
		// Cursor position movement
		/////////////////////////////////////////
		case 'H':
		case 'f':
		{
			curr_con->cursor_col = parameters[1];
			curr_con->cursor_row = parameters[0];
			if(curr_con->cursor_row >= curr_con->con_rows) curr_con->cursor_row = curr_con->con_rows - 1;
			if(curr_con->cursor_col >= curr_con->con_cols) curr_con->cursor_col = curr_con->con_cols - 1;
			break;
		}
		/////////////////////////////////////////
		// Screen clear
		/////////////////////////////////////////
		case 'J':
		{
			/* different erase modes not yet supported, just clear all */
			__console_clear();
			break;
		}
		/////////////////////////////////////////
		// Line clear
		/////////////////////////////////////////
		case 'K':
		{
			break;
		}
		/////////////////////////////////////////
		// Save cursor position
		/////////////////////////////////////////
		case 's':
		{
			con->saved_col = con->cursor_col;
			con->saved_row = con->cursor_row;
			break;
		}
		/////////////////////////////////////////
		// Load cursor position
		/////////////////////////////////////////
		case 'u':
			con->cursor_col = con->saved_col;
			con->cursor_row = con->saved_row;
			break;
		/////////////////////////////////////////
		// SGR Select Graphic Rendition
		/////////////////////////////////////////
		case 'm':
		{
			// handle 30-37,39 for foreground color changes
			if( (parameters[0] >= 30) && (parameters[0] <= 39) )
			{
				parameters[0] -= 30;

				//39 is the reset code
				if(parameters[0] == 9){
				    parameters[0] = 15;
				}
				else if(parameters[0] > 7){
					parameters[0] = 7;
				}

				if(parameters[1] == 1)
				{
					// Intensity: Bold makes color bright
					parameters[0] += 8;
				}
				con->foreground = color_table[parameters[0]];
			}
			// handle 40-47 for background color changes
			else if( (parameters[0] >= 40) && (parameters[0] <= 47) )
			{
				parameters[0] -= 40;

				if(parameters[1] == 1)
				{
					// Intensity: Bold makes color bright
					parameters[0] += 8;
				}
				con->background = color_table[parameters[0]];
			}
		  break;
		}
	}

	return(i);
}

int __console_write(struct _reent *r,int fd,const char *ptr,size_t len)
{
	int i = 0;
	char *tmp = (char*)ptr;
	console_data_s *con;
	char chr;

	if(__gecko_status>=0) {
		if(__gecko_safe)
			usb_sendbuffer_safe(__gecko_status,ptr,len);
		else
			usb_sendbuffer(__gecko_status,ptr,len);
	}

	if(!curr_con) return -1;
	con = curr_con;
	if(!tmp || len<=0) return -1;

	i = 0;
	while(*tmp!='\0' && i<len)
	{
		chr = *tmp++;
		i++;
		if ( (chr == 0x1b) && (*tmp == '[') )
		{
			/* escape sequence found */
			int k;

			tmp++;
			i++;
			k = __console_parse_escsequence(tmp);
			tmp += k;
			i += k;
		}
		else
		{
			switch(chr)
			{
				case '\n':
					con->cursor_row++;
					con->cursor_col = 0;
					break;
				case '\r':
					con->cursor_col = 0;
					break;
				case '\b':
					con->cursor_col--;
					if(con->cursor_col < 0)
					{
						con->cursor_col = 0;
					}
					break;
				case '\f':
					con->cursor_row++;
					break;
				case '\t':
					if(con->cursor_col%TAB_SIZE) con->cursor_col += (con->cursor_col%TAB_SIZE);
					else con->cursor_col += TAB_SIZE;
					break;
				default:
					__console_drawc(chr);
					con->cursor_col++;

					if( con->cursor_col >= con->con_cols)
					{
						/* if right border reached wrap around */
						con->cursor_row++;
						con->cursor_col = 0;
					}
			}
		}

		if( con->cursor_row >= con->con_rows)
		{
			/* if bottom border reached scroll */
			if (_bg_buffer) {
				_bg_scroll();
			} else {
				memcpy(con->destbuffer,
					con->destbuffer+con->con_stride*(FONT_YSIZE*FONT_YFACTOR+FONT_YGAP),
					con->con_stride*con->con_yres-FONT_YSIZE);

				unsigned int cnt = (con->con_stride * (FONT_YSIZE * FONT_YFACTOR + FONT_YGAP))/4;
				unsigned int *ptr = (unsigned int*)(con->destbuffer + con->con_stride * (con->con_yres - FONT_YSIZE));
				while(cnt--)
					*ptr++ = con->background;
			}
			con->cursor_row--;
		}
	}
	fb_change = 1;

	return i;
}

void CON_Init(void *framebuffer,int xstart,int ystart,int xres,int yres,int stride)
{
	__console_init(framebuffer,xstart,ystart,xres,yres,stride);
}


void _con_free_bg_buff()
{
	if(_bg_buffer) free(_bg_buffer);
	_bg_buffer = NULL;
	if(_c_buffer) free(_c_buffer);
	_c_buffer = NULL;
}

s32 CON_InitEx(GXRModeObj *rmode, s32 conXOrigin,s32 conYOrigin,s32 conWidth,s32 conHeight)
{
	VIDEO_SetPostRetraceCallback(NULL);
	if(_console_buffer)
		free(_console_buffer);
	
	_console_buffer = malloc(conWidth*conHeight*VI_DISPLAY_PIX_SZ);
	if(!_console_buffer) return -1;

	_con_free_bg_buff();

	__console_init_ex(_console_buffer,conXOrigin,conYOrigin,rmode->fbWidth*VI_DISPLAY_PIX_SZ,conWidth,conHeight,conWidth*VI_DISPLAY_PIX_SZ);

	return 0;
}

s32 CON_InitTr(GXRModeObj *rmode, s32 conXOrigin,s32 conYOrigin,s32 conWidth,s32 conHeight, s32 bgColor)
{
	VIDEO_SetPostRetraceCallback(NULL);

	if(_console_buffer) free(_console_buffer);
	_console_buffer = malloc(conWidth*conHeight*VI_DISPLAY_PIX_SZ);
	if(!_console_buffer) return -1;

	_con_free_bg_buff();

	_bg_buffer = malloc(conWidth*conHeight*VI_DISPLAY_PIX_SZ);
	if(!_bg_buffer) return -1;

	_c_buffer_size = sizeof(struct _c1) * (conWidth / FONT_XSIZE) * (conHeight/FONT_YSIZE);
	_c_buffer = malloc(_c_buffer_size);
	if(!_c_buffer) return -1;
	memset(_c_buffer, 0, _c_buffer_size);

	if (bgColor < 0 || bgColor > 15) bgColor = 0;
	_bg_color = color_table[bgColor];

	__console_init_ex(_console_buffer,conXOrigin,conYOrigin,rmode->fbWidth*VI_DISPLAY_PIX_SZ,conWidth,conHeight,conWidth*VI_DISPLAY_PIX_SZ);

	return 0;
}

void CON_GetMetrics(int *cols, int *rows)
{
	if(curr_con) {
		*cols = curr_con->con_cols;
		*rows = curr_con->con_rows;
	}
}

void CON_GetPosition(int *col, int *row)
{
	if(curr_con) {
		*col = curr_con->cursor_col;
		*row = curr_con->cursor_row;
	}
}

void CON_EnableGecko(int channel,int safe)
{
	if(channel && (channel>1 || !usb_isgeckoalive(channel))) channel = -1;

	__gecko_status = channel;
	__gecko_safe = safe;

	if(__gecko_status!=-1) {
		devoptab_list[STD_OUT] = &dotab_stdout;
		devoptab_list[STD_ERR] = &dotab_stdout;

		// line buffered output for threaded apps when only using the usbgecko
		if(!curr_con) {
			setvbuf(stdout, NULL, _IOLBF, 0);
			setvbuf(stderr, NULL, _IOLBF, 0);
		}
	}
}

