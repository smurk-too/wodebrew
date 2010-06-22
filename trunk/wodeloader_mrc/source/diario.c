/*
	DIARIO.C
	This code allows to modify play_rec.dat in order to store the
	game time in Wii's log correctly.

	by Marc
	Thanks to tueidj for giving me some hints on how to do it :)
	Most of the code was taken from here:
	http://forum.wiibrew.org/read.php?27,22130


	Modification by Dr. Clipper:
	now it starts counting the time when you boot the game
*/

// English Translation -- Well Tried lol -- By pepxl

#define SECONDS_TO_2000 946684800LL
#define TICKS_PER_SECOND 60750000LL

#include <stdio.h>
#include <ogcsys.h>

#define PLAYRECPATH "/title/00000001/00000002/data/play_rec.dat"

typedef struct{
	u32 checksum;
	union{
		u32 data[0x1f];
		struct{
			char name[84];	//u16 name[42];
			u64 ticks_boot;
			u64 ticks_last;
			char title_id[4];
			char title_gid[2];
			//u8 unknown[18];
		} ATTRIBUTE_PACKED;
	};
} playrec_struct;

playrec_struct playrec_buf;

u64 getWiiTime(void) {
	time_t uTime;
	uTime = time(NULL);
	return TICKS_PER_SECOND * (uTime - SECONDS_TO_2000);
}

int Diario_ActualizarDiario(char* bannerTitle,char* gameId){
	s32 ret,playrec_fd;
	u32 sum = 0;
	u8 i;

	//Open play_rec.dat
	playrec_fd = IOS_Open(PLAYRECPATH, IPC_OPEN_RW);
	if(playrec_fd < 0) {
		printf("* ERROR: opening play_rec.dat (%d)\n",playrec_fd);
		return playrec_fd;
	}

	//Read play_rec.dat
	ret = IOS_Read(playrec_fd, &playrec_buf, sizeof(playrec_buf));
	if(ret != sizeof(playrec_buf)){
		IOS_Close(playrec_fd);
		printf("* ERROR: reading play_rec.dat (%d)\n",ret);
		return -1;
	}
	IOS_Close(playrec_fd);

	//Update channel name and ID
	for(i=0;i<84;i++)
		playrec_buf.name[i]=bannerTitle[i];

	playrec_buf.title_id[0]=gameId[0];
	playrec_buf.title_id[1]=gameId[1];
	playrec_buf.title_id[2]=gameId[2];
	playrec_buf.title_id[3]=gameId[3];
	playrec_buf.title_gid[0]=gameId[4];
	playrec_buf.title_gid[1]=gameId[5];

	//Calculate and update checksum
	for(i=0; i<0x1f; i++)
		sum += playrec_buf.data[i];
	playrec_buf.checksum=sum;

	//Count from game boot (not homebrew launcher)
	u64 now=getWiiTime();
	playrec_buf.ticks_boot=now;
	playrec_buf.ticks_last=now;

	//Save new play_rec.dat
	ret = ISFS_Initialize();
	if(ret){
		ISFS_Deinitialize();
		printf("* ERROR: starting ISFS (%d)\n",ret);
		return -1;
	}

	playrec_fd = IOS_Open(PLAYRECPATH, IPC_OPEN_RW);
	if(playrec_fd<0){
		ISFS_Deinitialize();
		printf("* ERROR: opening play_rec.dat (%d)\n",playrec_fd);
		return playrec_fd;
	}

	ret = IOS_Write(playrec_fd, &playrec_buf, sizeof(playrec_buf));
	if(ret!=sizeof(playrec_buf)){
		IOS_Close(playrec_fd);
		ISFS_Deinitialize();
		printf("* ERROR: keeping play_rec.dat (%d)\n",ret);
		return -1;
	}

	IOS_Close(playrec_fd);
	ISFS_Deinitialize();

	return 0;
}
