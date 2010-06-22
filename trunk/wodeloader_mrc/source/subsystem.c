#include <stdio.h>
#include <ogcsys.h>

#include "fat.h"
#include "wpad.h"


void Subsystem_Init(void)
{
	/* Initialize Wiimote subsystem */
	//lo comento para evitar el error raro
	//se llama despues de WBFS_Init
	//Wpad_Init();

	/* Mount SDHC */
}

void Subsystem_Close(void)
{
	/* Disconnect Wiimotes */
	Wpad_Disconnect();

	/* Unmount SDHC */
	Fat_UnmountSDHC();
}
