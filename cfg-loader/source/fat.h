
// Modified by oggzee

#ifndef _FAT_H_
#define _FAT_H_

/* Prototypes */
s32 Fat_MountSDHC(void);
s32 Fat_UnmountSDHC(void);
s32 Fat_ReadFile(const char *, void **);
s32 Fat_MountUSB(void);
s32 Fat_UnmountUSB(void);

void Fat_UnmountAll();
void Fat_print_sd_mode();

extern int   fat_sd_mount;
extern sec_t fat_sd_sec;
extern int   fat_usb_mount;
extern sec_t fat_usb_sec;

#endif
