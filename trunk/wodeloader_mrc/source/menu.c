#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <asndlib.h>
#include <network.h>
#include <sys/errno.h>
#include <unistd.h> //para sleep
#include <sys/stat.h> //para mkdir

#include "disc.h"
#include "fat.h"
#include "fstfile.h"
#include "menu.h"
#include "sys.h"
#include "utils.h"
#include "video.h"
#include "wbfs.h"
#include "wdvd.h"
#include "wode.h"
#include "wpad.h"
#include "sonido.h"
#include "diario.h"
#include "http.h"


// Constants
#define CONSOLE_XCOORD		216
#define CONSOLE_YCOORD		432
#define CONSOLE_WIDTH		392
#define CONSOLE_HEIGHT		32
#define BIGCONSOLE_XCOORD	200
#define BIGCONSOLE_YCOORD	192
#define BIGCONSOLE_WIDTH	424
#define BIGCONSOLE_HEIGHT	272
#define BIGCONSOLE_MAXCH	38
#define PAGEBUTTONS_XCOORD	294
#define PAGEBUTTONS_YCOORD	403
#define PAGEBUTTONS_WIDTH	30
#define MODOSDEVISTA		2
#define USBLOADER_PATH			"sdhc:/wodeloader_mrc/"
#define USBLOADER_PATH_SINBARRA	"sdhc:/wodeloader_mrc"
#define USBLOADER_CONFIG_FILE	"wodeloader_mrc.cfg"
#define MAXJUEGOS			2800 //cambiar esto hará que no lea bien la configuración

#define BLACK	0
#define RED		1
#define GREEN	2
#define YELLOW	3
#define BLUE	4
#define MAGENTA	5
#define CYAN	6
#define WHITE	7

// English Translation -- Well Tried lol -- By pepxl

/* Gamelist buffer */
static struct discHdr *gameList = NULL;

/* Gamelist variables */
static s32 gameCnt = 0;

/* variables marc */
static u8 vistaActual=0;
static u8 paginasLeidas=0;
static u16 pagina=0;
static u8 columna;
static u8 fila;
static s32 gameSelected=0;
static bool wideScreen=false;
static bool volverAWii=false;
static bool ejecucionRapida=false;
static char partition[32];

static bool haHabidoCambios=false;


typedef struct {
    u8 modoVideo;
    bool anti002;
    u8 ocarina;
    u8 altdol;
    bool diario;
    u32* banner;
} configuracionJuego;

static configuracionJuego* configuracionJuegos=NULL;
extern char mrc_background_png[];
extern char mrc_banner_container_png[];
extern char mrc_banner_container_wide_png[];
extern char mrc_banner_selector_png[];
extern char mrc_banner_selector_wide_png[];
extern char mrc_banner_noimg_png[];
extern char mrc_banner_empty_png[];
extern char mrc_cover_container_png[];
extern char mrc_cover_container_wide_png[];
extern char mrc_cover_selector_png[];
extern char mrc_cover_selector_wide_png[];
extern char mrc_cover_noimg_png[];
extern char mrc_cover_empty_png[];
extern char mrc_left_button_png[];
extern char mrc_right_button_png[];

static void* imgFondo;
static void* imgLeftButton;
static void* imgRightButton;
struct tipoDeVista{
	const char ID;
	const char* COLETILLA;
	const void* IMAGENCONTENEDOR[2];
	const void* IMAGENSELECTOR[2];
	const void* IMAGENSINPORTADA;
	const void* IMAGENHUECO;
	const u8 COLUMNAS[2];
	const u8 FILAS;
	const u8 PRIMERACOLUMNA[2];
	const u8 PRIMERAFILA;
	const u8 SEPARACION;
	const u8 ANCHOIMAGEN[2];
	const u8 ALTOIMAGEN;
	const u8 BORDEIMAGEN;
	const u8 COLUMNADEFECTO[2];
	const u8 FILADEFECTO;
	u16* orden;
	u16 paginas;
};
static struct tipoDeVista vistas[]={
	{'b',"banner",
	{(void *)mrc_banner_container_png,(void *)mrc_banner_container_wide_png},
	{(void *)mrc_banner_selector_png,(void *)mrc_banner_selector_wide_png},
	(void *)mrc_banner_noimg_png,(void *)mrc_banner_empty_png,
	{3,4},5,
	{18,14},26,12,
	{192,144},64,4,
	{1,1},2,
	NULL,
	},

	{'c',"cover",
	{(void *)mrc_cover_container_png,(void *)mrc_cover_container_wide_png},
	{(void *)mrc_cover_selector_png,(void *)mrc_cover_selector_wide_png},
	(void *)mrc_cover_noimg_png,(void *)mrc_cover_empty_png,
	{4,5},2,
	{36,38},24,18,
	{128,96},176,4,
	{0,2},0,
	NULL
	}
};

void set_console_fgcolor(u8 color,u8 bold){
	printf("\x1b[%u;%um", color+30,bold);
	fflush(stdout);
}

void set_console_bgcolor(u8 color,u8 bold){
	printf("\x1b[%u;%um", color+40,bold);
	fflush(stdout);
}

void initialize_big_console(void){
	Con_Init(BIGCONSOLE_XCOORD, BIGCONSOLE_YCOORD, BIGCONSOLE_WIDTH, BIGCONSOLE_HEIGHT);
	set_console_bgcolor(BLUE,0);
}

void initialize_small_console(void){
	Con_InitTr(CONSOLE_XCOORD, CONSOLE_YCOORD, CONSOLE_WIDTH, CONSOLE_HEIGHT);
	set_console_bgcolor(BLUE,0);
}



char* substr(char* oldString,u16 start,u16 length){
	char *newString=malloc(length+1);
	u16 i;

	for(i=0;i<length;i++)
		newString[i]=oldString[start+i];
	newString[length]='\0';

	return newString;
}

void inicializar_vista(){
	gameSelected=0;
	pagina=0;
	paginasLeidas=0;

	if(gameCnt>vistas[vistaActual].COLUMNAS[wideScreen]*vistas[vistaActual].FILAS){
		columna=vistas[vistaActual].COLUMNADEFECTO[wideScreen];
		fila=vistas[vistaActual].FILADEFECTO;
	}else{
		columna=0;
		fila=0;
	}
}

s32 __Gui_DrawPng(void *img, u32 x, u32 y){
	IMGCTX   ctx = NULL;
	PNGUPROP imgProp;

	s32 ret;

	/* Select PNG data */
	ctx = PNGU_SelectImageFromBuffer(img);
	if (!ctx) {
		ret = -1;
		goto out;
	}

	/* Get image properties */
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK) {
		ret = -1;
		goto out;
	}

	/* Draw image */
	Video_DrawPng(ctx, imgProp, x, y);

	/* Success */
	ret = 0;

out:
	/* Free memory */
	if (ctx)
		PNGU_ReleaseImageContext(ctx);

	return ret;
}

s32 __Gui_DrawPngTrans(void *img, u32 x, u32 y){
	IMGCTX   ctx = NULL;
	PNGUPROP imgProp;

	s32 ret;

	/* Select PNG data */
	ctx = PNGU_SelectImageFromBuffer(img);
	if (!ctx) {
		ret = -1;
		goto out;
	}

	/* Get image properties */
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK) {
		ret = -1;
		goto out;
	}

	/* Draw image */
	Video_DrawPngTrans(ctx, imgProp, x, y);

	/* Success */
	ret = 0;

out:
	/* Free memory */
	if (ctx)
		PNGU_ReleaseImageContext(ctx);

	return ret;
}
s32 __Gui_DrawPngResize(void *img, u32 x, u32 y,u16 w,u16 h){
	IMGCTX   ctx = NULL;
	PNGUPROP imgProp;

	s32 ret;

	/* Select PNG data */
	ctx = PNGU_SelectImageFromBuffer(img);
	if (!ctx) {
		ret = -1;
		goto out;
	}

	/* Get image properties */
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK) {
		ret = -1;
		goto out;
	}

	/* Draw image */
	Video_DrawPngResize(ctx, imgProp, x, y,w,h);

	/* Success */
	ret = 0;

out:
	/* Free memory */
	if (ctx)
		PNGU_ReleaseImageContext(ctx);

	return ret;
}

void calcularPaginas(u8 vista){
	int i;
	for(i=MAXJUEGOS-1;i>-1;i--){
		if(vistas[vista].orden[i]!=MAXJUEGOS){
			break;
		}
	}

	int paginaFinal=0;
	while(paginaFinal*vistas[vista].COLUMNAS[wideScreen]*vistas[vista].FILAS<=i){
		paginaFinal++;
	}
	vistas[vista].paginas=paginaFinal;
}

s32 __Menu_GetEntries(void){
	struct discHdr *buffer = NULL;

	u32 cnt, len;
	s32 ret;

	/* Get list length */
	ret = WBFS_GetCount(&cnt);
	if (ret < 0)
		return ret;

	/* Buffer length */
	len = sizeof(struct discHdr) * cnt;

	/* Allocate memory */
	buffer = (struct discHdr *)memalign(32, len);
	if (!buffer)
		return -1;

	/* Clear buffer */
	memset(buffer, 0, len);

	/* Get header list */
	ret = WBFS_GetHeaders(buffer, cnt, sizeof(struct discHdr));
	if (ret < 0)
		goto err;

	/* Free memory */
	if (gameList)
		free(gameList);

	/* Set values */
	gameList = buffer;
	gameCnt  = cnt;

	//Reservar espacio para configuraciones
	if(configuracionJuegos)
		free(configuracionJuegos);
	configuracionJuegos=malloc(sizeof(configuracionJuego) * gameCnt);
	int l;
	for(l=0;l<MODOSDEVISTA;l++){
		if(vistas[l].orden)
			free(vistas[l].orden);
		vistas[l].orden=malloc(sizeof(u16)*MAXJUEGOS);
		vistas[l].paginas=0;
	}

	int i;

	for(i=0;i<MODOSDEVISTA;i++)
		for(l=0;l<MAXJUEGOS;l++)
			vistas[i].orden[l]=MAXJUEGOS;
		
	for(i=0;i<gameCnt;i++){
		configuracionJuegos[i].modoVideo=0;
		configuracionJuegos[i].anti002=true;
		configuracionJuegos[i].ocarina=0;
		configuracionJuegos[i].altdol=0;
		configuracionJuegos[i].diario=true;
	}

	memset(&partition, 0, 32);
	
	//Ordenar los juegos segun archivo de configuracion o alfabeticamente
	char *archivoLeido=NULL;
	char filepath[128];
	sprintf(filepath, USBLOADER_PATH "%s",USBLOADER_CONFIG_FILE);
	ret = Fat_ReadFile(filepath, (void *)&archivoLeido);

	if (ret > 0) {
		//Archivo encontrado

		u16 start=0;
		u16 length=0;
		u32 maxLength=ret;

		u16 juegosEnSD=0;
		u16* temporalJuegos=malloc(sizeof(u16)*MAXJUEGOS);
		bool* vistasActualizadas=malloc(sizeof(bool)*MODOSDEVISTA);
		for(i=0;i<MODOSDEVISTA;i++)
			vistasActualizadas[i]=false;

		for(;;){
			length++;
			if(start+length>maxLength){
				break;
			}

			if((archivoLeido[start+length]=='\n')){
				char* linea=substr(archivoLeido,start,length);

				if((length>=5) && linea[0]=='c'){
					vistaActual=linea[1];
					wideScreen=linea[2];
					volverAWii=linea[3];
					ejecucionRapida=linea[4];
					if (length>5) {
						strncpy(partition, linea + 5, 32);
					}
				}else if(length==11){
					char* buscoEste=substr(linea,0,6);
					int j;
					temporalJuegos[juegosEnSD]=MAXJUEGOS;
					for(j=0;j<gameCnt;j++){
						struct discHdr *header = &gameList[j];
						if(strcmp(buscoEste,(void *)header->id)==0){
							temporalJuegos[juegosEnSD]=j;
							//printf("\n* %s encontrado en %d",buscoEste,j);

							configuracionJuegos[j].modoVideo=linea[6];
							configuracionJuegos[j].anti002=linea[7];
							configuracionJuegos[j].ocarina=linea[8];
							configuracionJuegos[j].altdol=linea[9];
							configuracionJuegos[j].diario=linea[10];

							break;
						}
					}
					juegosEnSD++;
				}else if(length%2==0 && linea[0]=='v'){//sistema ANTIGUO de orden (mantener algunas versiones)
					u16* puntero=NULL;
					for(l=0;l<MODOSDEVISTA;l++)
						if(vistas[l].ID==linea[1]){
							vistasActualizadas[l]=true;
							puntero=vistas[l].orden;
							break;
						}

					if(puntero!=NULL){
						int k;
						int anadidos=0;
						for(k=2;k<length;k+=2){
							u8 byte1=linea[k]-17;
							u8 byte2=linea[k+1]-17;
							int juego=(byte2*224)+byte1;
							if(juego!=MAXJUEGOS && temporalJuegos[juego]!=MAXJUEGOS){
								puntero[(k/2)-1]=temporalJuegos[juego];
								anadidos++;
							}
						}

						if(anadidos<gameCnt){//en la SD habia menos juegos
							for(l=0;l<gameCnt;l++)
								for(k=0;k<MAXJUEGOS;k++){
									if(puntero[k]==l)
										break;

									if(k==MAXJUEGOS-1){
										for(i=0;i<MAXJUEGOS;i++){
											if(puntero[i]==MAXJUEGOS){
												puntero[i]=l;
												break;
											}
										}
									}
								}
						}
					}
				}else if(length%2==0 && linea[0]=='o'){//sistema NUEVO de orden
					u16* puntero=NULL;
					for(l=0;l<MODOSDEVISTA;l++)
						if(vistas[l].ID==linea[1]){
							vistasActualizadas[l]=true;
							puntero=vistas[l].orden;
							break;
						}

					if(puntero!=NULL){
						u16 anadidos=0;
						for(l=0;l<juegosEnSD;l++){
							if(temporalJuegos[l]!=MAXJUEGOS){
								u8 byte1=linea[l*2+2]-17;
								u8 byte2=linea[l*2+2+1]-17;
								u16 pos=(byte2*224)+byte1;
								/*printf("POS: %d, L: %d\n",pos,temporalJuegos[l]);
								Wpad_WaitButtons();*/

								puntero[pos]=temporalJuegos[l];
								anadidos++;
							}
						}

						/*printf("\nENTRO!\n%d y %d",anadidos,gameCnt);
						Wpad_WaitButtons();*/
						//comprobar que no falte ningun juego
						if(anadidos<gameCnt){
							u16 k;
							for(l=0;l<gameCnt;l++){
								for(k=0;k<MAXJUEGOS;k++)
									if(puntero[k]==l)
										break;

								if(k==MAXJUEGOS){//colocar el juego que falta en un hueco
									for(i=0;i<MAXJUEGOS;i++){
										if(puntero[i]==MAXJUEGOS){
											puntero[i]=l;
											break;
										}
									}
								}
							}
						}
					}
				}

				start=start+length+1;
				length=0;
			}
		}

		for(i=0;i<MODOSDEVISTA;i++)
			if(!vistasActualizadas[i])
				for(l=0;l<gameCnt;l++)
					vistas[i].orden[l]=l;

		free(temporalJuegos);
		free(vistasActualizadas);
		free(archivoLeido);


	}else{
		for(l=0;l<MODOSDEVISTA;l++)
			for(i=0;i<gameCnt;i++)
				vistas[l].orden[i]=i;
	}

	for(l=0;l<MODOSDEVISTA;l++)
		calcularPaginas(l);

	if (partition[0] == '\0') {
		u32 partIdx = WBFS_GetDefaultPartition();
		WBFS_GetPartitionName(partIdx, (char *) &partition);
	}

	inicializar_vista();

	return 0;

err:
	/* Free memory */
	if (buffer)
		free(buffer);

	return ret;
}



void __Gui_DrawBanner(u8 dcol,u8 drow,u8 selected, s32 game){
	u16 x=vistas[vistaActual].PRIMERACOLUMNA[wideScreen]+((vistas[vistaActual].SEPARACION+vistas[vistaActual].ANCHOIMAGEN[wideScreen])*dcol);
	u16 y=vistas[vistaActual].PRIMERAFILA+((vistas[vistaActual].SEPARACION+vistas[vistaActual].ALTOIMAGEN)*drow);

	u16 xborder=x-vistas[vistaActual].BORDEIMAGEN;
	u16 yborder=y-vistas[vistaActual].BORDEIMAGEN;

	if(selected){
		__Gui_DrawPngTrans((void *)vistas[vistaActual].IMAGENSELECTOR[wideScreen], xborder, yborder);
	}else{
		__Gui_DrawPngTrans((void *)vistas[vistaActual].IMAGENCONTENEDOR[wideScreen], xborder, yborder);
	}

	if(game!=MAXJUEGOS){
		if(!wideScreen)
			__Gui_DrawPng((void *)configuracionJuegos[game].banner, x, y);
		else
			__Gui_DrawPngResize((void *)configuracionJuegos[game].banner, x, y,vistas[vistaActual].ANCHOIMAGEN[wideScreen],vistas[vistaActual].ALTOIMAGEN);
	}else{
		if(!wideScreen)
			__Gui_DrawPng((void *)vistas[vistaActual].IMAGENHUECO, x, y);
		else
			__Gui_DrawPngResize((void *)vistas[vistaActual].IMAGENHUECO, x, y,vistas[vistaActual].ANCHOIMAGEN[wideScreen],vistas[vistaActual].ALTOIMAGEN);

	}
}


void __Gui_DrawPageBanners(void){
	// No game list
	if (!gameCnt)
		return;

	// Draw background
	__Gui_DrawPng(imgFondo, 0, 0);

	u16 i=0;
	u8 covcol=0;
	u8 covrow=0;

	if(paginasLeidas<pagina+1){
		int ret=Fat_MountSDHC();
		if(ret<0){
			u32 start=pagina*vistas[vistaActual].COLUMNAS[wideScreen]*vistas[vistaActual].FILAS;
			u32 end=start+vistas[vistaActual].COLUMNAS[wideScreen]*vistas[vistaActual].FILAS;
			for(i=start;i<end && i<MAXJUEGOS;i++){
				u16 game=vistas[vistaActual].orden[i];
				configuracionJuegos[game].banner=(void *)vistas[vistaActual].IMAGENSINPORTADA;
			}
		}else{
			printf("\n\nLoading Images...");
			//printf("\nLoading page %d banners...",page);
			u32 start=pagina*vistas[vistaActual].COLUMNAS[wideScreen]*vistas[vistaActual].FILAS;
			u32 end=start+vistas[vistaActual].COLUMNAS[wideScreen]*vistas[vistaActual].FILAS;
			for(i=start;i<end && i<MAXJUEGOS;i++){
				if(vistas[vistaActual].orden[i]!=MAXJUEGOS){
					char filepath[64];
					u16 game=vistas[vistaActual].orden[i];
					struct discHdr *header = &gameList[game];
					void *imgData=(void *)vistas[vistaActual].IMAGENSINPORTADA;
		
					// Cargar imagen con toda la ID
					sprintf(filepath, USBLOADER_PATH "%s_%s.png", header->id,vistas[vistaActual].COLETILLA);

					// Si la he encontrado
					s32 ret = Fat_ReadFile(filepath, &imgData);
					if(ret>128000){//demasiado grande
						free(imgData);
						imgData=(void *)vistas[vistaActual].IMAGENSINPORTADA;
					}else if(ret<0){
						// Cargar imagen solo con los 3 primeros caracteres
						sprintf(filepath, USBLOADER_PATH "%c%c%c_%s.png", header->id[0],header->id[1],header->id[2],vistas[vistaActual].COLETILLA);

						// Si la he encontrado
						ret = Fat_ReadFile(filepath, &imgData);
						if(ret>128000){//demasiado grande
							free(imgData);
							imgData=(void *)vistas[vistaActual].IMAGENSINPORTADA;
						}
					}
					configuracionJuegos[game].banner=imgData;

					// Draw banner
					__Gui_DrawBanner(covcol,covrow,0,game);

				}else{
					__Gui_DrawBanner(covcol,covrow,0,MAXJUEGOS);
				}
	
				covcol++;
				if(covcol==vistas[vistaActual].COLUMNAS[wideScreen]){
					covcol=0;
					covrow++;
				}
			}
			Fat_UnmountSDHC();
		}

		paginasLeidas++;




	}else{
		i=pagina*vistas[vistaActual].COLUMNAS[wideScreen]*vistas[vistaActual].FILAS;
		while(i<MAXJUEGOS && covrow<vistas[vistaActual].FILAS){
			// Draw banner
			__Gui_DrawBanner(covcol,covrow,0,vistas[vistaActual].orden[i]);
	
			covcol++;
			if(covcol==vistas[vistaActual].COLUMNAS[wideScreen]){
				covcol=0;
				covrow++;
			}
			i++;
		}
	}
	
	if(pagina>0)
		__Gui_DrawPng(imgLeftButton, PAGEBUTTONS_XCOORD, PAGEBUTTONS_YCOORD);

	if((pagina+1)<vistas[vistaActual].paginas)
		__Gui_DrawPng(imgRightButton, PAGEBUTTONS_XCOORD+PAGEBUTTONS_WIDTH, PAGEBUTTONS_YCOORD);
}

bool Open_Device(void){
	u32 status = 0;
	if (WDVD_GetCoverStatus(&status) != 0 || (status & 2) == 0) {
		// WDVD_WaitForDisc();

		printf("* Please mount a disc image with the wode\n");
		do
		{
			WPAD_ScanPads();
			s32 padsState = WPAD_ButtonsDown(0);
			if ((padsState & WPAD_BUTTON_B) != 0)
				break;
			usleep(100 * 1000);
		} while (WDVD_GetCoverStatus(&status) != 0 || (status & 2) == 0);
		if ((status & 2) == 0) return false;
		
		InitDVD();
	}

	/* Clear console */
	Con_Clear();

	printf("* Connection to Wode, please wait...");
	fflush(stdout);
	/* Initialize WBFS */
	WBFS_Init();

	//aqui inicializo mandos, para evitar el error raro
	//ya lo mejoraré
	Wpad_Init();

	/* Try to open device */
	if (partition[0] == '\0') {
		u32 partIdx = WBFS_GetDefaultPartition();
		WBFS_GetPartitionName(partIdx, partition);
	}
	WBFS_OpenNamed(partition);

	return true;
}

void Menu_Boot(){
	struct discHdr *header = NULL;

	s32 ret;

	/* No game list */
	if (!gameCnt)
		return;

	/* Selected game */
	header = &gameList[gameSelected];

	//initialize_small_console();
	//printf("\n* Ejecutando juego, por favor espera...\n");

	bool gc = header->magic == GC_MAGIC;
	
	WBFS_OpenDisc(header->id, header->game_idx);
	// Close the wode stuff
	WBFS_Close();
	use_dvdx = 0;

	/* Open disc */
	Disc_Init();
	Disc_Wait();
	ret = Disc_Open();
	if(ret < 0){
		printf("    ERROR: Could not open game! (ret = %d)\n", ret);
		goto out;
	}

	/* Boot Wii disc */
	u8 modoVideo=configuracionJuegos[gameSelected].modoVideo;

	u8 ocarina=configuracionJuegos[gameSelected].ocarina;

	if(ocarina && Fat_MountSDHC()<0)
		ocarina=0;

	if (gc) {
		WII_Initialize();
		ret = WII_LaunchTitle(0x0000000100000100ULL);
	} else {
		ret = Disc_WiiBoot(modoVideo,ocarina);
	}

	printf("    Returned! (ret = %d)\n", ret);

out:
	printf("\n");
	printf("    Press any button to continue...\n");

	/* Wait for button */
	Wpad_WaitButtons();
}

void save_changes(void){
	u32 cnt=0;

	char configpath[128];
	sprintf(configpath, USBLOADER_PATH "%s", USBLOADER_CONFIG_FILE);

	u32 configsize=(sizeof(char)*12)*gameCnt;

	configsize+=6; //configuracion: 'c'+vistaActual+wideScreen+volverAWii+ejecucionRapida+'\n'
	configsize+=(2+2*gameCnt+1)*MODOSDEVISTA; //2 cabecera+2*juegos+'\n'
	configsize+=strlen(partition);

	printf("\n * Saving changes...\n   Do not remove the SD card.");
	//printf("HAN HABIDO CAMBIOS... %d",size);

	char * outbufconfig=NULL;
	outbufconfig=malloc(configsize);

	u32 toSaveConfig=0;
	for(cnt=0;cnt<gameCnt;cnt++){
		struct discHdr *header = &gameList[cnt];
		outbufconfig[toSaveConfig+0]=header->id[0];
		outbufconfig[toSaveConfig+1]=header->id[1];
		outbufconfig[toSaveConfig+2]=header->id[2];
		outbufconfig[toSaveConfig+3]=header->id[3];
		outbufconfig[toSaveConfig+4]=header->id[4];
		outbufconfig[toSaveConfig+5]=header->id[5];

		outbufconfig[toSaveConfig+6]=configuracionJuegos[cnt].modoVideo;
		outbufconfig[toSaveConfig+7]=configuracionJuegos[cnt].anti002;
		outbufconfig[toSaveConfig+8]=configuracionJuegos[cnt].ocarina;
		outbufconfig[toSaveConfig+9]=configuracionJuegos[cnt].altdol;
		outbufconfig[toSaveConfig+10]=configuracionJuegos[cnt].diario;

		outbufconfig[toSaveConfig+11]='\n';
		toSaveConfig+=12;
	}

	int i,j;
	for(i=0;i<MODOSDEVISTA;i++){
		outbufconfig[toSaveConfig]='o';
		outbufconfig[toSaveConfig+1]=vistas[i].ID;
		toSaveConfig+=2;

		for(cnt=0;cnt<gameCnt;cnt++){//PROBAR!!!
			for(j=0;j<MAXJUEGOS;j++){
				if(vistas[i].orden[j]==cnt){
					u8 byte1=(j%224)+17; //para evitar que sea 0A (salto linea)
					u8 byte2=(j/224)+17;
					outbufconfig[toSaveConfig+0]=byte1;
					outbufconfig[toSaveConfig+1]=byte2;
					toSaveConfig+=2;
				}
			}
			/*
			if(j==MAXJUEGOS){ //algo IMPOSIBLE ha pasado!
			}
			*/
		}
		outbufconfig[toSaveConfig]='\n';
		toSaveConfig++;
	}

	outbufconfig[toSaveConfig]='c';
	outbufconfig[toSaveConfig+1]=vistaActual;
	outbufconfig[toSaveConfig+2]=wideScreen;
	outbufconfig[toSaveConfig+3]=volverAWii;
	outbufconfig[toSaveConfig+4]=ejecucionRapida;
	strncpy(outbufconfig + (toSaveConfig + 5), partition, strlen(partition));
	outbufconfig[toSaveConfig+5+strlen(partition)]='\n';

	int ret=Fat_MountSDHC();
	if(ret==0){
		Fat_SaveFile(configpath, (void *)&outbufconfig,configsize);
		Fat_UnmountSDHC();
	}

	haHabidoCambios=false;
}

bool descargar_imagenes(void){
	int ret,i;
	u16 descargados=0;
	char path[128];

	Con_Clear();
	printf("* Mounting SD...");
	ret=Fat_MountSDHC();
	if(ret<0){
		printf("ERROR!: Cannot mount SD (errno=%d)\n", ret);
		return false;
	}
	printf("OK!\n");

	printf("* Starting internet connection..."); fflush(stdout);
	for(;;){
		ret=net_init();
 		if(ret<0 && ret!=-EAGAIN){
			printf(" ERROR!: No internet connection (errno=%d)\n", ret);
			//net_deinit();
			return false;
    	}
		if(ret==0) //consigo conexion
			break;
		sleep(5);
		printf("."); fflush(stdout);
  	}
	printf(" OK!\n");

	for(i=0;i<gameCnt;i++){
		struct discHdr *header = &gameList[i];
		sprintf(path, USBLOADER_PATH "%s_%s.png",header->id,vistas[vistaActual].COLETILLA);
		if(!Fat_CheckFile(path)){
			sprintf(path, "http://usuaris.tinet.cat/mark/usbloader_mrc/%c%c%c_%s.png",header->id[0],header->id[1],header->id[2],vistas[vistaActual].COLETILLA);

			u8* outbuf=NULL;
			u32 outlen=0;
			u32 http_status=0;

			printf("  Downloading %c%c%c_%s.png...",header->id[0],header->id[1],header->id[2],vistas[vistaActual].COLETILLA);
			ret=http_request(path, 1 << 31);
			if(!ret){
				printf(" ERROR!: Not available?\n");
				continue;
			}

			ret = http_get_result(&http_status, &outbuf, &outlen); 

			if(outlen>32){//suficientes bytes
				__Gui_DrawPng(outbuf,12,12);
				sprintf(path, USBLOADER_PATH "%s_%s.png",header->id,vistas[vistaActual].COLETILLA);
				Fat_SaveFile(path, (void *)&outbuf,outlen);
				descargados++;
				printf(" OK.\n");
			}else{
				printf(" Not valid.\n");
			}

			if(outbuf!=NULL)
				free(outbuf);
		}
	}
	net_deinit();
	Fat_UnmountSDHC();

	if(descargados==0)
		printf("* You already have the images displaying :)\n");
	else
		inicializar_vista();

	return true;
}

void show_home_menu_2(void){
	u8 opcion=4;
	bool partChanged = false;
	for(;;){
		//Imprimir informacion del juego
		Con_Clear();
		printf("\n   ");
		set_console_fgcolor(YELLOW,1);
		printf(" Wode Loader mrc v13E\n");
		set_console_fgcolor(WHITE,1);

		printf("    By Marc R, English Translation By pepxl.\n");
		printf("    Wode version by r-win\n");
		printf("\n\n");
		printf("    GBATemp.net\n\n\n\n");

		printf(opcion==0? " >> ":"    ");
		printf("Quick Load(No Banner Sound):            ");
		printf(ejecucionRapida? "On\n":" Off\n");

		printf(opcion==1? " >> ":"    ");
		printf("Display Type:         ");
		printf(wideScreen? "		      Wide 16:9\n":"		     Normal 4:3\n");

		printf(opcion==2? " >> ":"    ");
		printf("Partition:             ");
		printf("			       %s\n", partition);
		
		printf(opcion==3? " >> ":"    ");
		printf("Return To Mode:        ");
		printf(volverAWii? "			   Wii Menu\n":"			        HBC\n");

		printf(opcion==4? " >> ":"    ");
		printf("Download images (%ss)\n",vistas[vistaActual].COLETILLA);
		printf("\n");
		printf(opcion==5? " >> ":"    ");
		printf("Save changes to SD\n");
		printf(opcion==6? " >> ":"    ");
		printf("Back");

		u32 button = Wpad_WaitButtons();
		if (button==WPAD_BUTTON_UP)
			if(opcion>0)
				opcion--;
			else
				opcion=6;
		else if (button==WPAD_BUTTON_DOWN)
			if(opcion==6)
				opcion=0;
			else
				opcion++;
		else if (button==WPAD_BUTTON_B || button==WPAD_BUTTON_HOME)
			break;
		else if (button==WPAD_BUTTON_A){
			if(opcion==0)
				ejecucionRapida=!ejecucionRapida;
			else if(opcion==1)
				wideScreen=!wideScreen;
			else if(opcion==2) { // Change partition
				u32 partIdx = WBFS_GetCurrentPartition() + 1;
				if (partIdx >= WBFS_GetPartitionCount()) {
					partIdx = 0;
				}
				WBFS_GetPartitionName(partIdx, (char *) &partition);
				partChanged = true;
			}
			else if(opcion==3)
				volverAWii=!volverAWii;
			else if(opcion==4){
				descargar_imagenes();
				printf("(Press any button to continue)\n");
				Wpad_WaitButtons();
			}
			else if(opcion==5)
				save_changes();
			else if(opcion==6)
				break;
		}
	}
	
	if (partChanged) {
		WBFS_OpenNamed(partition);
		__Menu_GetEntries();
	}
}

bool show_home_menu(void){
	//Abrir consola
	initialize_big_console();

	//Obtener informacion del juego
	struct discHdr *header;
	u8 opcion=0;

	header = &gameList[gameSelected];
	//char* gameId=substr((char*)header->id,0,6);
	unsigned char* gameId=header->id;
	u8 maxOcarina=2;

	if(Fat_MountSDHC()==0){
		char filepath[128];
		sprintf(filepath, USBLOADER_PATH "%s.gct", gameId);
		if(Fat_CheckFile(filepath)){
			while(Fat_CheckFile(filepath)){
				sprintf(filepath, USBLOADER_PATH "%s_%d.gct",gameId,maxOcarina);
				maxOcarina++;
			}
			maxOcarina--;
		}
		Fat_UnmountSDHC();
	}

	for(;;){
		//Imprimir informacion del juego
		Con_Clear();
		printf("\n   ");
		set_console_fgcolor(YELLOW,1);
		printf("%s\n", header->title);
		set_console_fgcolor(WHITE,1);
		printf("   (ID: %s)", gameId);

		printf("\n\n");
		printf(opcion==0? " >> ":"    ");
		printf("Video Mode:                 ");
		switch(configuracionJuegos[gameSelected].modoVideo){
			case 0: printf("     Automatic"); break;
			case 1: printf("     Force PAL"); break;
			case 2: printf("    Force NTSC"); break;
		}
		printf("\n");

		printf(opcion==1? " >> ":"    ");
		printf("Ocarina:                    ");
		if(configuracionJuegos[gameSelected].ocarina){
			if(configuracionJuegos[gameSelected].ocarina==1)
				printf("    %s.gct",gameId);
			else
				printf(" %s_%d.gct",gameId,configuracionJuegos[gameSelected].ocarina);
		}else{
			printf("           Off");
		}
		printf("\n");

		printf(opcion==2? " >> ":"    ");
		printf("Use Wii playstats:                     ");
		printf(configuracionJuegos[gameSelected].diario? "Yes":" No");
		printf("\n\n");

		printf(opcion==3? " >> ":"    ");
		printf("General Settings\n");
		printf(opcion==4? " >> ":"    ");
		printf(volverAWii? "Return To Wii Menu\n": "Return To HBC\n");

		u32 button = Wpad_WaitButtons();
		if (button==WPAD_BUTTON_UP)
			if(opcion>0)
				opcion--;
			else
				opcion=4;
		else if (button==WPAD_BUTTON_DOWN)
			if(opcion==4)
				opcion=0;
			else
				opcion++;
		else if (button==WPAD_BUTTON_B || button==WPAD_BUTTON_HOME)
			break;
		else if (button==WPAD_BUTTON_A){
			switch(opcion){
				case 0:
					configuracionJuegos[gameSelected].modoVideo++;
					if(configuracionJuegos[gameSelected].modoVideo==3)
						configuracionJuegos[gameSelected].modoVideo=0;
					break;
				case 1:
					configuracionJuegos[gameSelected].ocarina++;
					if(configuracionJuegos[gameSelected].ocarina==10 || configuracionJuegos[gameSelected].ocarina==maxOcarina)
						configuracionJuegos[gameSelected].ocarina=0;
					break;				
				case 2:
					configuracionJuegos[gameSelected].diario=!configuracionJuegos[gameSelected].diario;
					break;
				case 3: show_home_menu_2(); break;
				case 4: return true; break;
			}
		}
	}

	return false;
}

void __Menu_BuscarSkinSD(void){
	int i,j;
	char *imgData = NULL;
	s32 ret;

	//Abrir skin de la SD
	ret = Fat_ReadFile(USBLOADER_PATH "/mrc_background.png", (void*)&imgData);
	if(ret>0)
		imgFondo=imgData;
	else
		imgFondo=mrc_background_png;

	ret = Fat_ReadFile(USBLOADER_PATH "/mrc_left_button.png", (void*)&imgData);
	if(ret>0)
		imgLeftButton=imgData;
	else
		imgLeftButton=mrc_left_button_png;

	ret = Fat_ReadFile(USBLOADER_PATH "/mrc_right_button.png", (void*)&imgData);
	if(ret>0)
		imgRightButton=imgData;
	else
		imgRightButton=mrc_right_button_png;

	for(i=0;i<MODOSDEVISTA;i++){
		char* coletilla=(char*)vistas[i].COLETILLA;
		char* textos[6]={"container","container_wide","selector","selector_wide","noimg","empty"};
		for(j=0;j<6;j++){
			char filepath[128];
			sprintf(filepath,USBLOADER_PATH "/mrc_%s_%s.png",coletilla,textos[j]);
			ret = Fat_ReadFile(filepath, (void*)&imgData);
			if(ret>0){
				if(j==0)
					vistas[i].IMAGENCONTENEDOR[0]=imgData;
				else if(j==1)
					vistas[i].IMAGENCONTENEDOR[1]=imgData;
				else if(j==2)
					vistas[i].IMAGENSELECTOR[0]=imgData;
				else if(j==3)
					vistas[i].IMAGENSELECTOR[1]=imgData;
				else if(j==4)
					vistas[i].IMAGENSINPORTADA=imgData;
				else if(j==5)
					vistas[i].IMAGENHUECO=imgData;
			}
		}
	}
}
	
void Menu_Loop(void){
	imgFondo=mrc_background_png;
	imgLeftButton=mrc_left_button_png;
	imgRightButton=mrc_right_button_png;
	if(Fat_MountSDHC()==0){
		__Menu_BuscarSkinSD();
		if(!Fat_CheckDir(USBLOADER_PATH_SINBARRA))
			mkdir(USBLOADER_PATH_SINBARRA,S_IREAD | S_IWRITE);
	}
	
	// Draw background
	__Gui_DrawPng(imgFondo, 0, 0);
	initialize_small_console();

	// Open HDD
	if(!Open_Device()){
		return;
	}

	// Get game list
	__Menu_GetEntries();

	Fat_UnmountSDHC();

	// Draw first page
	__Gui_DrawPageBanners();

	s32 moviendo=-1;
	u32 posicionCursor=0;
	s32 poderMoverConDos=-1;;
	for(;;){
		//VIDEO_WaitVSync();
		if(gameCnt==0){
			initialize_big_console();
			if(gameCnt==0){
				break;
			}else{
				__Gui_DrawPageBanners();
				initialize_small_console();
			}
		}

		posicionCursor=pagina*vistas[vistaActual].COLUMNAS[wideScreen]*vistas[vistaActual].FILAS+fila*vistas[vistaActual].COLUMNAS[wideScreen]+columna;
		gameSelected=vistas[vistaActual].orden[posicionCursor];

		struct discHdr *header;
		if(gameSelected!=MAXJUEGOS){
			header = &gameList[gameSelected];
		}else{
			header = &gameList[0];
		}

		if(moviendo==-1){
			__Gui_DrawBanner(columna,fila,1,gameSelected);
			if(gameSelected!=MAXJUEGOS){
				printf("\n%s", header->title);
			}else{
				printf("\n ");
			}
			printf("\n                 1:Order   B:View   HOME:Menu");
		}else{
			__Gui_DrawBanner(columna,fila,1,vistas[vistaActual].orden[moviendo]);
			__Gui_DrawPng(imgRightButton, PAGEBUTTONS_XCOORD+PAGEBUTTONS_WIDTH, PAGEBUTTONS_YCOORD);
			printf("\nMoving game...\n");
			printf("1: Swap   ");
			if(poderMoverConDos==pagina){
				printf("2: Move Game");
			}else{
				printf("              ");
			}
			printf("      B:Cancel");

		}		

		u32 buttons = Wpad_WaitButtons();



		__Gui_DrawBanner(columna,fila,0,gameSelected);

		//ARRIBA y ABAJO
		if (buttons==WPAD_BUTTON_UP && fila>0)
			fila--;
		if (buttons==WPAD_BUTTON_DOWN && fila<vistas[vistaActual].FILAS-1)
			fila++;
	
		//IZQUIERDA y DERECHA
		if (buttons==WPAD_BUTTON_LEFT){
			if(columna==0){
				if(fila==0 && pagina>0){
					buttons=WPAD_BUTTON_MINUS;
					columna=vistas[vistaActual].COLUMNAS[wideScreen]-1;
					fila=vistas[vistaActual].FILAS-1;
				}else if(fila>0){
					fila--;
					columna=vistas[vistaActual].COLUMNAS[wideScreen]-1;
				}

			}else if(columna>0)
				columna--;
		}	
		if (buttons==WPAD_BUTTON_RIGHT){
			if(columna==vistas[vistaActual].COLUMNAS[wideScreen]-1){
				columna=0;
				fila++;
				if(fila==vistas[vistaActual].FILAS){
					buttons=WPAD_BUTTON_PLUS;
					columna=0;
					fila=0;
				}
			}
			else if(posicionCursor<MAXJUEGOS-1)
				columna++;
		}

		//+ y -
		if (buttons==WPAD_BUTTON_PLUS && ((pagina+1)*vistas[vistaActual].COLUMNAS[wideScreen]*vistas[vistaActual].FILAS)<MAXJUEGOS){
			if((moviendo>-1 && pagina<vistas[vistaActual].paginas) || (moviendo==-1 && pagina+1<vistas[vistaActual].paginas)){
				pagina++;
				__Gui_DrawPageBanners();
			}
		}	
		if (buttons==WPAD_BUTTON_MINUS && pagina>0){
			pagina--;
			__Gui_DrawPageBanners();
		}
		
		s32 futureGame=pagina*vistas[vistaActual].COLUMNAS[wideScreen]*vistas[vistaActual].FILAS+fila*vistas[vistaActual].COLUMNAS[wideScreen]+columna;
		while(!(futureGame<MAXJUEGOS)){
			if(fila>0){
				fila--;
			}else{
				if(columna>0){
					columna--;
				}else{
					pagina--;
				}
			}
			futureGame=pagina*vistas[vistaActual].COLUMNAS[wideScreen]*vistas[vistaActual].FILAS+fila*vistas[vistaActual].COLUMNAS[wideScreen]+columna;
		}


		//Boton HOME: abrir opciones
		if (buttons==WPAD_BUTTON_HOME && moviendo==-1 && gameSelected!=MAXJUEGOS){
			bool oldScreen=wideScreen;
			bool exit=show_home_menu();
			if(exit)
				break;

			if(oldScreen!=wideScreen){
				inicializar_vista();
			}
			
			__Gui_DrawPng(imgFondo, 0, 0);
			initialize_small_console();
			__Gui_DrawPageBanners();
		}


		//Boton B: cambiar vista/cancelar reordenar
		if (buttons==WPAD_BUTTON_B){
			if(moviendo==-1){
				vistaActual++;
				if(vistaActual==MODOSDEVISTA)
					vistaActual=0;

				inicializar_vista();
				__Gui_DrawPageBanners();
			}else{
				moviendo=-1;
			}
		}


		//Boton 1/2: reordenar juegos
		if(buttons==WPAD_BUTTON_1 || buttons==WPAD_BUTTON_2){
			if(moviendo==-1){
				moviendo=posicionCursor;
				poderMoverConDos=pagina;
			}else if(posicionCursor!=moviendo){
				if(buttons==WPAD_BUTTON_1){
					u16 copia0=vistas[vistaActual].orden[moviendo];
					u16 copia1=vistas[vistaActual].orden[posicionCursor];
					vistas[vistaActual].orden[moviendo]=copia1;
					vistas[vistaActual].orden[posicionCursor]=copia0;

					__Gui_DrawPageBanners();
					haHabidoCambios=true;
				}else if(buttons==WPAD_BUTTON_2 && poderMoverConDos==pagina){
					s32 viejaPos=moviendo;
					s32 nuevaPos=pagina*vistas[vistaActual].COLUMNAS[wideScreen]*vistas[vistaActual].FILAS+fila*vistas[vistaActual].COLUMNAS[wideScreen]+columna;
					s32 recorrer;
					if(viejaPos<nuevaPos){
						for(recorrer=viejaPos+1;recorrer<nuevaPos+1;recorrer++){
							u16 copia0=vistas[vistaActual].orden[recorrer];
							u16 copia1=vistas[vistaActual].orden[recorrer-1];
							vistas[vistaActual].orden[recorrer]=copia1;
							vistas[vistaActual].orden[recorrer-1]=copia0;
						}
					}else{
						for(recorrer=viejaPos-1;recorrer>nuevaPos-1;recorrer--){
							u16 copia0=vistas[vistaActual].orden[recorrer];
							u16 copia1=vistas[vistaActual].orden[recorrer+1];
							vistas[vistaActual].orden[recorrer]=copia1;
							vistas[vistaActual].orden[recorrer+1]=copia0;
						}
					}

					__Gui_DrawPageBanners();
					haHabidoCambios=true;
				}
				moviendo=-1;
				calcularPaginas(vistaActual);
			}else{
				moviendo=-1;
			}
		}




		//Boton A: Ejecutar juego
		if (gameSelected!=MAXJUEGOS && buttons==WPAD_BUTTON_A && moviendo==-1){
			int sep=(vistas[vistaActual].PRIMERAFILA+fila*(vistas[vistaActual].ALTOIMAGEN+vistas[vistaActual].SEPARACION))-vistas[vistaActual].BORDEIMAGEN;
			int sep2=vistas[vistaActual].ALTOIMAGEN+sep+vistas[vistaActual].BORDEIMAGEN*2;
			int sep3=(vistas[vistaActual].PRIMERACOLUMNA[wideScreen]+columna*(vistas[vistaActual].ANCHOIMAGEN[wideScreen]+vistas[vistaActual].SEPARACION))-vistas[vistaActual].BORDEIMAGEN;
			int sep4=sep3+vistas[vistaActual].ANCHOIMAGEN[wideScreen]+vistas[vistaActual].BORDEIMAGEN*2;
			
			int i=0;
			__Gui_DrawBanner(columna,fila,1,gameSelected);
			for(i=0;i<6;i++){
				Video_Oscurecer(0, 0, 640, sep);

				Video_Oscurecer(0, sep, sep3, sep2);

				Video_Oscurecer(sep4, sep, 640, sep2);

				Video_Oscurecer(0, sep2, 640, 480);
			}

			/*** SONIDO BANNER ***/
			if(ejecucionRapida){
				buttons=WPAD_BUTTON_A;
			}else{
				initialize_small_console();
				printf("\n\n\n                 A:Start   [Other key]:Back");
/*			
				//reproducir sonido banner
				ASND_Init();
				ASND_Pause(0);
				SoundInfo snd;
				memset(&snd, 0, sizeof(snd));
				WBFS_BannerSound(header->id, &snd);
				if (snd.dsp_data) {
					int fmt = (snd.channels == 2) ? VOICE_STEREO_16BIT : VOICE_MONO_16BIT;

					if(snd.loop){
						ASND_SetInfiniteVoice(0, fmt, snd.rate, 0,			
							snd.dsp_data, snd.size,
							160,160); //volumen,volumen
					}else{
						SND_SetVoice(0, fmt, snd.rate, 0,
							snd.dsp_data, snd.size,
							96,96, //volumen,volumen,
							NULL); //DataTransferCallback
					}
				}
*/
				buttons=Wpad_WaitButtons();
/*
				//detener sonido banner
				if (snd.dsp_data) {
					SND_StopVoice(0);
					ASND_End();

					free(snd.dsp_data);
					snd.dsp_data=NULL;
				}
*/				
			}

			/* Wait for user answer */
			if(buttons==WPAD_BUTTON_A){
				if(!ejecucionRapida)
					Video_Oscurecer(sep3, sep, sep4, sep2);

				if(haHabidoCambios)
					save_changes();

				if(configuracionJuegos[gameSelected].diario){
//					char* bannerTitle=WBFS_BannerTitle(header->id);
					char *bannerTitle= (char *) &header->title;
					if(Diario_ActualizarDiario(bannerTitle,(char*)header->id)<0)
						Wpad_WaitButtons();
//					free(bannerTitle);
				}

				Menu_Boot();
			}
			__Gui_DrawPageBanners();
		}
	}

	if(haHabidoCambios)
		save_changes();

	if(volverAWii)
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);		
}
