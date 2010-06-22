#ifndef _SYS_H_
#define _SYS_H_

#ifdef __cplusplus
extern "C" {
#endif

void wiilight(int enable);

/* Prototypes */
void Sys_Init(void);
void Sys_Reboot(void);
void Sys_Shutdown(void);
void Sys_ShutdownToIdel(void);
void Sys_ShutdownToStandby(void);
void Sys_LoadMenu(void);
void Sys_BackToLoader(void);

void ScreenShot();

void ShowMemInfo();

#ifdef __cplusplus
}
#endif

#endif

