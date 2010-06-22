#include <stdio.h>
#include <ogcsys.h>

#include "sys.h"
#include "wpad.h"

//Constants
#define MAX_WIIMOTES	4


#if 0
void __Wpad_PowerCallback(s32 chan)
{
	/* Poweroff console */
	Sys_Shutdown();
}
#endif


s32 Wpad_Init(void)
{
	s32 ret;

	//Initialize Wiimotes
	ret = WPAD_Init();

	if (ret < 0)
		return ret;

	//Initialize GC pads
	ret = PAD_Init();
	if (ret < 0)
		return ret;

#if 0
	/* Set POWER button callback */
	WPAD_SetPowerButtonCallback(__Wpad_PowerCallback);
#endif

	return ret;
}

void Wpad_Disconnect(void)
{
	u32 cnt;

	/* Disconnect Wiimotes */
	for (cnt = 0; cnt < MAX_WIIMOTES; cnt++)
		WPAD_Disconnect(cnt);

	/* Shutdown Wiimote subsystem */
	WPAD_Shutdown();
}


u32 Wpad_WaitButtons(void){
	u32 buttons=0;

	for(;;){
		//Obtener botones pulsados de Wiimotes
		buttons=0;
		WPAD_ScanPads();
		//for (cnt = 0; cnt < MAX_WIIMOTES; cnt++)
		//	buttons |= WPAD_ButtonsDown(cnt);
		buttons=WPAD_ButtonsDown(0);
		if(buttons){
			return buttons;
		}
		VIDEO_WaitVSync();

		//Obtener botones pulsados de mandos GC
		buttons=0;
		PAD_ScanPads();
		buttons=PAD_ButtonsDown(0);
		if(buttons){
			if(buttons & PAD_BUTTON_UP)
				return WPAD_BUTTON_UP;
			else if(buttons & PAD_BUTTON_DOWN)
				return WPAD_BUTTON_DOWN;
			else if(buttons & PAD_BUTTON_LEFT)
				return WPAD_BUTTON_LEFT;
			else if(buttons & PAD_BUTTON_RIGHT)
				return WPAD_BUTTON_RIGHT;
			else if(buttons & PAD_BUTTON_A)
				return WPAD_BUTTON_A;
			else if(buttons & PAD_BUTTON_B)
				return WPAD_BUTTON_B;
			else if(buttons & PAD_BUTTON_Y)
				return WPAD_BUTTON_1;
			else if(buttons & PAD_BUTTON_X)
				return WPAD_BUTTON_2;
			else if(buttons & PAD_TRIGGER_R)
				return WPAD_BUTTON_PLUS;
			else if(buttons & PAD_TRIGGER_L)
				return WPAD_BUTTON_MINUS;
			else if(buttons & PAD_BUTTON_START)
				return WPAD_BUTTON_HOME;
		}
		VIDEO_WaitVSync();

	}
}
