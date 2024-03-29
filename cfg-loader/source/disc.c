#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>

#include "apploader.h"
#include "disc.h"
#include "subsystem.h"
#include "video.h"
#include "wdvd.h"
#include "fst.h"
#include "cfg.h"
#include "music.h"
#include "restart.h"
#include "net.h"
#include "wbfs.h"
#include "fat.h"
#include "usbstorage.h"
#include "sdhc.h"
#include "sys.h"
#include "menu.h"
#include "gettext.h"

#include "wpad.h"
#define ALIGNED(x) __attribute__((aligned(x)))

/* Constants */
#define PTABLE_OFFSET	0x40000
#define WII_MAGIC	0x5D1C9EA3
#define GC_MAGIC	0xC2339F3D

/* Disc pointers */
static u32 *buffer = (u32 *)0x93000000;
static u8  *diskid = (u8  *)0x80000000;

static u8	tmd_buffer[0x49e4] ALIGNED(32);

int yal_OpenPartition(u64 Offset);
int yal_Identify();

#define        Sys_Magic		((u32*) 0x80000020)
#define        Version			((u32*) 0x80000024)
#define        Arena_L			((u32*) 0x80000030)
#define        Bus_Speed		((u32*) 0x800000f8)
#define        CPU_Speed		((u32*) 0x800000fc)

void __Disc_SetLowMem()
{
	/* Setup low memory */
	*(vu32 *)0x80000030 = 0x00000000; // Arena Low
	*(vu32 *)0x80000060 = 0x38A00040;
	*(vu32 *)0x800000E4 = 0x80431A80;
	*(vu32 *)0x800000EC = 0x81800000; // Dev Debugger Monitor Address
	*(vu32 *)0x800000F0 = 0x01800000; // Dev Debugger Monitor Address
	*(vu32 *)0x800000F4 = 0x817E5480;
	*(vu32 *)0x800000F8 = 0x0E7BE2C0; // bus speed
	*(vu32 *)0x800000FC = 0x2B73A840; // cpu speed
	*(vu32 *)0xCD00643C = 0x00000000; // 32Mhz on Bus

	/* Copy disc ID */
	// online check code, seems offline games clear it?
	memcpy((void *)0x80003180, (void *)0x80000000, 4);

	// from yal (kwiirk)
	// Patch in info missing from apploader reads
	*Sys_Magic	= 0x0d15ea5e;
	*Version	= 1;
	*Arena_L	= 0x00000000;
	*Bus_Speed	= 0x0E7BE2C0;
	*CPU_Speed	= 0x2B73A840;
	
	// From NeoGamme R4 (WiiPower)
	*(vu32*)0x800030F0 = 0x0000001C;
	*(vu32*)0x8000318C = 0x00000000;
	*(vu32*)0x80003190 = 0x00000000;
	// Fix for Sam & Max (WiiPower)
	// (only works if started from DVD)
	// Readded by Dr. Clipper
	*(vu32*)0x80003184	= 0x80000000;	// Game ID Address

	/* Flush cache */
	DCFlushRange((void *)0x80000000, 0x3F00);
}

GXRModeObj *disc_vmode = NULL;
u32 vmode_reg = 0;

void __Disc_SelectVMode(void)
{
	GXRModeObj *vmode = NULL;

	u32 progressive, tvmode;
	
	vmode_reg = 0;

	__console_flush(0);

	/* Get video mode configuration */
	progressive = (CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable();
	tvmode      = CONF_GetVideo();
	vmode		= VIDEO_GetPreferredMode(NULL);

	// note: TVEurgb60Hz480Prog crashes libogc, so we use TVNtsc480Prog

	/* Select video mode register */
	switch (tvmode) {
	case CONF_VIDEO_PAL:
		if (CONF_GetEuRGB60() > 0) {
			vmode_reg = 5; // VI_EURGB60
			vmode     = (progressive) ? &TVNtsc480Prog : &TVEurgb60Hz480IntDf;
		} else
			vmode_reg = 1; // VI_PAL

		break;

	case CONF_VIDEO_MPAL:
		//vmode_reg = 4; // 4=VI_DEBUG_PAL?? 2=VI_MPAL
		vmode_reg = VI_MPAL;
		break;

	case CONF_VIDEO_NTSC:
		vmode_reg = 0; // VI_NTSC
		break;
	}

	if (CFG.game.video == CFG_VIDEO_GAME) // game default
	{
	/* Select video mode */
	switch(diskid[3]) {
	/* PAL */
	case 'D':
	case 'F':
	case 'P':
	case 'X':
	case 'Y':
		if (tvmode != CONF_VIDEO_PAL) {
			vmode_reg = 1;
			vmode     = (progressive) ? &TVNtsc480Prog : &TVNtsc480IntDf;
		}

		break;

	/* NTSC or unknown */
	case 'E':
	case 'J':
	default:
		if (tvmode != CONF_VIDEO_NTSC) {
			vmode_reg = 0;
			vmode     = (progressive) ? &TVNtsc480Prog : &TVEurgb60Hz480IntDf;
		}

		break;
	}
	}
	else if (CFG.game.video == CFG_VIDEO_PAL50)
	{
        vmode     =  &TVPal528IntDf;
        vmode_reg = (vmode->viTVMode) >> 2;
	}
	else if (CFG.game.video == CFG_VIDEO_PAL60)
	{
        vmode     = (progressive) ? &TVNtsc480Prog : &TVEurgb60Hz480IntDf;
        vmode_reg = (vmode->viTVMode) >> 2;
		if (progressive) vmode_reg = TVEurgb60Hz480Prog.viTVMode >> 2;
	}
	else if (CFG.game.video == CFG_VIDEO_NTSC)
	{
        vmode     = (progressive) ? &TVNtsc480Prog : &TVNtsc480IntDf;
        vmode_reg = (vmode->viTVMode) >> 2;
	}
	else if (CFG.game.video == CFG_VIDEO_SYS)
	{
		// do nothing.
	}

	disc_vmode = vmode;
}

void __Disc_SetVMode(void)
{
	/* Set video mode register */
	*(vu32 *)0x800000CC = vmode_reg;

	/* Set video mode */
	if (disc_vmode)
		Video_Configure(disc_vmode);

	/* Clear screen */
	Video_Clear(COLOR_BLACK);
}

void __Disc_SetTime(void)
{
	/* Extern */
	extern void settime(u64);

	/* Set proper time */
	settime(secs_to_ticks(time(NULL) - 946684800));
}

s32 __Disc_FindPartition(u64 *outbuf)
{
	u64 offset = 0, table_offset = 0;

	u32 cnt, nb_partitions;
	s32 ret;

	/* Read partition info */
	ret = WDVD_UnencryptedRead(buffer, 0x20, PTABLE_OFFSET);
	if (ret < 0)
		return ret;

	/* Get data */
	nb_partitions = buffer[0];
	table_offset  = buffer[1] << 2;

	/* Read partition table */
	ret = WDVD_UnencryptedRead(buffer, 0x20, table_offset);
	if (ret < 0)
		return ret;

	/* Find game partition */
	for (cnt = 0; cnt < nb_partitions; cnt++) {
		u32 type = buffer[cnt * 2 + 1];

		/* Game partition */
		if(!type)
			offset = ((u64)buffer[cnt * 2]) << 2;
	}

	/* No game partition found */
	if (!offset)
		return -1;

	/* Set output buffer */
	*outbuf = offset;

	return 0;
}


s32 Disc_Init(void)
{
	/* Init DVD subsystem */
	return WDVD_Init();
}

s32 Disc_Open(void)
{
	s32 ret;

	/* Reset drive */
	ret = WDVD_Reset();
	if (ret < 0)
		return ret;

	/* Read disc ID */
	return WDVD_ReadDiskId(diskid);
}

s32 Disc_Wait(void)
{
	u32 cover = 0;
	s32 ret;

	/* Wait for disc */
	while (!(cover & 0x2)) {
		/* Get cover status */
		ret = WDVD_GetCoverStatus(&cover);
		if (ret < 0)
			return ret;
	}

	return 0;
}

s32 Disc_ReadHeader(void *outbuf)
{
	/* Read disc header */
	return WDVD_UnencryptedRead(outbuf, sizeof(struct discHdr), 0);
}

s32 Disc_ReadGCHeader(void *outbuf)
{
	/* Read disc header */
	return WDVD_UnencryptedRead(outbuf, sizeof(struct gc_discHdr), 0);
}

s32 Disc_Type(bool gc)
{
	s32 ret;
	u32 check;
	u32 magic;
	
	if (!gc) {
		check = WII_MAGIC;
		struct discHdr *header = (struct discHdr *)buffer;
		ret = Disc_ReadHeader(header);
		magic = header->magic;
	} else {
		check = GC_MAGIC;
		struct gc_discHdr *header = (struct gc_discHdr *)buffer;
		ret = Disc_ReadGCHeader(header);
		magic = header->magic;
	}

	if (ret < 0)
		return ret;
		
	/* Check magic word */
	if (magic != check)
		return -1;

	return 0;
}

s32 Disc_IsWii(void)
{
	return Disc_Type(0);
}

s32 Disc_IsGC(void)
{
	return Disc_Type(1);
}

s32 Disc_BootPartition(u64 offset)
{
	entry_point p_entry;

	s32 ret;
	
	/* Open specified partition */

	ret = WDVD_OpenPartition(offset, tmd_buffer);
	if (ret < 0) {
		printf(gt("ERROR: OpenPartition(0x%llx) %d"), offset, ret);
		printf("\n");
		return ret;
	}

	printf(gt("Loading ..."));

	util_clear();
	
	/* Close subsystems */
	Net_Close(1);
	Subsystem_Close();
	
	// Load Disc IOS
	u32 disc_ios = tmd_buffer[0x18B];
	if (disc_ios != IOS_GetVersion()) {
		WDVD_ClosePartition();
		WDVD_Close();

		printf("\nLoading IOS %u\n", disc_ios);
		ret = IOS_ReloadIOS(disc_ios);
		if (ret < 0) {
			printf_x(gt("ERROR:"));
			printf("\n");
			printf_(gt("Disc IOS %u could not be loaded! (ret = %d)"), disc_ios, ret);
			printf("\n");
			Restart_Wait();
			return ret;
		}
		Disc_Init();
		Disc_Open();
		WDVD_OpenPartition(offset, tmd_buffer);
	}
	
	/* Setup low memory */
	__Disc_SetLowMem();

	// Select an appropiate video mode
	__Disc_SelectVMode();	

	/* Run apploader */
	ret = Apploader_Run(&p_entry);
	if (ret < 0) {
		printf(gt("ERROR: Apploader %d"), ret);
		printf("\n");
		Restart_Wait();
		return ret;
	}

	// OCARINA STUFF - FISHEARS
	if (CFG.game.ocarina || CFG.wiird) {
		ocarina_do_code();
	}
	
	__console_flush(0);
	
	/* Set an appropiate video mode */
	__Disc_SetVMode();

	/* Set time */
	__Disc_SetTime();
	
	// Anti-green screen fix
	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	/* Shutdown IOS */
// 	SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
	extern void __exception_closeall();
	IRQ_Disable();
	__IOS_ShutdownSubsystems();
	__exception_closeall();
	
	appentrypoint = (u32)p_entry;

	// check if the codehandler is used - if Ocarina and/or debugger is used
	if (CFG.game.ocarina || CFG.wiird)
	{
		__asm__(
			"lis %r3, appentrypoint@h\n"
			"ori %r3, %r3, appentrypoint@l\n"
			"lwz %r3, 0(%r3)\n"
			"mtlr %r3\n"
			"lis %r3, 0x8000\n"
			"ori %r3, %r3, 0x18A8\n"
			"mtctr %r3\n"
			"bctr\n"
		);
	} else
	{
		__asm__(
			"lis %r3, appentrypoint@h\n"
			"ori %r3, %r3, appentrypoint@l\n"
			"lwz %r3, 0(%r3)\n"
			"mtlr %r3\n"
			"blr\n"
		);
	}

	/* Epic failure */
	while (1);

	return 0;
}

s32 Disc_WiiBoot()
{
	u64 offset;
	s32 ret;

	/* Find game partition offset */
	ret = __Disc_FindPartition(&offset);
	if (ret < 0)
		return ret;

	/* Boot partition */
	return Disc_BootPartition(offset);
}



// from 'yal' by kwiirk

#define CERTS_SIZE	0xA00
static const char certs_fs[] ALIGNED(32) = "/sys/cert.sys";

static signed_blob* Certs		= 0;
static signed_blob* Ticket		= 0;
static signed_blob* Tmd		= 0;

static unsigned int C_Length	= 0;
static unsigned int T_Length	= 0;
static unsigned int MD_Length	= 0;

static u8	Ticket_Buffer[0x800] ALIGNED(32);

s32 yal_GetCerts(signed_blob** Certs, u32* Length)
{
	static unsigned char		Cert[CERTS_SIZE] ALIGNED(32);
	memset(Cert, 0, CERTS_SIZE);
	s32				fd, ret;

	fd = IOS_Open(certs_fs, ISFS_OPEN_READ);
	if (fd < 0) return fd;

	ret = IOS_Read(fd, Cert, CERTS_SIZE);
	if (ret < 0)
	{
		if (fd >0) IOS_Close(fd);
		return ret;
	}

	*Certs = (signed_blob*)(Cert);
	*Length = CERTS_SIZE;

	if (fd > 0) IOS_Close(fd);

	return 0;
}

int yal_OpenPartition(u64 Offset)
{
	int ret = -1;
	// Offset = Partition_Info.Offset << 2
	u32 Partition_Info_Offset = Offset >> 2;

	//printf_("Loading .");
	
    //DI_Set_OffsetBase(Offset);
    ret = YAL_Set_OffsetBase(Offset);
	if (ret < 0) {
		printf("\n");
		printf(gt("ERROR: Offset(0x%llx) %d"), Offset, ret);
		printf("\n");
		return ret; 
	}

	ret = yal_GetCerts(&Certs, &C_Length);
	if (ret < 0) {
		printf("\n");
		printf(gt("ERROR: GetCerts %d"), ret);
		printf("\n");
		return ret; 
	}

	//DI_Read_Unencrypted(Ticket_Buffer, 0x800, Partition_Info.Offset << 2);
	WDVD_UnencryptedRead(Ticket_Buffer, 0x800, Offset);
	Ticket		= (signed_blob*)(Ticket_Buffer);
	T_Length	= SIGNED_TIK_SIZE(Ticket);

	// Open Partition and get the TMD buffer
	//if (DI_Open_Partition(Partition_Info.Offset, 0,0,0, tmd_buffer) < 0)
	ret = YAL_Open_Partition(Partition_Info_Offset, 0,0,0, tmd_buffer);
	if (ret < 0) {
		printf("\n");
		printf(gt("ERROR: OpenPartition %d"), ret);
		printf("\n");
		return ret;
	}
	Tmd = (signed_blob*)(tmd_buffer);
	MD_Length = SIGNED_TMD_SIZE(Tmd);

	//printf(".");
	return 0;
}


int yal_Identify()
{
	// Identify as the game
	if (IS_VALID_SIGNATURE(Certs) 	&& IS_VALID_SIGNATURE(Tmd) 	&& IS_VALID_SIGNATURE(Ticket) 
		&&  C_Length > 0 				&& MD_Length > 0 			&& T_Length > 0)
	{
			int ret = ES_Identify(Certs, C_Length, Tmd, MD_Length, Ticket, T_Length, NULL);
			if (ret < 0)
					return ret;
	}
	return 0;
}

