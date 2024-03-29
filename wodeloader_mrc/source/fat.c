#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ogcsys.h>
#include <fat.h>
#include <sys/stat.h>
#include <sdcard/wiisd_io.h>
#include <dirent.h>

/* Constants */
#define SDHC_MOUNT	"sdhc"


s32 Fat_MountSDHC(void){
	s32 ret;

	/* Initialize SDHC interface */
	ret = __io_wiisd.startup();
	if (!ret)
		return -1;

	/* Mount device */
	ret = fatMountSimple(SDHC_MOUNT, &__io_wiisd);
	if (!ret)
		return -2;

	return 0;
}

s32 Fat_UnmountSDHC(void){
	s32 ret;

	/* Unmount device */
	fatUnmount(SDHC_MOUNT);

	/* Shutdown SDHC interface */
	ret = __io_wiisd.shutdown();
	if (!ret)
		return -1;

	return 0;
}

s32 Fat_ReadFile(const char *filepath, void **outbuf){
	FILE *fp     = NULL;
	void *buffer = NULL;

	u32         filelen;

	s32 ret;

	/* Open file */
	fp = fopen(filepath, "rb");
	if (!fp)
		goto err;

	/* Get filesize */
	fseek(fp, 0, SEEK_END);
	filelen = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* Allocate memory */
	buffer = malloc(filelen);
	if (!buffer)
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

bool Fat_CheckDir(const char *dname){
	DIR *dir;
	bool res=false;
	dir=opendir(dname);
	if(dir)
		res=true;
	closedir(dir);

	return res;
}

bool Fat_CheckFile(const char *filepath){
	FILE *fp     = NULL;

	/* Open file */
	fp = fopen(filepath, "rb");
	if (!fp)
		return false;

	fclose(fp);

	return true;
}

s32 Fat_SaveFile(const char *filepath, void **outbuf,u32 outlen){
	s32 ret;
	FILE *fd;
	fd = fopen(filepath, "wb");
	if(fd){
		ret=fwrite(*outbuf, 1, outlen, fd);
		fclose(fd);
		//printf(" FWRITE: %d ",ret);
	}else{
		ret=-1;
	}

	return ret;
}
