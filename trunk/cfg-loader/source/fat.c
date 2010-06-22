
// Modified by oggzee

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ogcsys.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sdcard/wiisd_io.h>
#include <string.h>
#include <locale.h>
#include <errno.h>

#include <fat.h>

#include "sdhc.h"
#include "util.h"
#include "wpad.h"
#include "menu.h"
#include "gettext.h"
#include "cfg.h"
#include "sys.h"


/* Constants */
#define SDHC_MOUNT	"sd"
#define USB_MOUNT	"usb"

//#define FAT_CACHE 4
//#define FAT_SECTORS 64
//#define FAT_CACHE 64
#define FAT_CACHE 32
#define FAT_SECTORS 64
#define FAT_SECTORS_SD 32
//#define FAT_CACHE_SECTORS 256
#define FAT_CACHE_SECTORS FAT_CACHE, FAT_SECTORS
#define FAT_CACHE_SECTORS_SD FAT_CACHE, FAT_SECTORS_SD

/* Disc interfaces */
extern const DISC_INTERFACE __io_sdhc;
extern DISC_INTERFACE __io_usbstorage;

void _FAT_mem_init();
extern sec_t _FAT_startSector;

//#define MOUNT_NONE 0
//#define MOUNT_SD   1
//#define MOUNT_SDHC 2

int   fat_sd_mount = 0;
sec_t fat_sd_sec = 0; // u32

int   fat_usb_mount = 0;
sec_t fat_usb_sec = 0;

void fat_Unmount(const char* name);

s32 Fat_MountSDHC(void)
{
	s32 ret;

	if (fat_sd_mount) return 0;
	fat_sd_sec = 0;

	_FAT_mem_init();

	if (IOS_GetVersion() >= 60) // IOS above 60 or higher supports SDHC
	{
		sdhc_mode_sd = 1;
	}

	/* Initialize SD/SDHC interface */
	retry:
	dbg_printf("SD init\n");
	ret = __io_sdhc.startup();
	if (!ret) {
		dbg_printf("ERROR: SDHC init! (%d)\n", ret); sleep(1);
		if (!sdhc_mode_sd) {
		   sdhc_mode_sd = 1;
		   goto retry;
		}
		return -1;
	}

	dbg_printf("SD fat mount\n");
	/* Mount device */
	if (!sdhc_mode_sd) {
		ret = fatMount(SDHC_MOUNT, &__io_sdhc, 0, FAT_CACHE_SECTORS);
	} else {
		ret = fatMount(SDHC_MOUNT, &__io_sdhc, 0, FAT_CACHE_SECTORS_SD);
	}
	if (!ret) {
		//printf_x("ERROR: SDHC/FAT init! (%d)\n", ret); sleep(1);
		return -2;
	}

	fat_sd_mount = 1;
	fat_sd_sec = _FAT_startSector;
	return 0;

#if 0
	try_sd:
	// SDHC failed, try SD
	ret = __io_wiisd.startup();
	if (!ret) {
		//printf_x("ERROR: SD init! (%d)\n", ret); sleep(1);
		return -3;
	}
	sdhc_mode_sd = 1;
	//ret = fatMountSimple("sd", &__io_wiisd);
	ret = fatMount(SDHC_MOUNT, &__io_wiisd, 0, FAT_CACHE_SECTORS_SD);
	//ret = fatMount(SDHC_MOUNT, &__io_wiisd, 0, FAT_CACHE, FAT_SECTORS);
	if (!ret) {
		// printf_x("ERROR: SD/FAT init! (%d)\n", ret); sleep(1);
		return -4;
	}
	//printf_x("NOTE: SDHC mode not available\n");
	//printf_x("NOTE: card in standard SD mode\n\n");

	fat_sd_mount = 1;
	fat_sd_sec = _FAT_startSector;

	return 0;
#endif
}

s32 Fat_UnmountSDHC(void)
{
	s32 ret = 1;
	if (!fat_sd_mount) return 0;

	/* Unmount device */
	fat_Unmount(SDHC_MOUNT);

	/* Shutdown SDHC interface */
	if (sdhc_mode_sd == 0) {
		SDHC_Close();
	} else {
		ret = __io_wiisd.shutdown();
	}
	fat_sd_mount = 0;
	fat_sd_sec = 0;
	if (!ret)
		return -1;

	return 0;
}

s32 Fat_ReadFile(const char *filepath, void **outbuf)
{
	FILE *fp     = NULL;
	void *buffer = NULL;

	struct stat filestat;
	u32         filelen;

	s32 ret;

	/* Get filestats */
	ret = stat(filepath, &filestat);
	if (ret != 0) {
		//printf("File not found %s %d\n", filepath, (int)filestat.st_size); sleep(3);
		return -1;
	}

	/* Get filesize */
	filelen = filestat.st_size;

	/* Allocate memory */
	buffer = memalign(32, filelen);
	if (!buffer)
		goto err;

	/* Open file */
	fp = fopen(filepath, "rb");
	if (!fp)
		goto err;

	/* Read file */
	ret = fread(buffer, 1, filelen, fp);
	if (ret != filelen)
		goto err;

	/* Set pointer */
	*outbuf = buffer;

	goto out;

err:
	/* Free memory */
	if (buffer)
		free(buffer);

	/* Error code */
	ret = -1;

out:
	/* Close file */
	if (fp)
		fclose(fp);

	return ret;
}

void Fat_print_sd_mode()
{
	printf(gt("FAT32 mount: "));
	if (fat_sd_mount) printf("%s ", sdhc_mode_sd ? "sd" : "SD");
	if (!fat_sd_mount) printf("NONE");
	printf("\n");
}

s32 Fat_MountUSB(void)
{
	s32 ret;

	if (fat_usb_mount) return 0;
	_FAT_mem_init();

	/* Initialize USB interface */
	ret = __io_usbstorage.startup();
	if (!ret) {
		//printf_x("ERROR: USB init! (%d)\n", ret); sleep(1);
		return -1;
	}

	/* Mount device */
	ret = fatMount(USB_MOUNT, &__io_usbstorage, 0, FAT_CACHE_SECTORS);
	//ret = fatMount(USB_MOUNT, &__io_usbstorage, 0, FAT_CACHE, FAT_SECTORS);
	if (!ret) {
		//printf_x("ERROR: USB/FAT init! (%d)\n", ret); sleep(1);
		return -2;
	}

	fat_usb_mount = 1;
	fat_usb_sec = _FAT_startSector;

	return 0;
}

s32 Fat_UnmountUSB(void)
{
	if (fat_usb_mount == 0) return 0;

	/* Unmount device */
	fat_Unmount(USB_MOUNT);

	fat_usb_mount = 0;
	fat_usb_sec = 0;

	return 0;
}

void Fat_UnmountAll()
{
	Fat_UnmountSDHC();
	Fat_UnmountUSB();
}

// fat cache alloc

#if 1

static void *fat_pool = NULL;
static size_t fat_size;
#define FAT_SLOTS (FAT_CACHE * 3)
#define FAT_SLOT_SIZE (512 * FAT_SECTORS)
#define FAT_SLOT_SIZE_MIN (512 * FAT_SECTORS_SD)
static int fat_alloc[FAT_SLOTS];

void _FAT_mem_init()
{
	if (fat_pool) return;
	fat_size = FAT_SLOTS * FAT_SLOT_SIZE;
	fat_pool = LARGE_memalign(32, fat_size);
}

void* _FAT_mem_allocate(size_t size)
{
	return malloc(size);
}

void* _FAT_mem_align(size_t size)
{
	if (size < FAT_SLOT_SIZE_MIN || size > FAT_SLOT_SIZE) goto fallback;
	if (fat_pool == NULL) goto fallback;
	int i;
	for (i=0; i<FAT_SLOTS; i++) {
		if (fat_alloc[i] == 0) {
			void *ptr = fat_pool + i * FAT_SLOT_SIZE;
			fat_alloc[i] = 1;
			return ptr;
		}	
	}
	fallback:
	return memalign (32, size);		
}

void _FAT_mem_free(void *mem)
{
	if (fat_pool == NULL || mem < fat_pool || mem >= fat_pool + fat_size) {
		free(mem);
		return;
	}
	int i;
	for (i=0; i<FAT_SLOTS; i++) {
		if (fat_alloc[i]) {
			void *ptr = fat_pool + i * FAT_SLOT_SIZE;
			if (mem == ptr) {
				fat_alloc[i] = 0;
				return;
			}
		}	
	}
	// FATAL
	printf("\n");
	printf(gt("FAT: ALLOC ERROR %p %p %p"), mem, fat_pool, fat_pool + fat_size);
	printf("\n");
	sleep(5);
	exit(1);
}

#else

void _FAT_mem_init()
{
}

void* _FAT_mem_allocate (size_t size) {
	return malloc (size);
}

void* _FAT_mem_align (size_t size) {
	return memalign (32, size);
}

void _FAT_mem_free (void* mem) {
	free (mem);
}

#endif

// fix for newlib/libfat unmount without :

void fat_Unmount (const char* name)
{
	char name2[16];
	strcpy(name2, name);
	strcat(name2, ":");
	fatUnmount(name2);
}

#if 0
#include <sys/iosupport.h>
//extern int FindDevice(const char* name);
//extern const devoptab_t* GetDeviceOpTab (const char *name);
void check_dev(char *name)
{
	printf("DEV: %s %d %p\n", name, FindDevice(name), GetDeviceOpTab(name));
}
	check_dev("sd");
	check_dev("sd:");
#endif

