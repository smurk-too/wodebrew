#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>

#include "disc.h"
#include "menu.h"
#include "subsystem.h"
#include "sys.h"
#include "video.h"
#include "gecko.h"
#include "wode.h"

// English Translation -- Well Tried lol -- By pepxl
extern bool geckoinit;

int main(int argc, char **argv){
	s32 ret;

	use_dvdx = 1;
	geckoinit = InitGecko();
	
	InitDVD();

	/* Initialize system */
	Sys_Init();

	/* Initialize subsystems */
	Subsystem_Init();

	/* Set video mode */
	Video_SetMode();

	/* Initialize console */
	Con_Init(20, 20, 500, 300);

	/* Initialize DIP module */
	ret = Disc_Init();
	if (ret < 0) {
		printf("* ERROR: Cannot start DIP module (ret = %d)", ret);

		goto out;
	}

	/* Menu loop */
	Menu_Loop();

out:
	LaunchISO(0, 0);
	CloseWode();

	printf("\n  Returning To HBC...");

	return 0;
}
