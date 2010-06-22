/****************************************************************************
 * USB Loader GX Team
 *
 * Main loadup of the application
 *
 * libwiigui
 * Tantric 2009
 ***************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <ogcsys.h>
#include <unistd.h>
#include <locale.h>
#include <wiiuse/wpad.h>
#include <ogc/libversion.h>
//#include <debug.h>
extern "C"
{
    extern void __exception_setreload(int t);
}


#include <di/di.h>
#include <sys/iosupport.h>

#include "libwiigui/gui.h"
#include "usbloader/wbfs.h"
#include "settings/cfg.h"
#include "language/gettext.h"
#include "FreeTypeGX.h"
#include "video.h"
#include "audio.h"
#include "menu.h"
#include "input.h"
#include "filelist.h"
#include "listfiles.h"
#include "main.h"
#include "fatmounter.h"
#include "sys.h"
#include "wpad.h"
#include "fat.h"
#include "gecko.h"
#include "svnrev.h"
#include "wad/title.h"
#include "memory/mem2.h"
#include "lstub.h"
#include "usbloader/wode.h"

extern bool geckoinit;
extern bool textVideoInit;
extern char headlessID[8];
extern char game_partition[6];

/* Constants */
#define CONSOLE_XCOORD      260
#define CONSOLE_YCOORD      115
#define CONSOLE_WIDTH       340
#define CONSOLE_HEIGHT      218

FreeTypeGX *fontSystem=0;
FreeTypeGX *fontClock=0;

u8 dbvideo =0;

unsigned int *xfb = NULL;

void InitTextVideo ()
{
    gprintf("\n\nInitTextVideo ()");
    if (textVideoInit)
    {
	gprintf("...0\n");
        return;
    }
    dbvideo=1;
    VIDEO_Init();
	
	// get default video mode
    GXRModeObj *vmode = VIDEO_GetPreferredMode(NULL);

    // widescreen fix
    VIDEO_Configure (vmode);

    // Allocate the video buffers
    xfb = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));

    // A console is always useful while debugging
    console_init (xfb, 20, 64, vmode->fbWidth, vmode->xfbHeight, vmode->fbWidth * 2);

    // Clear framebuffers etc.
    VIDEO_ClearFrameBuffer (vmode, xfb, COLOR_BLACK);
    VIDEO_SetNextFramebuffer (xfb);

    VIDEO_SetBlack (FALSE);
    VIDEO_Flush ();
    VIDEO_WaitVSync ();
    if (vmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync ();

    //send console output to the gecko
    if (geckoinit)CON_EnableGecko(1, true);
    textVideoInit = true;
    gprintf("...1\n");

}

bool file_exists(char *path) {
	FILE *fp = fopen(path, "rb");
	if (fp) {
		fclose(fp);
		return true;
	}
	return false;
}

int
main(int argc, char *argv[])
{
    setlocale(LC_ALL, "en.UTF-8");
    geckoinit = InitGecko();
	
	use_dvdx = 1;
	InitDVD();

    if (hbcStubAvailable() || geckoinit)
    {
        InitTextVideo();
    }

    //	DEBUG_Init(GDBSTUB_DEVICE_USB, 1);
    //_break();

    __exception_setreload(5);                     //auto reset code dump nobody gives us codedump info anyways.

    gprintf("------------------\n");
    gprintf("USB Loader GX rev%s\n",GetRev());
    gprintf("<< %s >>\n", _V_STRING );
    gprintf("main(%d", argc);
    for (int i=0;i<argc;i++)
        gprintf(", %s",argv[i]?argv[i]:"<NULL>");
    gprintf(")\n");

    printf("Starting up\n");

    MEM2_init(36);                                // Initialize 36 MB
    MEM2_takeBigOnes(true);

    s32 ret;

    printf("\n\tInitializing controllers");

    /** PAD_Init has to be before InitVideo don't move that **/
    PAD_Init();                                   // initialize PAD/WPAD

    printf("\n\tInit wbfs...");
    ret = WBFS_Init();
    printf("%d", ret);

    printf("\n\tInitialize sd card");
    SDCard_Init();                                // mount SD for loading cfg's
	USBDevice_Init();

    // Try opening and closing the configuration file here
    // to prevent a crash dump later on - giantpune
	
	if (fat_usb_mount != 0 && file_exists((char *) "USB:/config/WGXGlobal.cfg")) {
		sprintf(bootDevice, "USB:");
	} else if (fat_sd_mount != 0 && file_exists((char *) "SD:/config/WGXGlobal.cfg")) {
		sprintf(bootDevice, "SD:");
	} else {
		sprintf(bootDevice, fat_usb_mount != 0 ? "USB:" : "SD:");
	}
	
    char GXGlobal_cfg[26];
    sprintf(GXGlobal_cfg, "%s/config/WGXGlobal.cfg", bootDevice);
    FILE *fp = fopen(GXGlobal_cfg, "r");
    if (fp)
    {
        fclose(fp);
    }

    gettextCleanUp();
    printf("\n\tLoading configuration...");
    CFG_Load();
    printf("done");
    //	gprintf("\n\tbootDevice = %s",bootDevice);

    //if a ID was passed via args copy it and try to boot it after the partition is mounted
    //its not really a headless mode.  more like hairless.
    if (argc > 1 && argv[1])
    {
        if (strlen(argv[1])==6)
            strncpy(headlessID, argv[1], sizeof(headlessID));
    }

    //! Init the rest of the System
    Sys_Init();
    Wpad_Init();
	InitVideo();
    InitAudio();                                  // Initialize audio

    WPAD_SetDataFormat(WPAD_CHAN_ALL,WPAD_FMT_BTNS_ACC_IR);
    WPAD_SetVRes(WPAD_CHAN_ALL, screenwidth, screenheight);

	WBFS_OpenPart(Settings.partition, (char *) &game_partition); // Open WBFS partition

    // load main font from file, or default to built-in font
    fontSystem = new FreeTypeGX();
    char *fontPath = NULL;
    asprintf(&fontPath, "%sfont.ttf", CFG.theme_path);
    fontSystem->loadFont(fontPath, font_ttf, font_ttf_size, 0);
    fontSystem->setCompatibilityMode(FTGX_COMPATIBILITY_DEFAULT_TEVOP_GX_PASSCLR | FTGX_COMPATIBILITY_DEFAULT_VTXDESC_GX_NONE);
    free(fontPath);

    fontClock = new FreeTypeGX();
    fontClock->loadFont(NULL, clock_ttf, clock_ttf_size, 0);
    fontClock->setCompatibilityMode(FTGX_COMPATIBILITY_DEFAULT_TEVOP_GX_PASSCLR | FTGX_COMPATIBILITY_DEFAULT_VTXDESC_GX_NONE);

    gprintf("\n\tEnd of Main()");
    InitGUIThreads();
    MainMenu(MENU_DISCLIST);
	
	LaunchISO(0, 0);
	CloseWode();
	
    return 0;
}
