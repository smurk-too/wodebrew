#ifndef _FATMOUNTER_H_
#define _FATMOUNTER_H_

#ifdef __cplusplus
extern "C" {
#endif

	extern int   fat_sd_mount;
	extern int   fat_usb_mount;

    int isInserted(const char *path);
    int SDCard_Init();
    void SDCard_deInit();
    int USBDevice_Init();
    void USBDevice_deInit();

#ifdef __cplusplus
}
#endif

#endif
