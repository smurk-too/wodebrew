#include <stdio.h>
#include <ogcsys.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>

#include "settings/cfg.h"
#include "usbloader/wdvd.h"
#include "usbloader/disc.h"
#include "usbloader/wbfs.h"
#include "usbloader/wode.h"
#include "fatmounter.h"
#include "sys.h"
#include "wpad.h"

extern char game_partition[6];
extern u8 load_from_fs;

extern int screenheight;
extern int screenwidth;

//Wiilight stuff
static vu32 *_wiilight_reg = (u32*)0xCD0000C0;

void wiilight(int enable) {             // Toggle wiilight (thanks Bool for wiilight source)
    u32 val = (*_wiilight_reg&~0x20);
    if (enable && Settings.wiilight) val |= 0x20;
    *_wiilight_reg=val;
}

/* Variables */
u8 shutdown = 0;
u8 reset = 0;

void __Sys_ResetCallback(void) {
    /* Reboot console */
    reset = 1;
}

void __Sys_PowerCallback(void) {
    /* Poweroff console */
    shutdown = 1;
}

void Sys_Init(void) {
    /* Initialize video subsytem */
    //VIDEO_Init();

    /* Set RESET/POWER button callback */
    SYS_SetResetCallback(__Sys_ResetCallback);
    SYS_SetPowerCallback(__Sys_PowerCallback);
}

void ExitGUIThreads(void);
void StopAudio();
void StopGX();

static void _ExitApp() {
    ExitGUIThreads();
    StopGX();
    StopAudio();

    SDCard_deInit();
	
	LaunchISO(0, 0);
	CloseWode();
}

void Sys_Reboot(void) {
    /* Restart console */
    _ExitApp();
    STM_RebootSystem();
}

#define ShutdownToDefault	0
#define ShutdownToIdle		1
#define ShutdownToStandby	2

static void _Sys_Shutdown(int SHUTDOWN_MODE) {
    _ExitApp();
    WPAD_Flush(0);
    WPAD_Disconnect(0);
    WPAD_Shutdown();

    /* Poweroff console */
    if ((CONF_GetShutdownMode() == CONF_SHUTDOWN_IDLE &&  SHUTDOWN_MODE != ShutdownToStandby) || SHUTDOWN_MODE == ShutdownToIdle) {
        s32 ret;

        /* Set LED mode */
        ret = CONF_GetIdleLedMode();
        if (ret >= 0 && ret <= 2)
            STM_SetLedMode(ret);

        /* Shutdown to idle */
        STM_ShutdownToIdle();
    } else {
        /* Shutdown to standby */
        STM_ShutdownToStandby();
    }
}
void Sys_Shutdown(void) {
    _Sys_Shutdown(ShutdownToDefault);
}
void Sys_ShutdownToIdel(void) {
    _Sys_Shutdown(ShutdownToIdle);
}
void Sys_ShutdownToStandby(void) {
    _Sys_Shutdown(ShutdownToStandby);
}

void Sys_LoadMenu(void) {
    _ExitApp();
    /* Return to the Wii system menu */
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}

void Sys_BackToLoader(void) {
    if (*((u32*) 0x80001800)) {
        _ExitApp();
        exit(0);
    }
    // Channel Version
    Sys_LoadMenu();
}

bool Sys_IsHermes() {
	return IOS_GetVersion() == 222 || IOS_GetVersion() == 223;
}

// #include "prompts/PromptWindows.h"

void ShowMemInfo() {
	char buf[255];
    struct mallinfo mymallinfo = mallinfo();
    sprintf((char *) &buf,"Total: %d, Used: %d, Can be freed: %d", mymallinfo.arena/1024, mymallinfo.uordblks/1024, mymallinfo.keepcost/1024);
//	WindowPrompt("Mem info", (char *) &buf, "OK");
}

#include <time.h>

s32 TakeScreenshot(const char *path);

void ScreenShot()
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];
   char buffer2 [80];

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  //USBLoader_GX_ScreenShot-Month_Day_Hour_Minute_Second_Year.png
  strftime (buffer,80,"USBLoader_GX_ScreenShot-%b%d%H%M%S%y.png",timeinfo);
   sprintf(buffer2, "%s/config/%s", bootDevice, buffer);

  TakeScreenshot(buffer2);
}
