#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <unistd.h>
#include <string.h>

#include "disc.h"
#include "gui.h"
#include "menu.h"
#include "restart.h"
#include "subsystem.h"
#include "sys.h"
#include "video.h"
#include "cfg.h"
#include "wpad.h"
#include "fat.h"
#include "util.h"
#include "gettext.h"
#include "mload.h"
#include "wode.h"
#include "wdvd.h"
#include "wbfs.h"
#include "gecko.h"

extern int __console_disable;
extern bool geckoinit;

int main(int argc, char **argv)
{
	s32 ret;

	use_dvdx = 1;
	geckoinit = InitGecko();
	
	InitDVD();
	
	/* Initialize system */
	mem_init();
	Sys_Init();

	cfg_parsearg_early(argc, argv);

	/* Set video mode */
	Video_SetMode();

	Gui_DrawIntro();

	/* Initialize subsystems */
	//Subsystem_Init();
	// delay wpad_init after second reloadIOS
	dbg_printf("Fat Mount SD\n");
	Fat_MountSDHC();
	Fat_MountUSB();

	//save_dip();

	/* Load configuration */
	dbg_printf("CFG Load\n");
	CFG_Load(argc, argv);

	__console_disable = 1;

   	// set widescreen as set by CFG.widescreen
	Video_SetWide();

	util_init();

	// Gui Init
	Gui_Init();

	// Show background
	//Gui_DrawBackground();
	Gui_LoadBackground();

	/* Initialize console */
	Gui_InitConsole();

	/* Initialize Wiimote subsystem */
	Wpad_Init();

	/* Set up config parameters */
	//printf("CFG_Setup\n"); //Wpad_WaitButtons();
	CFG_Setup(argc, argv);

	/* Initialize DIP module */
	ret = Disc_Init();
	if (ret < 0) {
		Gui_Console_Enable();
		printf_x(gt("ERROR:"));
		printf("\n");
		printf_(gt("Could not initialize DIP module! (ret = %d)"), ret);
		printf("\n");
		goto out;
	}
	
	u32 status = 0;
	if (WDVD_GetCoverStatus(&status) != 0 || (status & 2) == 0) {
		// WDVD_WaitForDisc();

		printf("Please mount a disc image with the wode\n");
		do
		{
			WPAD_ScanPads();
			s32 padsState = WPAD_ButtonsDown(0);
			if ((padsState & WPAD_BUTTON_B) != 0)
				break;
			usleep(100 * 1000);
		} while (WDVD_GetCoverStatus(&status) != 0 || (status & 2) == 0);
		if ((status & 2) == 0) return -2;
		
		InitDVD();
	}
	WBFS_Init();
	CFG_Partition_Default();
	
	// Mount partition
	WBFS_OpenNamed(CFG.partition);
	__Menu_GetEntries();

	/* Menu loop */
	//dbg_printf("Menu_Loop\n"); //Wpad_WaitButtons();
	Menu_Loop();

out:
	LaunchISO(0, 0);
	CloseWode();
	sleep(8);
	/* Restart */
	exit(0);
	Restart_Wait();

	return 0;
}
