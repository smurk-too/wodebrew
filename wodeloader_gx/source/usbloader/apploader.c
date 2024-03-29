#include <stdio.h>
#include <ogcsys.h>
#include <string.h>
#include <malloc.h>

#include "patches/patchcode.h"
#include "apploader.h"
#include "wdvd.h"
#include "wpad.h"
#include "disc.h"
#include "fstfile.h"
#include "settings/cfg.h"
#include "gecko.h"
#include "patches/wip.h"
#include "sys.h"

extern bool geckoinit;

/* Apploader function pointers */
typedef int   (*app_main)(void **dst, int *size, int *offset);
typedef void  (*app_init)(void (*report)(const char *fmt, ...));
typedef void *(*app_final)();
typedef void  (*app_entry)(void (**init)(void (*report)(const char *fmt, ...)), int (**main)(), void *(**final)());

/* Apploader pointers */
static u8 *appldr = (u8 *)0x81200000;


/* Constants */
#define APPLDR_OFFSET	0x2440

/* Variables */
static u32 buffer[0x20] ATTRIBUTE_ALIGN(32);
struct SSettings Settings;

bool compare_videomodes(GXRModeObj* mode1, GXRModeObj* mode2) {
    if (mode1->viTVMode != mode2->viTVMode || mode1->fbWidth != mode2->fbWidth ||	mode1->efbHeight != mode2->efbHeight || mode1->xfbHeight != mode2->xfbHeight ||
            mode1->viXOrigin != mode2->viXOrigin || mode1->viYOrigin != mode2->viYOrigin || mode1->viWidth != mode2->viWidth || mode1->viHeight != mode2->viHeight ||
            mode1->xfbMode != mode2->xfbMode || mode1->field_rendering != mode2->field_rendering || mode1->aa != mode2->aa || mode1->sample_pattern[0][0] != mode2->sample_pattern[0][0] ||
            mode1->sample_pattern[1][0] != mode2->sample_pattern[1][0] ||	mode1->sample_pattern[2][0] != mode2->sample_pattern[2][0] ||
            mode1->sample_pattern[3][0] != mode2->sample_pattern[3][0] ||	mode1->sample_pattern[4][0] != mode2->sample_pattern[4][0] ||
            mode1->sample_pattern[5][0] != mode2->sample_pattern[5][0] ||	mode1->sample_pattern[6][0] != mode2->sample_pattern[6][0] ||
            mode1->sample_pattern[7][0] != mode2->sample_pattern[7][0] ||	mode1->sample_pattern[8][0] != mode2->sample_pattern[8][0] ||
            mode1->sample_pattern[9][0] != mode2->sample_pattern[9][0] ||	mode1->sample_pattern[10][0] != mode2->sample_pattern[10][0] ||
            mode1->sample_pattern[11][0] != mode2->sample_pattern[11][0] || mode1->sample_pattern[0][1] != mode2->sample_pattern[0][1] ||
            mode1->sample_pattern[1][1] != mode2->sample_pattern[1][1] ||	mode1->sample_pattern[2][1] != mode2->sample_pattern[2][1] ||
            mode1->sample_pattern[3][1] != mode2->sample_pattern[3][1] ||	mode1->sample_pattern[4][1] != mode2->sample_pattern[4][1] ||
            mode1->sample_pattern[5][1] != mode2->sample_pattern[5][1] ||	mode1->sample_pattern[6][1] != mode2->sample_pattern[6][1] ||
            mode1->sample_pattern[7][1] != mode2->sample_pattern[7][1] ||	mode1->sample_pattern[8][1] != mode2->sample_pattern[8][1] ||
            mode1->sample_pattern[9][1] != mode2->sample_pattern[9][1] ||	mode1->sample_pattern[10][1] != mode2->sample_pattern[10][1] ||
            mode1->sample_pattern[11][1] != mode2->sample_pattern[11][1] || mode1->vfilter[0] != mode2->vfilter[0] ||
            mode1->vfilter[1] != mode2->vfilter[1] ||	mode1->vfilter[2] != mode2->vfilter[2] || mode1->vfilter[3] != mode2->vfilter[3] ||	mode1->vfilter[4] != mode2->vfilter[4] ||
            mode1->vfilter[5] != mode2->vfilter[5] ||	mode1->vfilter[6] != mode2->vfilter[6] ) {
        return false;
    } else {
        return true;
    }
}


void patch_videomode(GXRModeObj* mode1, GXRModeObj* mode2) {
    mode1->viTVMode = mode2->viTVMode;
    mode1->fbWidth = mode2->fbWidth;
    mode1->efbHeight = mode2->efbHeight;
    mode1->xfbHeight = mode2->xfbHeight;
    mode1->viXOrigin = mode2->viXOrigin;
    mode1->viYOrigin = mode2->viYOrigin;
    mode1->viWidth = mode2->viWidth;
    mode1->viHeight = mode2->viHeight;
    mode1->xfbMode = mode2->xfbMode;
    mode1->field_rendering = mode2->field_rendering;
    mode1->aa = mode2->aa;
    mode1->sample_pattern[0][0] = mode2->sample_pattern[0][0];
    mode1->sample_pattern[1][0] = mode2->sample_pattern[1][0];
    mode1->sample_pattern[2][0] = mode2->sample_pattern[2][0];
    mode1->sample_pattern[3][0] = mode2->sample_pattern[3][0];
    mode1->sample_pattern[4][0] = mode2->sample_pattern[4][0];
    mode1->sample_pattern[5][0] = mode2->sample_pattern[5][0];
    mode1->sample_pattern[6][0] = mode2->sample_pattern[6][0];
    mode1->sample_pattern[7][0] = mode2->sample_pattern[7][0];
    mode1->sample_pattern[8][0] = mode2->sample_pattern[8][0];
    mode1->sample_pattern[9][0] = mode2->sample_pattern[9][0];
    mode1->sample_pattern[10][0] = mode2->sample_pattern[10][0];
    mode1->sample_pattern[11][0] = mode2->sample_pattern[11][0];
    mode1->sample_pattern[0][1] = mode2->sample_pattern[0][1];
    mode1->sample_pattern[1][1] = mode2->sample_pattern[1][1];
    mode1->sample_pattern[2][1] = mode2->sample_pattern[2][1];
    mode1->sample_pattern[3][1] = mode2->sample_pattern[3][1];
    mode1->sample_pattern[4][1] = mode2->sample_pattern[4][1];
    mode1->sample_pattern[5][1] = mode2->sample_pattern[5][1];
    mode1->sample_pattern[6][1] = mode2->sample_pattern[6][1];
    mode1->sample_pattern[7][1] = mode2->sample_pattern[7][1];
    mode1->sample_pattern[8][1] = mode2->sample_pattern[8][1];
    mode1->sample_pattern[9][1] = mode2->sample_pattern[9][1];
    mode1->sample_pattern[10][1] = mode2->sample_pattern[10][1];
    mode1->sample_pattern[11][1] = mode2->sample_pattern[11][1];
    mode1->vfilter[0] = mode2->vfilter[0];
    mode1->vfilter[1] = mode2->vfilter[1];
    mode1->vfilter[2] = mode2->vfilter[2];
    mode1->vfilter[3] = mode2->vfilter[3];
    mode1->vfilter[4] = mode2->vfilter[4];
    mode1->vfilter[5] = mode2->vfilter[5];
    mode1->vfilter[6] = mode2->vfilter[6];
}

GXRModeObj* vmodes[] = {
    &TVNtsc240Ds,
    &TVNtsc240DsAa,
    &TVNtsc240Int,
    &TVNtsc240IntAa,
    &TVNtsc480IntDf,
    &TVNtsc480IntAa,
    &TVNtsc480Prog,
    &TVMpal480IntDf,
    &TVPal264Ds,
    &TVPal264DsAa,
    &TVPal264Int,
    &TVPal264IntAa,
    &TVPal524IntAa,
    &TVPal528Int,
    &TVPal528IntDf,
    &TVPal574IntDfScale,
    &TVEurgb60Hz240Ds,
    &TVEurgb60Hz240DsAa,
    &TVEurgb60Hz240Int,
    &TVEurgb60Hz240IntAa,
    &TVEurgb60Hz480Int,
    &TVEurgb60Hz480IntDf,
    &TVEurgb60Hz480IntAa,
    &TVEurgb60Hz480Prog,
    &TVEurgb60Hz480ProgSoft,
    &TVEurgb60Hz480ProgAa
};

GXRModeObj* PAL2NTSC[]={
    &TVMpal480IntDf,		&TVNtsc480IntDf,
    &TVPal264Ds,			&TVNtsc240Ds,
    &TVPal264DsAa,			&TVNtsc240DsAa,
    &TVPal264Int,			&TVNtsc240Int,
    &TVPal264IntAa,			&TVNtsc240IntAa,
    &TVPal524IntAa,			&TVNtsc480IntAa,
    &TVPal528Int,			&TVNtsc480IntAa,
    &TVPal528IntDf,			&TVNtsc480IntDf,
    &TVPal574IntDfScale,	&TVNtsc480IntDf,
    &TVEurgb60Hz240Ds,		&TVNtsc240Ds,
    &TVEurgb60Hz240DsAa,	&TVNtsc240DsAa,
    &TVEurgb60Hz240Int,		&TVNtsc240Int,
    &TVEurgb60Hz240IntAa,	&TVNtsc240IntAa,
    &TVEurgb60Hz480Int,		&TVNtsc480IntAa,
    &TVEurgb60Hz480IntDf,	&TVNtsc480IntDf,
    &TVEurgb60Hz480IntAa,	&TVNtsc480IntAa,
    &TVEurgb60Hz480Prog,	&TVNtsc480Prog,
    &TVEurgb60Hz480ProgSoft,&TVNtsc480Prog,
    &TVEurgb60Hz480ProgAa,  &TVNtsc480Prog,
    0,0
};

GXRModeObj* NTSC2PAL[]={
    &TVNtsc240Ds,			&TVPal264Ds,
    &TVNtsc240DsAa,			&TVPal264DsAa,
    &TVNtsc240Int,			&TVPal264Int,
    &TVNtsc240IntAa,		&TVPal264IntAa,
    &TVNtsc480IntDf,		&TVPal528IntDf,
    &TVNtsc480IntAa,		&TVPal524IntAa,
    &TVNtsc480Prog,			&TVPal528IntDf,
    0,0
};

GXRModeObj* NTSC2PAL60[]={
    &TVNtsc240Ds,			&TVEurgb60Hz240Ds,
    &TVNtsc240DsAa,			&TVEurgb60Hz240DsAa,
    &TVNtsc240Int,			&TVEurgb60Hz240Int,
    &TVNtsc240IntAa,		&TVEurgb60Hz240IntAa,
    &TVNtsc480IntDf,		&TVEurgb60Hz480IntDf,
    &TVNtsc480IntAa,		&TVEurgb60Hz480IntAa,
    &TVNtsc480Prog,			&TVEurgb60Hz480Prog,
    0,0
};
bool Search_and_patch_Video_Modes(void *Address, u32 Size, GXRModeObj* Table[]) {
    u8 *Addr = (u8 *)Address;
    bool found = 0;
    u32 i;

    while (Size >= sizeof(GXRModeObj)) {



        for (i = 0; Table[i]; i+=2) {


            if (compare_videomodes(Table[i], (GXRModeObj*)Addr))

            {
                found = 1;
                patch_videomode((GXRModeObj*)Addr, Table[i+1]);
                Addr += (sizeof(GXRModeObj)-4);
                Size -= (sizeof(GXRModeObj)-4);
                break;
            }
        }

        Addr += 4;
        Size -= 4;
    }


    return found;
}

void gamepatches(void * dst, int len, u8 videoSelected, u8 patchcountrystring, u8 vipatch, u8 cheat) {

	GXRModeObj** table = NULL;
    if (videoSelected == 5) // patch

    {
        switch (CONF_GetVideo()) {
        case CONF_VIDEO_PAL:
            if (CONF_GetEuRGB60() > 0) {
                table = NTSC2PAL60;
            } else {
                table = NTSC2PAL;
            }
            break;

        case CONF_VIDEO_MPAL:



            table = NTSC2PAL;
            break;


        default:
            table = PAL2NTSC;
            break;
        }
        Search_and_patch_Video_Modes(dst, len, table);
    }

	if(cheat)
		dogamehooks(dst,len);

    if (vipatch)
	vidolpatcher(dst,len);


    /*LANGUAGE PATCH - FISHEARS*/
    langpatcher(dst,len);

    /*Thanks to WiiPower*/
    if (patchcountrystring == 1)
        PatchCountryStrings(dst, len);

	do_wip_code((u8 *)0x80000000);

	//patchdebug(dst, len);

}

s32 Apploader_Run(entry_point *entry, u8 cheat, u8 videoSelected, u8 vipatch, u8 patchcountrystring, u32 rtrn) {
    app_entry appldr_entry;
    app_init  appldr_init;
    app_main  appldr_main;
    app_final appldr_final;

    u32 appldr_len;
    s32 ret;
	gprintf("Apploader_Run( %p, %d, %d, %d, %d, %08x )\n", \
		entry, cheat, videoSelected, vipatch, patchcountrystring, rtrn );

	//u32 geckoattached = usb_isgeckoalive(EXI_CHANNEL_1);
	//if (geckoattached)usb_flush(EXI_CHANNEL_1);
	geckoinit = InitGecko();

    /* Read apploader header */
    ret = WDVD_Read(buffer, 0x20, APPLDR_OFFSET);
    if (ret < 0)
        return ret;

    /* Calculate apploader length */
    appldr_len = buffer[5] + buffer[6];

    /* Read apploader code */
    ret = WDVD_Read(appldr, appldr_len, APPLDR_OFFSET + 0x20);
    if (ret < 0)
        return ret;

	/* Set apploader entry function */
    appldr_entry = (app_entry)buffer[4];

    /* Call apploader entry */
    appldr_entry(&appldr_init, &appldr_main, &appldr_final);

    /* Initialize apploader */
    appldr_init( gprintf );

    u32 dolStart = 0x90000000;
    u32 dolEnd = 0x0;

    for (;;) {
        void *dst = NULL;
        int len = 0, offset = 0;

        /* Run apploader main function */
        ret = appldr_main(&dst, &len, &offset);
        if (!ret)
            break;

        /* Read data from DVD */
        WDVD_Read(dst, len, (u64)(offset << 2));

	gamepatches(dst, len, videoSelected, patchcountrystring, vipatch, cheat);

    DCFlushRange(dst, len);
	if( (u32)dst < dolStart )dolStart = (u32)dst;
	if( (u32)dst + len > dolEnd ) dolEnd = (u32)dst + len;
    }

    //this patch should be run on the entire dol at 1 time
    if(rtrn)
    {
		if( PatchReturnTo( (u32*)dolStart, dolEnd - dolStart , rtrn) )
		{
			//gprintf("return-to patched\n" );
			DCFlushRange( (u32*)dolStart, dolEnd - dolStart );
		}
    }

    *entry = appldr_final();

    return 0;
}
