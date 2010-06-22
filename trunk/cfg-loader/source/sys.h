#ifndef _SYS_H_
#define _SYS_H_

/* Prototypes */
void Sys_Init(void);
void Sys_Reboot(void);
void Sys_Shutdown(void);
void Sys_LoadMenu(void);
void Sys_Exit(void);
void Sys_HBC();
void Sys_Channel(u32 channel);

s32  Sys_GetCerts(signed_blob **, u32 *);

#endif
