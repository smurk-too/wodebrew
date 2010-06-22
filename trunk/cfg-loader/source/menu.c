
// Modified by oggzee & usptactical

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <ctype.h>
#include <wctype.h>
#include <wchar.h>
#include <locale.h>
#include <asndlib.h>

#include "disc.h"
#include "fat.h"
#include "cache.h"
#include "gui.h"
#include "menu.h"
#include "restart.h"
#include "sys.h"
#include "util.h"
#include "utils.h"
#include "video.h"
#include "wbfs.h"
#include "wpad.h"
#include "patchcode.h"
#include "cfg.h"
#include "http.h"
#include "dns.h"
#include "wdvd.h"
#include "music.h"
#include "subsystem.h"
#include "net.h"
#include "fst.h"
#include "wiip.h"
#include "xml.h" /* WiiTDB database - Lustar */
#include "sort.h"
#include "gettext.h"
#include "playlog.h"
#include "wode.h"

#define CHANGE(V,M) {V+=change; if(V>(M)) V=(M); if(V<0) V=0;}


char CFG_VERSION[] = CFG_VERSION_STR;

void Sys_Exit();

extern int gui_style;
extern long long gettime();
extern u32 diff_msec(long long start,long long end);
extern int __console_disable;

/* Gamelist buffer */
struct discHdr *all_gameList = NULL;
static struct discHdr *fav_gameList = NULL;
struct discHdr *gameList = NULL;
struct discHdr *filter_gameList = NULL;

/* Gamelist variables */
bool enable_favorite = false;
s32 all_gameCnt = 0;
s32 fav_gameCnt = 0;
s32 filter_gameCnt = 0;
extern s32 filter_type;
extern s32 filter_index;

s32 gameCnt = 0, gameSelected = 0, gameStart = 0;

bool imageNotFound = false;

/* admin unlock mode variables */
static int disable_options = 0;
static int confirm_start = 1;
static bool unlock_init = true;

/*VIDEO OPTION - hungyip84 */
char videos[CFG_VIDEO_MAX+1][15] = 
{
{"System Def."},
{"Game Default"},
{"Force PAL50"},
{"Force PAL60"},
{"Force NTSC"}
//{"Patch Game"},
};

/*LANGUAGE PATCH - FISHEARS*/
char languages[11][22] =
{
{"Console Def."},
{"Japanese"},
{"English"},
{"German"},
{"French"},
{"Spanish"},
{"Italian"},
{"Dutch"},
{"S. Chinese"},
{"T. Chinese"},
{"Korean"}
};
/*LANGUAGE PATCH - FISHEARS*/

int Menu_Global_Options();
int Menu_Game_Options();
void Switch_Favorites(bool enable);

char action_string[40] = "";

#ifdef FAKE_GAME_LIST
// Debug Test mode with fake game list
#define WBFS_GetCount dbg_WBFS_GetCount
#define WBFS_GetHeaders dbg_WBFS_GetHeaders
#endif

char *skip_sort_ignore(char *s)
{
	char tok[200], *p;
	int len;
	p = CFG.sort_ignore;
	while (p) {
		p = split_token(tok, p, ',', sizeof(tok));
		len = strlen(tok);
		if (len && strncasecmp(s, tok, strlen(tok)) == 0) {
			if (s[len] == ' ' || s[len] == '\'') {
				s += len + 1;
			}
		}
	}
	return s;
}

s32 __Menu_GetEntries(void)
{
	struct discHdr *buffer = NULL;

	u32 cnt, len;
	s32 ret;

	Cache_Invalidate();

	Switch_Favorites(false);
	SAFE_FREE(fav_gameList);
	fav_gameCnt = 0;
	
	
	SAFE_FREE(filter_gameList);
	filter_gameCnt = 0;

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

	/* Sort entries */
	__set_default_sort();
	qsort(buffer, cnt, sizeof(struct discHdr), default_sort_function);

	// hide and re-sort preffered
	if (!CFG.admin_lock || CFG.admin_mode_locked)
		cnt = CFG_hide_games(buffer, cnt);
	CFG_sort_pref(buffer, cnt);

	/* Free memory */
	if (gameList)
		free(gameList);

	/* Set values */
	gameList = buffer;
	gameCnt  = cnt;

	/* Reset variables */
	gameSelected = gameStart = 0;

	// init favorites
	all_gameList = gameList;
	all_gameCnt  = gameCnt;
	len = sizeof(struct discHdr) * all_gameCnt;
	fav_gameList = (struct discHdr *)memalign(32, len);
	if (fav_gameList) {
		memcpy(fav_gameList, all_gameList, len);
		fav_gameCnt = all_gameCnt;
	}
	
	filter_gameList = (struct discHdr *)memalign(32, len);
	if (filter_gameList) {
		memcpy(filter_gameList, all_gameList, len);
		filter_gameCnt = all_gameCnt;
	}

	return 0;

err:
	/* Free memory */
	if (buffer)
		free(buffer);

	return ret;
}

void Switch_Favorites(bool enable)
{
	int i, len;
	u8 *id = NULL;
	// filter
	if (fav_gameList) {
		len = sizeof(struct discHdr) * all_gameCnt;
		memcpy(fav_gameList, all_gameList, len);
		fav_gameCnt = CFG_filter_favorite(fav_gameList, all_gameCnt);
	}
	// switch
	//printf("fav %d %p %d\n", enable, fav_gameList, fav_gameCnt); sleep(5);
	if (fav_gameList == NULL || fav_gameCnt == 0) enable = false;
	if (gameSelected < gameCnt) {
		id = gameList[gameSelected].id;
	}
	if (enable) {
		gameList = fav_gameList;
		gameCnt = fav_gameCnt;
	} else {
		gameList = all_gameList;
		gameCnt = all_gameCnt;
	}
	enable_favorite = enable;
	// find game selected
	gameStart = 0;
	gameSelected = 0;
	for (i=0; i<gameCnt; i++) {
		if (strncmp((char*)gameList[i].id, (char*)id, 6) == 0) {
			gameSelected = i;
			break;
		}
	}
	// scroll start list
	__Menu_ScrollStartList();
}

char *__Menu_PrintTitle(char *name)
{
	//static char buffer[MAX_CHARACTERS + 4];
	static char buffer[200];
	int len = con_len(name);

	/* Clear buffer */
	memset(buffer, 0, sizeof(buffer));

	/* Check string length */
	if (len >= MAX_CHARACTERS) {
		//strncpy(buffer, name,  MAX_CHARACTERS - 4);
		STRCOPY(buffer, name);
		con_trunc(buffer, MAX_CHARACTERS - 4);
		strcat(buffer, "...");
		return buffer;
	}

	return name;
}

void __Menu_MoveList(s8 delta)
{
	/* No game list */
	if (!gameCnt)
		return;

	#if 0
	/* Select next entry */
	gameSelected += delta;

	/* Out of the list? */
	if (gameSelected <= -1)
		gameSelected = (gameCnt - 1);
	if (gameSelected >= gameCnt)
		gameSelected = 0;
	#endif

	if(delta>0) {
		if(gameSelected == gameCnt - 1) {
			gameSelected = 0;
		}
		else {
			gameSelected +=delta;
			if(gameSelected >= gameCnt) {
				gameSelected = (gameCnt - 1);
			}
		}
	}
	else {
		if(!gameSelected) {
			gameSelected = gameCnt - 1;
		}
		else {
			gameSelected +=delta;
			if(gameSelected < 0) {
				gameSelected = 0;
			}
		}
	}

	/* List scrolling */
	__Menu_ScrollStartList();
}

void __Menu_ScrollStartList()
{
	s32 index = (gameSelected - gameStart);

	if (index >= ENTRIES_PER_PAGE)
		gameStart += index - (ENTRIES_PER_PAGE - 1);
	if (index <= -1)
		gameStart += index;
}

void __Menu_ShowList(void)
{
	FgColor(CFG.color_header);
	if (enable_favorite) {
		printf_x(gt("Favorite Games"));
		printf(":\n");
	} else {
		if (!CFG.hide_header) {
			printf_x(gt("Select the game you want to boot"));
			printf(":\n");
		}
	}
	DefaultColor();
	if (CFG.console_mark_page && gameStart > 0) {
		printf(" %s +", CFG.cursor_space);
	}
	printf("\n");

	/* No game list*/
	if (gameCnt) {
		u32 cnt;

		/* Print game list */
		for (cnt = gameStart; cnt < gameCnt; cnt++) {
			struct discHdr *header = &gameList[cnt];

			/* Entries per page limit reached */
			if ((cnt - gameStart) >= ENTRIES_PER_PAGE)
				break;

			if (gameSelected == cnt) {
				FgColor(CFG.color_selected_fg);
				BgColor(CFG.color_selected_bg);
				Con_ClearLine();
			} else {
				DefaultColor();
			}

			/* Print entry */
			//printf(" %2s %s\n", (gameSelected == cnt) ? ">>" : "  ",
			char *title = __Menu_PrintTitle(get_title(header));
			// cursor
			printf(" %s", (gameSelected == cnt) ? CFG.cursor : CFG.cursor_space);
			// favorite mark
			printf("%s", (CFG.console_mark_favorite && is_favorite(header->id))
					? CFG.favorite : " ");
			// title
			printf("%s", title);
			// saved mark
			if (CFG.console_mark_saved) {
				printf("%*s", (MAX_CHARACTERS - con_len(title)),
					(CFG_is_saved(header->id)) ? CFG.saved : " ");
			}
			printf("\n");
		}
		DefaultColor();
		if (CFG.console_mark_page && cnt < gameCnt) {
			printf(" %s +", CFG.cursor_space);
		} else {
			printf(" %s  ", CFG.cursor_space);
		}
		//if (CFG.hide_hddinfo) {
		FgColor(CFG.color_footer);
		BgColor(CONSOLE_BG_COLOR);
		int num_page = 1 + (gameCnt - 1) / ENTRIES_PER_PAGE;
		int cur_page = 1 + gameSelected / ENTRIES_PER_PAGE;
		printf(" %-*.*s %d/%d", MAX_CHARACTERS - 8, MAX_CHARACTERS - 8, action_string, cur_page, num_page);
		action_string[0] = 0;
		//}
	} else {
		printf(" ");
		printf(gt("%s No games found!!"), CFG.cursor);
		printf("\n");
	}

	/* Print free/used space */
	FgColor(CFG.color_footer);
	BgColor(CONSOLE_BG_COLOR);
	if (!CFG.hide_footer) {
		printf("\n");
		// (B) GUI (1) Options (2) Favorites
		// B: GUI 1: Options 2: Favorites
		//char c_gui = 'B', c_opt = '1';
		//if (CFG.buttons == CFG_BTN_OPTIONS_B) {
		//	c_gui = '1'; c_opt = 'B';
		//}
		printf_("");
		if (CFG.gui && CFG.button_gui) {
			printf("%s: GUI ", (button_names[CFG.button_gui]));
		}
		if (!CFG.disable_options && CFG.button_opt) {
			printf("%s: Options ", (button_names[CFG.button_opt]));
		}
		if (CFG.button_fav) {
			printf("%s: Favorites", (button_names[CFG.button_fav]));
		}
	}
	if (CFG.db_show_info) {
		printf("\n");
		//load game info from XML - lustar
		__Menu_ShowGameInfo(false, gameList[gameSelected].id);
	}
	DefaultColor();
	__console_flush(0);
}

/* load game info from XML - lustar */
void __Menu_ShowGameInfo(bool showfullinfo, u8 *id)
{
	if (LoadGameInfoFromXML(id)) {
		FgColor(CFG.color_inactive);
		PrintGameInfo(showfullinfo);
		//printf("Play Count: %d\n", getPlayCount(id));
		DefaultColor();
	}
}

void __Menu_ShowCover(void)
{
	struct discHdr *header = &gameList[gameSelected];

	/* No game list*/
	if (!gameCnt)
		return;

	/* Draw cover */
	Gui_DrawCover(header->id);
}

bool Save_ScreenShot(char *fn, int size)
{
	int i;
	struct stat st;
	for (i=1; i<100; i++) {
		snprintf(fn, size, "%s/screenshot%d.png", USBLOADER_PATH, i);
		if (stat(fn, &st)) break;
	}
	return ScreenShot(fn);
}

void Make_ScreenShot()
{
	bool ret;
	char fn[200];
	ret = Save_ScreenShot(fn, sizeof(fn));
	printf("\n%s: %s\n", ret ? "Saved" : "Error Saving", fn);
	sleep(1);
}

void Handle_Home(int disable_screenshot)
{
	if (CFG.home == CFG_HOME_EXIT) {
		Con_Clear();
		printf("\n");
		printf_("Exiting...");
		__console_flush(0);
		Sys_Exit();
	} else if (CFG.home == CFG_HOME_SCRSHOT) {
		__console_flush(0);
		Make_ScreenShot();
		if (disable_screenshot)	CFG.home = CFG_HOME_EXIT;
	} else if (CFG.home == CFG_HOME_HBC) {
		Con_Clear();
		printf("\n");
		printf_("HBC...");
		__console_flush(0);
		Sys_HBC();
	} else if (CFG.home == CFG_HOME_REBOOT) { 
		Con_Clear();
		Restart();
	} else {
		// Priiloader magic words, and channels
		if ((CFG.home & 0xFF) < 'a') {
			// upper case final letter implies channel
			Con_Clear();
			Sys_Channel(CFG.home);
		} else {
			// lower case final letter implies magic word
			Con_Clear();
			*(vu32*)0x8132FFFB = CFG.home;
			Restart();
		}
	}
}

void Print_SYS_Info()
{
	FgColor(CFG.color_inactive);
	printf_("");
	Fat_print_sd_mode();
	printf_(gt("CFG base: %s"), USBLOADER_PATH);
	printf("\n");
	if (strcmp(LAST_CFG_PATH, USBLOADER_PATH)) {
		// if last cfg differs, print it out
		printf_(gt("Additional config:"));
		printf("\n");
		printf_("  %s/config.txt\n", LAST_CFG_PATH);
	}
	printf_(gt("Loader Version: %s"), CFG_VERSION);
	printf("\n");
	printf_("IOS%u (Rev %u)\n",
			IOS_GetVersion(), IOS_GetRevision());
	DefaultColor();
}

// WiiMote to char map for admin mode unlocking
char get_unlock_buttons(buttons)
{
	switch (buttons)
	{
		case WPAD_BUTTON_UP:	return 'U';
		case WPAD_BUTTON_DOWN:	return 'D';
		case WPAD_BUTTON_LEFT:	return 'L';
		case WPAD_BUTTON_RIGHT:	return 'R';
		case WPAD_BUTTON_A:	return 'A';
		case WPAD_BUTTON_B:	return 'B';
		case WPAD_BUTTON_MINUS:	return 'M';
		case WPAD_BUTTON_PLUS:	return 'P';
		case WPAD_BUTTON_1:	return '1';
		case WPAD_BUTTON_2:	return '2';
		case WPAD_BUTTON_HOME:	return 'H';
	}
	return 'x';
}

void Menu_Unlock() {
	u32 buttons;
	static long long t_start;
	long long t_now;
	unsigned ms_diff = 0;
	int i = 0;
	char buf[16];
	bool unlocked = false;
	memset(buf, 0, sizeof(buf));

	//init the previous settings
	if (unlock_init) {
		disable_options = CFG.disable_options;
		confirm_start = CFG.confirm_start;
		unlock_init = false;
	}
	
	Con_Clear();
	printf(gt("Configurable Loader %s"), CFG_VERSION);
	printf("\n\n");

	if (CFG.admin_mode_locked) {
		printf(gt("Enter Code: "));
		t_start = gettime();
		
		while (ms_diff < 30000 && !unlocked) {
			buttons = Wpad_GetButtons();
			if (buttons) {
				printf("*");
				buf[i] = get_unlock_buttons(buttons);
				i++;
				if (stricmp(buf, CFG.unlock_password) == 0) {
					unlocked = true;
				}
				if (i >= 10) break;
			}
			VIDEO_WaitVSync();
			t_now = gettime();
			ms_diff = diff_msec(t_start, t_now);
		}
	}
	if (unlocked) {
		//save existing settings
		disable_options = CFG.disable_options;
		confirm_start = CFG.confirm_start;

		//enable all "admin-type" screens
		CFG.disable_options = 0;
		CFG.confirm_start = 1;
		CFG.admin_mode_locked = 0;
		printf("\n\n");
		printf(gt("SUCCESS!"));
		sleep(1);
		//show the hidden games
		__Menu_GetEntries();
	} else {
		//set the lock back on
		CFG.disable_options = disable_options;
		CFG.confirm_start = confirm_start;
		CFG.admin_mode_locked = 1;
		printf("\n\n");
		printf(gt("LOCKED!"));
		sleep(1);
		//reset the hidden games
		__Menu_GetEntries();
	}
}


int Menu_Views()
{
	struct discHdr *header = NULL;
	int redraw_cover = 0;
	struct Menu menu;

	if (gameCnt) {
		header = &gameList[gameSelected];
	}
	
	const int NUM_OPT = 8;
	char active[NUM_OPT];
	menu_init(&menu, NUM_OPT);

	for (;;) {

		menu.line_count = 0;
		menu_init_active(&menu, active, sizeof(active));

		active[3] = 0; // disable_remove
		active[4] = 0; // disable_install
		
		if (CFG.disable_options) {
			active[1] = 0;
			active[2] = 0;
		}
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Main Menu"));
		printf(":\n\n");
		
		DefaultColor();
		MENU_MARK();
		printf("<%s>\n", gt("Start Game"));
		MENU_MARK();
		printf("<%s>\n", gt("Game Options"));
		MENU_MARK();
		printf("<%s>\n", gt("Global Options"));
		MENU_MARK();
		printf("<%s>\n", gt("Sort Games"));
		MENU_MARK();
		printf("<%s>\n", gt("Filter Games"));
		MENU_MARK();
		printf("<%s>\n", gt("Boot Disc"));
   
		DefaultColor();

		printf("\n");
		printf_h(gt("Press %s button to select."),
				(button_names[CFG.button_confirm.num]));
		printf("\n");
		DefaultColor();
		__console_flush(0);

		if (redraw_cover) {
			if (header) Gui_DrawCover(header->id);
			redraw_cover = 0;
		}
		
		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move_active(&menu, buttons);
		
		int change = -2;
		if (buttons & WPAD_BUTTON_LEFT) change = -1;
		if (buttons & WPAD_BUTTON_RIGHT) change = +1;
		if (buttons & CFG.button_confirm.mask) change = 0;
//		#define CHANGE(V,M) {V+=change;if(V>M)V=M;if(V<0)V=0;}

		if (change > -2) {
			switch (menu.current) {
			case 0:
				CFG.confirm_start = 0;
				Menu_Boot(0);
				break;
			case 1:
				Menu_Game_Options();
				break;
			case 2:
				Menu_Global_Options();
				break;
			case 3:
				Menu_Sort();
				break;
			case 4:
				Menu_Filter();
				break;
			case 5:
				Menu_Boot(1);
				break;
			}
		}

		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_cancel.mask) break;
	}
	
	return 0;
}

int Menu_Game_Options() {
	return Menu_Boot_Options(&gameList[gameSelected]);
}

int Menu_Boot_Options(struct discHdr *header) {

	int ret_val = 0;
	if (CFG.disable_options) return 0;

	struct Game_CFG_2 *game_cfg2 = NULL;
	struct Game_CFG *game_cfg = NULL;
	int opt_saved, opt_ios_reload; 
	int redraw_cover = 0;
	int rows, cols, win_size;
	CON_GetMetrics(&cols, &rows);
	if ((win_size = rows-9) < 3) win_size = 3;
	Con_Clear();
	FgColor(CFG.color_header);
	printf_x(gt("Selected Game"));
	printf(":");
	__console_flush(0);
	printf(" (%.6s)\n", header->id);

	DefaultColor();
	printf(" %s %s\n\n", CFG.cursor_space, __Menu_PrintTitle(get_title(header)));
	__console_flush(0);
//	load_dolmenu((char*)header->id);

	game_cfg2 = CFG_get_game(header->id);
	if (!game_cfg2) {
		printf(gt("ERROR game opt"));
		printf("\n");
		sleep(5);
		return 0;
	}
	game_cfg = &game_cfg2->curr;

	struct Menu menu;
	const int NUM_OPT = 16;
	char active[NUM_OPT];
	menu_init(&menu, NUM_OPT);

	for (;;) {
		/*
		// fat on 249?
		if (wbfs_part_fs && !disc) {
			if (!is_ios_idx_mload(game_cfg->ios_idx))
			{
				game_cfg->ios_idx = CFG_IOS_222_MLOAD;
			}
		}
		*/

		menu_init_active(&menu, active, sizeof(active));
		opt_saved = game_cfg2->is_saved;
		// if not mload disable block ios reload opt
		opt_ios_reload = game_cfg->block_ios_reload;

		// if not ocarina and not wiird, deactivate hooks
		if (!game_cfg->ocarina && !CFG.wiird) {
			active[11] = 0;
		}
		//if admin lock is off or they're not in admin 
		// mode then they can't hide/unhide games
		if (!CFG.admin_lock || CFG.admin_mode_locked) {
			active[14] = 0;
		}

		//These things shouldn't be changed if using a disc...maybe
		active[0] = 0;
		active[8] = 0;
		active[9] = 0;
		active[14] = 0;
		
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Selected Game"));
		printf(":");
		printf(" (%.6s)\n", header->id);
		DefaultColor();
		printf(" %s %s\n\n", CFG.cursor_space, __Menu_PrintTitle(get_title(header)));
		FgColor(CFG.color_header);
		printf_x(gt("Game Options:  %s"),
				CFG_is_changed(header->id) ? gt("[ CHANGED ]") :
				opt_saved ? gt("[ SAVED ]") : "");
		printf("\n");
		DefaultColor();
		char c1 = '<', c2 = '>';
		//if (opt_saved) { c1 = '['; c2 = ']'; }

		const char *str_vpatch[3];
		str_vpatch[0] = gt("Off");
		str_vpatch[1] = gt("On");
		str_vpatch[2] = gt("All");

		// start menu draw

		menu_begin(&menu);
		menu_jump_active(&menu);

		#define PRINT_OPT_S(N,V) \
			printf("%s%c %s %c\n", con_align(N,18), c1, V, c2)

		#define PRINT_OPT_A(N,V) \
			printf("%s%c%s%c\n", con_align(N,18), c1, V, c2)

		#define PRINT_OPT_B(N,V) \
			PRINT_OPT_S(N,(V?gt("On"):gt("Off"))) 

		menu_window_begin(&menu, win_size, NUM_OPT);
		if (menu_window_mark(&menu))
			PRINT_OPT_S(gt("Favorite:"), is_favorite(header->id) ? gt("Yes") : gt("No"));
		if (menu_window_mark(&menu))
			PRINT_OPT_S(gt("Language:"), languages[game_cfg->language]);
		if (menu_window_mark(&menu))
			PRINT_OPT_S(gt("Video:"), videos[game_cfg->video]);
		if (menu_window_mark(&menu))
			PRINT_OPT_S(gt("Video Patch:"), str_vpatch[game_cfg->video_patch]);
		if (menu_window_mark(&menu))
			PRINT_OPT_B("VIDTV:", game_cfg->vidtv);
		if (menu_window_mark(&menu))
			PRINT_OPT_B(gt("Country Fix:"), game_cfg->country_patch);
		if (menu_window_mark(&menu))
			PRINT_OPT_B(gt("Anti 002 Fix:"), game_cfg->fix_002);
		if (menu_window_mark(&menu))
			PRINT_OPT_B(gt("Block IOS Reload:"), opt_ios_reload);
		if (menu_window_mark(&menu))
			PRINT_OPT_B(gt("Ocarina (cheats):"), game_cfg->ocarina);
		if (menu_window_mark(&menu))
			PRINT_OPT_S(gt("Hook Type:"), hook_name[game_cfg->hooktype]);
		if (menu_window_mark(&menu))
			PRINT_OPT_A(gt("Cheat Codes:"), gt("Manage"));
		if (menu_window_mark(&menu))
			printf("%s%s\n", con_align(gt("Cover Image:"), 18), 
				imageNotFound ? gt("< DOWNLOAD >") : gt("[ FOUND ]"));
		if (menu_window_mark(&menu))
			PRINT_OPT_S(gt("Hide Game:"), is_hide_game(header->id) ? gt("Yes") : gt("No"));
		if (menu_window_mark(&menu))
			PRINT_OPT_B(gt("Write Playlog:"), game_cfg->write_playlog);
		DefaultColor();
		menu_window_end(&menu, cols);

		printf_h(gt("Press %s to start game"), (button_names[CFG.button_confirm.num]));
		printf("\n");
		bool need_save = !opt_saved || CFG_is_changed(header->id);
		if (need_save)
			printf_h(gt("Press %s to save options"), (button_names[CFG.button_save.num]));
		else
			printf_h(gt("Press %s to discard options"), (button_names[CFG.button_save.num]));
		printf("\n");
		printf_h(gt("Press %s for global options"), (button_names[CFG.button_other.num]));
		DefaultColor();
		__console_flush(0);

		if (redraw_cover) {
			Gui_DrawCover(header->id);
			redraw_cover = 0;
		}
		
		u32 buttons = Wpad_WaitButtonsRpt();
		int change = 0;

		menu_move_active(&menu, buttons);

		if (buttons & WPAD_BUTTON_LEFT) change = -1;
		if (buttons & WPAD_BUTTON_RIGHT) change = +1;

		if (change) {
			switch (menu.current) {
			case 0:
				printf("\n\n");
				printf_x(gt("Saving Settings... "));
				__console_flush(0);
				if (set_favorite(header->id, change > 0)) {
					printf(gt("OK"));
				} else {
					printf(gt("ERROR"));
					sleep(1);
				}
				__console_flush(0);
				Gui_DrawCover(header->id);
				break;
			case 1:
				CHANGE(game_cfg->language, CFG_LANG_MAX);
				break;
			case 2:
				CHANGE(game_cfg->video, CFG_VIDEO_MAX);
				break;
			case 3:
				CHANGE(game_cfg->video_patch, 2);
				break;
			case 4:
				CHANGE(game_cfg->vidtv, 1);
				break;
			case 5:
				CHANGE(game_cfg->country_patch, 1);
				break;
			case 6:
				CHANGE(game_cfg->fix_002, 1);
				break;
			case 7:
				CHANGE(game_cfg->block_ios_reload, 1);
				break;
			case 8:
				CHANGE(game_cfg->ocarina, 1);
				break;
			case 9:
				CHANGE(game_cfg->hooktype, NUM_HOOK-1);
				break;
			case 10:
				Menu_Cheats(header);
				break;
			case 11:
				printf("\n\n");
				Download_Cover((char*)header->id, change > 0, true);
				Cache_Invalidate();
				Gui_DrawCover(header->id);
				Menu_PrintWait();
				break;
			case 12: // hide game
				printf("\n\n");
				printf_x(gt("Saving Settings... "));
				__console_flush(0);
				if (set_hide_game(header->id, change > 0)) {
					printf(gt("OK"));
				} else {
					printf(gt("ERROR"));
					sleep(1);
				}
				__console_flush(0);
				Gui_DrawCover(header->id);
				break;
			case 13:
				CHANGE(game_cfg->write_playlog, 1);
				break;
			}
		}
		if (buttons & CFG.button_confirm.mask) {
			CFG.confirm_start = 0;
			Menu_Boot();
			break;
		}
		if (buttons & CFG.button_save.mask) {
			bool ret;
			printf("\n\n");
			if (need_save) {
				ret = CFG_save_game_opt(header->id);
				if (ret) {
					printf_x(gt("Options saved for this game."));
				} else printf(gt("Error saving options!")); 
			} else {
				ret = CFG_discard_game_opt(header->id);
				if (ret) printf_x(gt("Options discarded for this game."));
				else printf(gt("Error discarding options!")); 
			}
			sleep(1);
		}
		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_other.mask) { ret_val = 1; break; }
		if (buttons & CFG.button_cancel.mask) break;
	}
	CFG_release_game(game_cfg2);

	return ret_val;
}

void Save_Game_List()
{
	struct discHdr *buffer = NULL;
	u32 cnt, len;
	s32 ret;
	char name[200];
	FILE *f;
	int i;

	// Get Game List
	ret = WBFS_GetCount(&cnt);
	if (ret < 0) return;

	printf_x(gt("Saving gamelist.txt ... "));
	__console_flush(0);

	len = sizeof(struct discHdr) * cnt;
	buffer = (struct discHdr *)memalign(32, len);
	if (!buffer) goto error;
	memset(buffer, 0, len);
	ret = WBFS_GetHeaders(buffer, cnt, sizeof(struct discHdr));
	if (ret < 0) goto error;

	snprintf(D_S(name), "%s/%s", USBLOADER_PATH, "gamelist.txt");
	f = fopen(name, "wb");
	if (!f) goto error;
	fprintf(f, "# CFG USB Loader game list %s\n", CFG_VERSION);
	for (i=0; i<gameCnt; i++) {
		fprintf(f, "# %.6s %s\n", gameList[i].id, gameList[i].title);
		fprintf(f, "%.6s = %s\n", gameList[i].id, get_title(&gameList[i]));
	}
	fclose(f);
	SAFE_FREE(buffer);
	printf("OK");
	return;

	error:
	SAFE_FREE(buffer);
	printf(gt("ERROR"));
}

int Menu_Global_Options()
{
	int rows, cols, win_size = 11;
	CON_GetMetrics(&cols, &rows);
	if (strcmp(LAST_CFG_PATH, USBLOADER_PATH)) win_size += 2;
	if ((win_size = rows-win_size) < 3) win_size = 3;

	if (CFG.disable_options) return 0;

	struct discHdr *header = NULL;
	int redraw_cover = 0;

	struct Menu menu;
	menu_init(&menu, 9);

	for (;;) {

		menu.line_count = 0;

		if (gameCnt) {
			header = &gameList[gameSelected];
		} else {
			header = NULL;
		}

		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Global Options"));
		printf(":\n\n");
		DefaultColor();
		menu_window_begin(&menu, win_size, 9);
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Main Menu"));
		if (menu_window_mark(&menu))
			printf("%s%2d/%-2d< %s > (%d)\n", con_align(gt("Profile:"),8),
				CFG.current_profile + 1, CFG.num_profiles,
				CFG.profile_names[CFG.current_profile],
				CFG.num_favorite_game);
		if (menu_window_mark(&menu))
			printf("%s%2d/%2d < %s >\n", con_align(gt("Theme:"),7),
				cur_theme + 1, num_theme, *CFG.theme ? CFG.theme : gt("none"));
		if (menu_window_mark(&menu))
			printf("%s< %s >\n", con_align(gt("Partition:"),13), CFG.partition);
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Download All Missing Covers"));
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Update WiiTDB Game Database")); // download database - lustar
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Update titles.txt"));
		if (menu_window_mark(&menu))
			printf("<%s>\n", gt("Check For Updates"));
		DefaultColor();
		menu_window_end(&menu, cols);
		
		printf_h(gt("Press %s for game options"), (button_names[CFG.button_other.num]));
		printf("\n");
		printf_h(gt("Press %s to save global settings"), (button_names[CFG.button_save.num]));
		printf("\n\n");
		Print_SYS_Info();
		DefaultColor();
		__console_flush(0);

		if (redraw_cover) {
			if (header) Gui_DrawCover(header->id);
			redraw_cover = 0;
		}
		
		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move(&menu, buttons);

		int change = 0;
		if (buttons & WPAD_BUTTON_LEFT) change = -1;
		if (buttons & WPAD_BUTTON_RIGHT) change = +1;
		if (buttons & CFG.button_confirm.mask) change = +1;

		if (change) {
			switch (menu.current) {
			case 0:
				Menu_Views();
				return 0;
			case 1:
				CHANGE(CFG.current_profile, CFG.num_profiles-1);
				// refresh favorites list
				Switch_Favorites(enable_favorite);
				redraw_cover = 1;
				break;
			case 2:
				CFG_switch_theme(cur_theme + change);
				redraw_cover = 1;
				Cache_Invalidate();
				break;
			case 3:
				Menu_Partition(true);
				return 0;
			case 4:
				Download_All_Covers(change > 0);
				Cache_Invalidate();
				if (header) Gui_DrawCover(header->id);
				Menu_PrintWait();
				break;
			case 5:
				Download_XML();
				break;
			case 6:
				Download_Titles();
				break;
			case 7:
				Online_Update();
				break;
			}
		}
		// HOME button
		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_save.mask) {
			int ret;
			printf("\n");
			printf_x(gt("Saving Settings... "));
			printf("\n");
			__console_flush(0);
			FgColor(CFG.color_inactive);
			ret = CFG_Save_Global_Settings();
			DefaultColor();
			if (ret) {
				printf_(gt("OK"));
				printf("\n");
				Save_Game_List();
			} else {
				printf_(gt("ERROR"));
			}
			printf("\n");
			//sleep(2);
			Menu_PrintWait();
		}
		if (buttons & WPAD_BUTTON_PLUS) {
			printf("\n");
			mem_stat();
			Menu_PrintWait();
		}
		if (buttons & CFG.button_other.mask) return 1;
		if (buttons & CFG.button_cancel.mask) break;
	}
	return 0;
}

void Menu_Options()
{
	int ret = 1;
	while(ret)
	{
		if (gameCnt) {
			ret = Menu_Game_Options();
		}
		if (ret) {
			ret = Menu_Global_Options();
		}
	}
}

bool go_gui = false;
extern int action_alpha;

void DoAction(int action)
{
	if (action & CFG_BTN_REMAP) return;
	switch(action) {
		case CFG_BTN_NOTHING:
			break;
		case CFG_BTN_OPTIONS: 
			if (!CFG.disable_options) Menu_Options();
			break;
		case CFG_BTN_GUI:
			if (go_gui) {
				action_string[0] = 0;
				action_alpha=0;
			}
			if (CFG.gui) go_gui = !go_gui;
			break;
		case CFG_BTN_REBOOT:
			Con_Clear();
			Restart();
			break;
		case CFG_BTN_EXIT:
			Con_Clear();
			printf("\n");
			printf_("Exiting...");
			__console_flush(0);
			Sys_Exit();
			break;
		case CFG_BTN_SCREENSHOT:
			__console_flush(0);
			Make_ScreenShot();
			CFG.home = CFG_HOME_EXIT;
			break;
		case CFG_BTN_MAIN_MENU: 
			Menu_Views();
			break;
		case CFG_BTN_GLOBAL_OPS:
			if (!CFG.disable_options) Menu_Global_Options();
			break;
		case CFG_BTN_PROFILE: 
			if (CFG.current_profile == CFG.num_profiles-1)
				CFG.current_profile = 0;
			else
				CFG.current_profile++;
			Switch_Favorites(enable_favorite);
			
			sprintf(action_string, gt("Profile: %s"), CFG.profile_names[CFG.current_profile]);
			
			break;
		case CFG_BTN_FAVORITES:
			{
				extern void reset_sort_default();
				reset_sort_default();
				Switch_Favorites(!enable_favorite);
			}
			break;
		case CFG_BTN_BOOT_GAME:
			Menu_Boot(0);
			break;
		case CFG_BTN_BOOT_DISC:
			Menu_Boot(1);
			break;
		case CFG_BTN_THEME:
			CFG_switch_theme(cur_theme + 1);
			if (gameCnt) Gui_DrawCover(gameList[gameSelected].id);//redraw_cover = 1;
			Cache_Invalidate();
			
			sprintf(action_string, gt("Theme: %s"), theme_list[cur_theme]);
			if (go_gui) action_alpha = 0xFF;
			
			break;
		case CFG_BTN_UNLOCK:
			if (CFG.admin_lock) Menu_Unlock();
			break;
		case CFG_BTN_HBC:
			Con_Clear();
			printf("\n");
			printf_("HBC...");
			__console_flush(0);
			Sys_HBC();
			break;
		case CFG_BTN_SORT:
			if (sort_desc) {
				sort_desc = 0;
				if (sort_index == sortCnt - 1)
					sort_index = 0;
				else
					sort_index = sort_index + 1;
				sortList(sortTypes[sort_index].sortAsc);
			} else {
				sort_desc = 1;
				sortList(sortTypes[sort_index].sortDsc);
			}
			if (gameCnt) Gui_DrawCover(gameList[gameSelected].id);//redraw_cover = 1;

			
			sprintf(action_string, gt("Sort: %s-%s"), sortTypes[sort_index].name, (sort_desc) ? "DESC":"ASC");
			
			break;
		case CFG_BTN_FILTER:
			Menu_Filter();
			break;
		default:
			// Priiloader magic words, and channels
			if ((action & 0xFF) < 'a') {
				// upper case final letter implies channel
				Con_Clear();
				Sys_Channel(action);
			} else {
				// lower case final letter implies magic word
				Con_Clear();
				*(vu32*)0x8132FFFB = action;
				Restart();
			}
			break;
	}
}

void __Menu_Controls(void)
{
	if (CFG.gui == CFG_GUI_START) {
		go_gui = true;
		goto gui_mode;
	}

	//u32 buttons = Wpad_WaitButtons();
	u32 buttons = Wpad_WaitButtonsRpt();

	/* UP/DOWN buttons */
	if (buttons & WPAD_BUTTON_UP)
		__Menu_MoveList(-1);

	if (buttons & WPAD_BUTTON_DOWN)
		__Menu_MoveList(1);

	/* LEFT/RIGHT buttons */
	if (buttons & WPAD_BUTTON_LEFT) {
		//__Menu_MoveList(-ENTRIES_PER_PAGE);
		if (CFG.cursor_jump) {
			__Menu_MoveList(-CFG.cursor_jump);
		} else {
			__Menu_MoveList((gameSelected-gameStart == 0) ? -ENTRIES_PER_PAGE : -(gameSelected-gameStart));
		}
	}

	if (buttons & WPAD_BUTTON_RIGHT) {
		//__Menu_MoveList(ENTRIES_PER_PAGE);
		if (CFG.cursor_jump) {
			__Menu_MoveList(CFG.cursor_jump);
		} else {
			__Menu_MoveList((gameSelected-gameStart == (ENTRIES_PER_PAGE - 1)) ? ENTRIES_PER_PAGE : ENTRIES_PER_PAGE - (gameSelected-gameStart) - 1);
		}
	}

	check_buttons:


	if (CFG.admin_lock) {
		if (buttons & CFG.button_other.mask) {

			static long long t_start;
			long long t_now;
			unsigned ms_diff = 0;
			bool display_unlock = false;

			Con_Clear();
			t_start = gettime();
			while (!display_unlock && (Wpad_Held(0) & CFG.button_other.mask)) {
				buttons = Wpad_GetButtons();
				VIDEO_WaitVSync();
				t_now = gettime();
				ms_diff = diff_msec(t_start, t_now);
				if (ms_diff > 5000)
					display_unlock = true;
			}
			if (display_unlock)
				Menu_Unlock();
			else
				buttons = buttonmap[MASTER][CFG.button_other.num];
		}
	}

	/* A button */
	//if (buttons & CFG.button_confirm.mask)
	//	Menu_Boot(0);

	int i;
	for (i = 4; i < MAX_BUTTONS; i++) {
			if (buttons & buttonmap[MASTER][i]) 
				DoAction(*(&CFG.button_M + (i - 4)));
	}
		
	//if (buttons & CFG.button_cancel.mask)
	//	DoAction(CFG.button_B);

	///* HOME button */
	//if (buttons & CFG.button_exit.mask) {
	//	DoAction(CFG.button_H);
	//	//Handle_Home(1);
	//}

	///* PLUS (+) button */
	//if (buttons & WPAD_BUTTON_PLUS)
	//	DoAction(CFG.button_P);
	////	Menu_Install();

	///* MINUS (-) button */
	//if (buttons & WPAD_BUTTON_MINUS)
	//	DoAction(CFG.button_M);
	////	Menu_Views();
	////	Menu_Remove();

	//if (buttons & WPAD_BUTTON_2)
	//	DoAction(CFG.button_2);

	//if (buttons & CFG.button_other.mask)
	//	DoAction(CFG.button_1);

	//if (buttons & WPAD_BUTTON_X)
	//	DoAction(CFG.button_X);

	//if (buttons & WPAD_BUTTON_Y)
	//	DoAction(CFG.button_Y);

	//if (buttons & WPAD_BUTTON_Z)
	//	DoAction(CFG.button_Z);

	//if (buttons & WPAD_BUTTON_C)
	//	DoAction(CFG.button_C);

	//if (buttons & WPAD_BUTTON_L)
	//	DoAction(CFG.button_L);

	//if (buttons & WPAD_BUTTON_R)
	//	DoAction(CFG.button_R);

	//// button 2 - switch favorites
	//if (buttons & CFG.button_save.mask) {
	//	extern void reset_sort_default();
	//	reset_sort_default();
	//	Switch_Favorites(!enable_favorite);
	//}


	//if (CFG.gui) {
	//	if (CFG.buttons == CFG_BTN_OPTIONS_1) {
	//		if (buttons & CFG.button_cancel.mask) go_gui = true;
	//	} else if (CFG.buttons == CFG_BTN_OPTIONS_B) {
	//		if (buttons & CFG.button_other.mask) go_gui = true;
	//	}
	//}
	//if (!CFG.disable_options) {
	//	if (CFG.buttons == CFG_BTN_OPTIONS_1) {
	//		if (buttons & CFG.button_other.mask) Menu_Options();
	//	} else if (CFG.buttons == CFG_BTN_OPTIONS_B) {
	//		if (buttons & CFG.button_cancel.mask) Menu_Options();
	//	} else { 
	//		/* ONE (1) button */
	//		if (buttons & CFG.button_other.mask) {
	//			//Menu_Device();
	//			Menu_Options();
	//		}
	//	}
	//}
	
	if (go_gui) {
		gui_mode:;
		int prev_sel = gameSelected;
		CFG.gui = 1; // disable auto start
		buttons = Gui_Mode();
		if (prev_sel != gameSelected) {
			// List scrolling
			__Menu_ScrollStartList();
		}
		// if only returning to con mode, clear button status
		/*if (CFG.buttons == CFG_BTN_OPTIONS_1) {
			if (buttons & CFG.button_cancel.mask) buttons = 0;
		} else if (CFG.buttons == CFG_BTN_OPTIONS_B) {
			if (buttons & CFG.button_other.mask) buttons = 0;
		}
		*/
		// if action started from gui, process it then return to gui
		//if (buttons) {
		//	go_gui = true;
		goto check_buttons;
		//}
	}
}

void Menu_Partition(bool must_select)
{
	int i;
	s32 ret = 0;
	int pnum = WBFS_GetPartitionCount();

	struct Menu menu;
	char active[256];
	menu_init(&menu, pnum);
	menu_init_active(&menu, active, 256);

loop:
	menu_begin(&menu);
	/* Clear console */
	Con_Clear();

	FgColor(CFG.color_header);
	printf_x(gt("Select a partition"));
	printf(":\n\n");
	DefaultColor();

	printf_("P# Name\n");
	printf_("-----------------------------\n");
	//       P#1  ram1ro
	//       P#2  sda1ro
	//       P#3  sda2ro

	char partname[32];
	for (i = 0; i < pnum; i++) {
		memset(partname, 0, 32);
		WBFS_GetPartitionName(i, (char *) &partname);
		MENU_MARK();
		printf("%d  %s\n", i, partname);
	}
	printf("\n");
	printf_h(gt("Press %s button to select."), (button_names[CFG.button_confirm.num]));
	printf("\n");
	printf_h(gt("Press %s button to go back."), (button_names[CFG.button_cancel.num]));
	printf("\n");

	u32 buttons = Wpad_WaitButtonsCommon();

	menu_move(&menu, buttons);

	// B button
	if (buttons & CFG.button_cancel.mask) {
		if (must_select) {
			if (WBFS_Selected()) return;
			printf("\n");
			printf_(gt("No partition selected!"));
			printf("\n");
			sleep(2);
		} else {
			return;
		}
	}

	// A button
	if (buttons & CFG.button_confirm.mask) {
		i = menu.current;

		__console_flush(0);
		ret = WBFS_OpenPart(i, CFG.partition);
		if (ret == 0) {
			if (must_select) {
				// called from global options
				__Menu_GetEntries();
			}
			return;
		}
	}

	goto loop;
}

u8 BCA_Data[64] ATTRIBUTE_ALIGN(32);

void Menu_DumpBCA(u8 *id)
{
	int ret;
	char fname[100];
	memset(BCA_Data, 0, 64);
	printf_("\n");
	printf_(gt("Reading BCA..."));
	printf("\n\n");
	ret = WDVD_Read_Disc_BCA(BCA_Data);
	hex_dump3(BCA_Data, 64);
	printf_("\n");
	if (ret) {
		printf_(gt("ERROR reading BCA!"));
		printf("\n\n");
		goto out;
	}
	// save
	snprintf(D_S(fname), "%s/%.6s.bca", USBLOADER_PATH, (char*)id);
	if (!Menu_Confirm(gt("save"))) return;
	printf("\n");
	printf_(gt("Writing: %s"), fname);
	printf("\n\n");
	FILE *f = fopen(fname, "wb");
	if (!f) {
		printf_(gt("ERROR writing BCA!"));
		printf("\n\n");
		goto out;
	}
	fwrite(BCA_Data, 64, 1, f);
	fclose(f);
	out:
	Menu_PrintWait();
}

extern s32 __WBFS_ReadDVD(void *fp, u32 lba, u32 len, void *iobuf);
extern int block_used(u8 *used,u32 i,u32 wblk_sz);

void Menu_Boot()
{
	struct discHdr *header;
	bool gc = false;

	header = &gameList[gameSelected];
	
	s32 ret;
	struct Game_CFG_2 *game_cfg = NULL;

	/* Clear console */
	if (!CFG.direct_launch) {
		Con_Clear();
	}
	FgColor(CFG.color_header);
	printf_x(gt("Start this game?"));
	printf("\n\n");
	DefaultColor();
	
	game_cfg = CFG_find_game(header->id);

	// Get game size
	gc = header->magic == GC_MAGIC;
	bool do_skip = !CFG.confirm_start;
/*
	SoundInfo snd;
	u8 banner_title[84];
	memset(banner_title, 0, 84);
	memset(&snd, 0, sizeof(snd));
	WBFS_Banner(header->id, &snd, banner_title, !do_skip, CFG_read_active_game_setting(header->id).write_playlog);
*/
	if (do_skip) {
		goto skip_confirm;
	}

	printf("\n");

	/* Show game info */
	printf_("%s\n", get_title(header));
	printf_("(%.6s)\n\n", header->id);

	__Menu_ShowGameInfo(true, header->id); /* load game info from XML */
	printf("\n");

	//Does DL warning apply to launching discs too? Not sure
	printf_h(gt("Press %s button to continue."), (button_names[CFG.button_confirm.num]));
	printf("\n");
	printf_h(gt("Press %s button to go back."), (button_names[CFG.button_cancel.num]));
	if (!gc) {
		printf("\n");
		printf_h(gt("Press %s button for options."), (button_names[CFG.button_other.num]));
	}
	printf("\n\n");
	__console_flush(0);

	// play banner sound
/*
	if (snd.dsp_data) {
		SND_PauseVoice(0, 1); // pause mp3
		int fmt = (snd.channels == 2) ? VOICE_STEREO_16BIT : VOICE_MONO_16BIT;
		SND_SetVoice(1, fmt, snd.rate, 0,
			snd.dsp_data, snd.size,
			255,255, //volume,volume,
			NULL); //DataTransferCallback
	}
*/
	/* Wait for user answer */
	u32 buttons;
	for (;;) {
		buttons = Wpad_WaitButtons();
		if (buttons & CFG.button_confirm.mask) break;
		if (buttons & CFG.button_cancel.mask) break;
		if (!gc && (buttons & CFG.button_other.mask)) break;
		if (buttons & CFG.button_exit.mask) break;
	}
/*
	// stop banner sound, resume mp3
	if (snd.dsp_data) {
		SND_StopVoice(1);
		SAFE_FREE(snd.dsp_data);
		if (buttons & CFG.button_confirm.mask) {
			SND_ChangeVolumeVoice(0, 0, 0);
		}
		SND_PauseVoice(0, 0);
	}
*/
	if (buttons & CFG.button_cancel.mask) goto close;
	if (buttons & CFG.button_exit.mask) {
		Handle_Home(0);
		return;
	}
	if (!gc && (buttons & CFG.button_other.mask)) {
		Menu_Boot_Options(header);
		return;
	}
	// A button: continue to boot

	skip_confirm:

	if (game_cfg) {
		CFG.game = game_cfg->curr;
	}

	if (CFG.game.write_playlog && set_playrec(header->id, (u8 *) header->title) < 0) { // banner_title) < 0) {
		printf_(gt("Error storing playlog file.\nStart from the Wii Menu to fix."));
		printf("\n");
		printf_h(gt("Press %s button to exit."), (button_names[CFG.button_exit.num]));
		printf("\n");
		if (!Menu_Confirm(0)) return;
	}

	WBFS_OpenDisc(header->id, header->game_idx);

	printf("\n");
	printf_x(gt("Booting Wii game, please wait..."));
	printf("\n\n");
	
	// load stuff before ios reloads & services close
	ocarina_load_code(header->id);
	load_wip_patches(header->id);

	// Close the wode stuff
	WBFS_Close();
	use_dvdx = 0;
	
	Disc_Init();
	ret = Disc_Wait();
	if (ret < 0) {
		printf("Cannot mount newly selected image: %d\n", ret);
	}
	Disc_Open();

	// stop services (music, gui)
	Services_Close();

	setPlayStat(header->id); //I'd rather do this after the check, but now you unmount fat before that ;)
	
	Fat_UnmountAll();

	if (gc) {
		WII_Initialize();
		ret = WII_LaunchTitle(0x0000000100000100ULL);
	} else {
		switch(CFG.game.language)
				{
						// 0 = CFG_LANG_CONSOLE
						case 0: configbytes[0] = 0xCD; break; 
						case 1: configbytes[0] = 0x00; break; 
						case 2: configbytes[0] = 0x01; break; 
						case 3: configbytes[0] = 0x02; break; 
						case 4: configbytes[0] = 0x03; break; 
						case 5: configbytes[0] = 0x04; break; 
						case 6: configbytes[0] = 0x05; break; 
						case 7: configbytes[0] = 0x06; break; 
						case 8: configbytes[0] = 0x07; break; 
						case 9: configbytes[0] = 0x08; break; 
						case 10: configbytes[0] = 0x09; break;
				}

		/* Boot Wii disc */
		ret = Disc_WiiBoot();
	}
	printf_(gt("Returned! (ret = %d)"), ret);
	printf("\n");

	printf("\n");
	printf_(gt("Press any button to exit..."));
	printf("\n");

	/* Wait for button */
	Wpad_WaitButtonsCommon();
	exit(0);
close:
	WDVD_StopMotor();
	header = &gameList[gameSelected];
	Gui_DrawCover(header->id);
	
	// Reopen the wode
	WBFS_Init();
	
	return;
}

void Direct_Launch()
{
	int i;

	if (!CFG.direct_launch) return;

	// enable console in case there is some input
	// required when loading game, depends on options, like:
	// confirm ocarina, alt_dol=disk (asks for dol)
	// but this can be moved to those points and removed from here
	// so that console is shown only if needed...
	if (CFG.intro) {
		Gui_Console_Enable();
	}

	// confirm_direct_start
	//CFG.confirm_start = 0;

	//sleep(1);
	Con_Clear();
	printf(gt("Configurable Loader %s"), CFG_VERSION);
	printf("\n\n");
	//sleep(1);
	for (i=0; i<gameCnt; i++) {
		if (strncmp(CFG.launch_discid, (char*)gameList[i].id, 6) == 0) {
			gameSelected = i;
			Menu_Boot(0);
			goto out;
		}
	}
	Gui_Console_Enable();
	printf(gt("Auto-start game: %.6s not found!"), CFG.launch_discid);
	printf("\n");
	printf(gt("Press any button..."));
	printf("\n");
	Wpad_WaitButtons();
	out:
	CFG.direct_launch = 0;
}

void Menu_Loop(void)
{
	// enable the console if starting with console mode
	if (CFG.gui != CFG_GUI_START) {
		if (!(CFG.direct_launch && !CFG.intro)) {
			Gui_Console_Enable();
		}
	}

	// Direct Launch?
	Direct_Launch();

	// Start Music
	Music_Start();

	// Clear console
	// (so that it doesn't show when switching back from gui)
	Con_Clear();
	__console_scroll = 0;

	// Init Favorites
	Switch_Favorites(CFG.start_favorites);

	// Start GUI
	if (CFG.gui == CFG_GUI_START) goto skip_list;

	/* Menu loop */
	for (;;) {
		/* Clear console */
		Con_Clear();

		/* Show gamelist */
		__Menu_ShowList();

		/* Show cover */
		__Menu_ShowCover();

		//memstat();

		skip_list:
		/* Controls */
		__Menu_Controls();
	}
}



// menu support routines

void menu_init(struct Menu *m, int num_opt)
{
	memset(m, 0, sizeof(*m));
	m->num_opt = num_opt;
}

void menu_begin(struct Menu *m)
{
	m->line_count = 0;
}

void menu_init_active(struct Menu *m, char *active, int active_size)
{
	m->active = active;
	m->active_size = active_size;
	if (!active) return;
	memset(active, 1, active_size);
}

char m_active(struct Menu *m, int i)
{
	if (m->active == NULL) return 1;
	if (i >= m->active_size) return 0;
	if (i < 0) return 0;
	return m->active[i];
}

void menu_jump_active(struct Menu *m)
{
	int i;
	if (m->current >= m->num_opt) m->current = m->num_opt - 1;
	if (m->current < 0) m->current = 0;
	if (m_active(m, m->current)) return;
	// move to first m->active
	for (i=0; i < m->num_opt; i++) {
		if (m_active(m, i)) {
			m->current = i;
			break;
		}
	}
	if (i == m->num_opt) m->current = 0;
}

void menu_move_cap(struct Menu *m)
{
	if (m->current >= m->num_opt) m->current = m->num_opt-1;
	if (m->current < 0) m->current = 0;
}

void menu_move_wrap(struct Menu *m)
{
	if (m->current >= m->num_opt) m->current = 0;
	if (m->current < 0) m->current = m->num_opt-1;
	if (m->current < 0) m->current = 0;
}

void menu_move_adir(struct Menu *m, int dir)
{
	int i, n;
	menu_move_cap(m);
	n = m->current;
	for (i=0; i<m->num_opt; i++) {
		m->current += dir;
		menu_move_wrap(m);
		if (m_active(m, m->current)) break;
	}
}

// only move on active lines
void menu_move_active(struct Menu *m, int buttons)
{
	if (buttons & WPAD_BUTTON_UP) {
		menu_move_adir(m, -1);
	}
	if (buttons & WPAD_BUTTON_DOWN) {
		menu_move_adir(m, 1);
	}
}

// simple, move on all lines
void menu_move(struct Menu *m, int buttons)
{
	int dir = 0;
	menu_move_cap(m);
	if (buttons & WPAD_BUTTON_UP) dir = -1;
	if (buttons & WPAD_BUTTON_DOWN) dir = 1;
	m->current += dir;
	menu_move_wrap(m);
}

void menu_print_mark(struct Menu *m)
{
	//if (m->active[m->line_count] && m->current == m->line_count) {
	if (m->current == m->line_count) {
		BgColor(CFG.color_selected_bg);
		FgColor(CFG.color_selected_fg);
		Con_ClearLine();
	} else if (m_active(m, m->line_count)) {
		DefaultColor();
	} else {
		DefaultColor();
		FgColor(CFG.color_inactive);
	}
	char *xx;
	//if (m->active[m->line_count] && m->current == m->line_count) {
	if (m->current == m->line_count) {
		xx = CFG.cursor;
	} else {
		xx = CFG.cursor_space;
	}
	printf(" %s ", xx);
}

int menu_mark(struct Menu *m)
{
	menu_print_mark(m);
	return m->line_count++;
}


// size is number of visible lines on screen
// num_items is number of items in the list
void menu_window_begin(struct Menu *m, int size, int num_items)
{
	if (num_items > size + 1)
		m->window_size = size;
	else
		m->window_size = size + 1;
	m->window_items = num_items;
	m->window_begin = m->line_count;
	// adjust window_pos so that selected line is inside the window
	// current_pos = position of selected line
	//   (relative offset from window_begin)
	int current_pos = m->current - m->window_begin;
	if (current_pos >= m->window_pos + m->window_size) {
		if (current_pos > num_items) {
			m->window_pos = num_items - m->window_size + 1;
		} else {
			m->window_pos = current_pos - m->window_size + 1;
		}
	} else if (current_pos < m->window_pos) {
		m->window_pos = current_pos;
	}
	if (m->window_pos < 0) m->window_pos = 0;
	// print page continuation marker
	// only if needed
	if (m->window_size < m->window_items) {
		if (m->window_pos > 0) {
			printf(" %s +\n", CFG.cursor_space);
		} else {
			printf("\n");
		}
	}
}

bool menu_window_mark(struct Menu *m)
{
	int pos = m->line_count - m->window_begin;
	if (pos < m->window_pos) {
		m->line_count++;
		return false;
	}

	if (pos >= m->window_pos + m->window_size) {
		m->line_count++;
		return false;
	}

	menu_mark(m);
	return true;
}

void menu_window_end(struct Menu *m, int cols)
{
	int pos = m->line_count - m->window_begin;
	char str[20] = "";
	int len;
	printf(" %s ", CFG.cursor_space);
	if (pos > m->window_pos + m->window_size) {
		printf("+");
	} else {
		printf(" ");
	}
	if (m->window_size < m->window_items)
		sprintf(str, "%s: %d/%d", gt("page"), 
			    (m->current - m->window_begin) / m->window_size + 1,
				(m->window_items-1) / m->window_size + 1);
	len = strlen(str);
	printf("%*.*s", cols-6-len, cols-6-len, str);
	printf("\n");
	// debug
	//printf("c:%d s:%d b:%d i:%d p:%d\n", m->current, m->window_size,
	//		m->window_begin, m->window_items, m->window_pos);
}


// indented printf
// will indent also each \n in string
void printf_a(const char *fmt, va_list argp)
{
	char strbuf[512];
	char *s, *n;
	int ret;
	ret = vsnprintf(strbuf, sizeof(strbuf), fmt, argp);
	if (ret >= sizeof(strbuf)) {
		// buffer too small, just print directly
		vprintf(fmt, argp);
	} else {
		// append space also to each \n
		s = strbuf;
		do {
			n = strchr(s, '\n');
			if (n) *n = 0; // terminate new line
			printf("%s", s);
			if (n) {
				printf("\n");
				s = n + 1;
				// only indent if there's more text to print
				// don't indent if \n is at the end of string
				if (*s == 0) break;
				printf("%s", CFG.menu_plus_s);
			}
		} while (n);
	}
}

void printf_(const char *fmt, ...)
{
	va_list argp;
	printf("%s", CFG.menu_plus_s);
	va_start(argp, fmt);
	printf_a(fmt, argp);
	va_end(argp);
}

void printf_x(const char *fmt, ...)
{
	va_list argp;
	printf("%s", CFG.menu_plus);
	va_start(argp, fmt);
	//vprintf(fmt, argp);
	printf_a(fmt, argp);
	va_end(argp);
}

void printf_h(const char *fmt, ...)
{
	va_list argp;
	DefaultColor();
	FgColor(CFG.color_help);
	printf("%s", CFG.menu_plus_s);
	va_start(argp, fmt);
	//vprintf(fmt, argp);
	printf_a(fmt, argp);
	va_end(argp);
	DefaultColor();
}

int Menu_PrintWait()
{
	printf_h(gt("Press any button to continue..."));
	printf("\n");
	// clear button states
	WPAD_Flush(WPAD_CHAN_ALL);
	return Wpad_WaitButtonsCommon();
}

bool Menu_Confirm(const char *msg)
{
	if (msg) {
		printf_h(gt("Press %s button to %s."), (button_names[CFG.button_confirm.num]), msg);
	} else {
		printf_h(gt("Press %s button to continue."), (button_names[CFG.button_confirm.num]));
	}
	printf("\n");
	printf_h(gt("Press %s button to go back."), (button_names[CFG.button_cancel.num]));
	printf("\n");
	DefaultColor();
	WPAD_Flush(WPAD_CHAN_ALL);
	for (;;) {
		u32 buttons = Wpad_WaitButtonsCommon();
		if (buttons & CFG.button_confirm.mask) return true;
		if (buttons & CFG.button_cancel.mask) return false;
	}
}

/*
Maybe:

Ocarina (cheats): < Off >

>> Compatibility  < Edit >
   Favorite:      < Yes >
   Hide Game:     < No >
   Cheat Codes:   < Manage >
   Cover Image:   < Download >
   < Start Game >
   < Remove Game >
   < Install Game >
   < Global Options >

*/

