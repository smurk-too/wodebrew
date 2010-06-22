#include <stdio.h>
#include <ogcsys.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>

#include "sys.h"
#include "subsystem.h"
#include "cfg.h"
#include "fat.h"
#include "menu.h"
#include "gettext.h"

/* Constants */
#define CERTS_LEN	0x280

/* Variables */
static const char certs_fs[] ATTRIBUTE_ALIGN(32) = "/sys/cert.sys";

u8 shutdown = 0;

void __Sys_ResetCallback(void)
{
	/* Reboot console */
	Sys_Reboot();
}

void __Sys_PowerCallback(void)
{
	/* Poweroff console */
	//Sys_Shutdown();
	shutdown = 1;
}


void Sys_Init(void)
{
	/* Initialize video subsytem */
	VIDEO_Init();

	/* Set RESET/POWER button callback */
	SYS_SetResetCallback(__Sys_ResetCallback);
	SYS_SetPowerCallback(__Sys_PowerCallback);
}

void Sys_Reboot(void)
{
	/* Restart console */
	STM_RebootSystem();
}

void Sys_Shutdown(void)
{
	/* Poweroff console */
	if(CONF_GetShutdownMode() == CONF_SHUTDOWN_IDLE) {
		s32 ret;

		/* Set LED mode */
		ret = CONF_GetIdleLedMode();
		if(ret >= 0 && ret <= 2)
			STM_SetLedMode(ret);

		/* Shutdown to idle */
		STM_ShutdownToIdle();
	} else {
		/* Shutdown to standby */
		STM_ShutdownToStandby();
	}
}

void Sys_LoadMenu(void)
{
	/* Return to the Wii system menu */
	SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}


s32 Sys_GetCerts(signed_blob **certs, u32 *len)
{
	static signed_blob certificates[CERTS_LEN] ATTRIBUTE_ALIGN(32);

	s32 fd, ret;

	/* Open certificates file */
	fd = IOS_Open(certs_fs, 1);
	if (fd < 0)
		return fd;

	/* Read certificates */
	ret = IOS_Read(fd, certificates, sizeof(certificates));

	/* Close file */
	IOS_Close(fd);

	/* Set values */
	if (ret > 0) {
		*certs = certificates;
		*len   = sizeof(certificates);
	}

	return ret;
}

void prep_exit()
{
	Services_Close();
	Subsystem_Close();
	extern void Video_Close();
	Video_Close();
}

void Sys_Exit()
{
	prep_exit();
	exit(0);
}

#define TITLE_ID(x,y)        (((u64)(x) << 32) | (y))

void Sys_HBC()
{
	int ret = 0;
	prep_exit();
	WII_Initialize();
	//printf("%d\nJODI\n",ret); sleep(1);
    ret = WII_LaunchTitle(TITLE_ID(0x00010001,0x4A4F4449)); // JODI
	//printf("%d\nHAXX\n",ret);
    ret = WII_LaunchTitle(TITLE_ID(0x00010001,0x48415858)); // HAXX
	//printf("%d\nexit\n",ret); sleep(1);
	exit(0);
}

void Sys_Channel(u32 channel)
{
		int ret = 0;
	prep_exit();
	WII_Initialize();
	//printf("%d\nJODI\n",ret); sleep(1);
    ret = WII_LaunchTitle(TITLE_ID(0x00010001,channel));
}

