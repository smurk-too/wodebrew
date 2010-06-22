#include <string.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/mutex.h>
#include <ogc/system.h>
#include <ogc/usbstorage.h>
#include <sdcard/wiisd_io.h>
#include <locale.h>
#include <fat.h>
#include <malloc.h>
#include <ogc/usbstorage.h>

#include "usbloader/sdhc.h"
#include "usbloader/wbfs.h"
#include "gecko.h"

//these are the only stable and speed is good
#define CACHE 8
#define SECTORS 64
#define SECTORS_SD 32

#define MOUNT_NONE 0
#define MOUNT_SD   1
#define MOUNT_SDHC 2
#define MOUNT_USB  1

/* Disc interfaces */
extern const DISC_INTERFACE __io_sdhc;

void _FAT_mem_init();

int   fat_sd_mount = MOUNT_NONE;
int   fat_usb_mount = MOUNT_NONE;

int isInserted(const char *path) {
    if (!strncmp(path, "USB:", 4))
        return 1;

    return __io_sdhc.isInserted() || __io_wiisd.isInserted();
}

int SDCard_Init() {
    //closing all open Files write back the cache and then shutdown em!
    fatUnmount("SD:/");
    //right now mounts first FAT-partition
	if (fatMount("SD", &__io_wiisd, 0, CACHE, SECTORS)) {
		fat_sd_mount = MOUNT_SD;
		return 1;
	}
	else if (fatMount("SD", &__io_sdhc, 0, CACHE, SDHC_SECTOR_SIZE)) {
		fat_sd_mount = MOUNT_SDHC;
		return 1;
	}
	return -1;
}

void SDCard_deInit() {
    //closing all open Files write back the cache and then shutdown em!
    fatUnmount("SD:/");

	fat_sd_mount = MOUNT_NONE;
}

int USBDevice_Init() {
	fatUnmount("USB:/");
	if (fatMount("USB", &__io_usbstorage, 0, CACHE, SECTORS)) {
		fat_usb_mount = MOUNT_USB;
		return 1;
	}
	return -1;
}

void USBDevice_deInit() {
	fatUnmount("USB:/");
	fat_usb_mount = MOUNT_NONE;
}


void _FAT_mem_init()
{
}

void* _FAT_mem_allocate(size_t size)
{
	return malloc(size);
}

void* _FAT_mem_align(size_t size)
{
	return memalign(32, size);
}

void _FAT_mem_free(void *mem)
{
	free(mem);
}
