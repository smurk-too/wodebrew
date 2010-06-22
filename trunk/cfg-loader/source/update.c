
// by oggzee

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
#include "gettext.h"
#include "xml.h"

////////////////////////////////////////
//
//  Updates
//
////////////////////////////////////////

#define MAX_UP_DESC 5

struct UpdateInfo
{
	char version[32];
	char url[100];
	int size;
	char date[16]; // 2009-12-30
	char desc[MAX_UP_DESC][40];
	int ndesc;
};


#define MAX_UPDATES 8
int num_updates = 0;
struct UpdateInfo updates[MAX_UPDATES];
char meta_xml[1024];

bool parsebuf_line(char *buf, int (*line_func)(char*))
{
	char line[200];
	char *p, *nl;
	int len, ret;

	nl = buf;
	for (;;) {
		p = nl;
		if (p == NULL) break;
		while (*p == '\n') p++;
		if (*p == 0) break;
		nl = strchr(p, '\n');
		if (nl == NULL) {
			len = strlen(p);
		} else {
			len = nl - p;
		}
		if (len >= sizeof(line)) len = sizeof(line) - 1;
		strcopy(line, p, len+1);
		ret = line_func(line);
		if (ret) break;
	}
	return true;
}

int update_set(char *line)
{
	char name[100], val[100];
	struct UpdateInfo *u;
	bool opt = trimsplit(line, name, val, '=', sizeof(name));
	// if space in opt name, then don't consider it an option
	// this is so to allow = in comments 
	if (opt && strchr(name, ' ') != NULL) opt = false;
	if (opt)
	{
		if (stricmp("metaxml", name) == 0) {
			strncat(meta_xml, val, sizeof(meta_xml)-1);
			strncat(meta_xml, "\n", sizeof(meta_xml)-1);
			return 0;
		}
		if (stricmp("release", name) == 0) {
			if (num_updates >= MAX_UPDATES) {
				return 1;
			}
			num_updates++;
			u = &updates[num_updates-1]; 
			u->ndesc = 0;
			u->size = 0;
			*u->url = 0;
			STRCOPY(u->version, val);
			return 0;
		}

		if (num_updates <= 0) return 0;
		u = &updates[num_updates-1]; 
		if (stricmp("size", name) == 0) {
			int n;
			if (sscanf(val, "%d", &n) == 1) {
				if (n > 500000 && n < 5000000) {
					u->size = n;
				}
			}
		} else if (stricmp("date", name) == 0) {
			STRCOPY(u->date, val);
		} else if (stricmp("url", name) == 0) {
			STRCOPY(u->url, val);
		} else if (*u->url) {
			// anything unknown after url is a comment till a new release line
			goto comment;
		} else {
			// ignore unknown options
		}
	}
	else
	{
		comment:
		if (num_updates > 0) {
			u = &updates[num_updates-1]; 
			if (u->ndesc < MAX_UP_DESC) {
				STRCOPY(u->desc[u->ndesc], line);
				u->ndesc++;
			}
		}
	}
	return 0;

}

void parse_updates(char *buf)
{
	num_updates = 0;
	memset(updates, 0, sizeof(updates));
	memset(meta_xml, 0, sizeof(meta_xml));
	parsebuf_line(buf, update_set);
}

void Download_Update(int n, char *app_path, int update_meta)
{
	struct UpdateInfo *u;
	struct block file;
	FILE *f = NULL;
	file.data = NULL;
	u = &updates[n]; 

	printf_(gt("Downloading: %s"), u->version);
	printf("\n");
	printf_("[.");

	file = downloadfile_progress(u->url, 1);
	printf("]\n");
	
	if (file.data == NULL || file.size < 500000) {
		goto err;
	}

	if (u->size && u->size != file.size) {
		printf_(gt("Wrong Size: %d (%d)"), file.size, u->size);
		printf("\n");
		goto err;
	}

	printf_(gt("Complete. Size: %d"), file.size);
	printf("\n");

	printf_(gt("Saving: %s"), app_path);
	printf("\n");
	// backup
	char bak_name[200];
	strcpy(bak_name, app_path);
	strcat(bak_name, ".bak");
	// remove old backup
	remove(bak_name);
	// rename current to backup
	rename(app_path, bak_name);
	// save new
	f = fopen(app_path, "wb");
	if (!f) {
		printf_(gt("Error opening: %s"), app_path);
		printf("\n");
		goto err;
	}
	fwrite(file.data, 1, file.size, f);
	fclose(f);

	if (update_meta && *meta_xml) {
		str_replace(meta_xml, "{VERSION}", u->version, sizeof(meta_xml));
		str_replace(meta_xml, "{DATE}", u->date, sizeof(meta_xml));
		strcpy(strrchr(app_path, '/')+1, "meta.xml");
		printf_(gt("Saving: %s"), app_path);
		printf("\n");
		f = fopen(app_path, "wb");
		if (!f) {
			printf_(gt("Error opening: %s"), app_path);
			printf("\n");
		} else {
			fwrite(meta_xml, 1, strlen(meta_xml), f);
			fclose(f);
		}
	}

	printf_(gt("Done."));
	printf("\n\n");
	printf_(gt("NOTE: loader restart is required\nfor the update to take effect."));
	printf("\n\n");
	printf_(gt("Press any button..."));
	Wpad_WaitButtonsCommon();

	SAFE_FREE(file.data);
	return;

err:
	SAFE_FREE(file.data);
	printf_(gt("Error downloading update..."));
	printf("\n");
	sleep(3);
}

void Menu_Updates()
{
	struct Menu menu;
	struct UpdateInfo *u;
	char app_dir[100];
	char app_path[100];
	struct stat st;
	int cols, rows;
	int max_desc;
	int i;

	CON_GetMetrics(&cols, &rows);
	if (rows < 22 && num_updates > 6) num_updates = 6;
	menu_init(&menu, num_updates);

	STRCOPY(app_dir, APPS_DIR);
	snprintf(D_S(app_path),"%s/%s", app_dir, "boot.dol");
	// check if app_dir/boot.dol exists
	if (stat(app_path, &st) != 0) {
		// not found, use default path
		STRCOPY(app_dir, "sd:/apps/usbloader");
		snprintf(D_S(app_path),"%s/%s", app_dir, "boot.dol");
	}

	for (;;) {
		Con_Clear();
		FgColor(CFG.color_header);
		printf_x(gt("Available Updates"));
		printf(":\n");
		DefaultColor();
		menu.line_count = 0;
		max_desc = 0;
		for (i=0; i<num_updates; i++) {
			MENU_MARK();
			printf("%s\n", updates[i].version);
			if (updates[i].ndesc > max_desc) {
				max_desc = updates[i].ndesc;
			}
		}
		DefaultColor();
		FgColor(CFG.color_header);
		printf("\n");
		printf_x(gt("Release Notes: (short)"));
		printf("\n");
		DefaultColor();
		u = &updates[menu.current]; 
		for (i=0; i<max_desc; i++) {
			if (i < u->ndesc) {
				printf_("%.*s\n",
						cols - strlen(CFG.menu_plus_s) - 1,
						u->desc[i]);
			} else {
				printf("\n");
			}
		}

		// current line:
		// 1+6 +1+1+5 = 14
		// + remaining: 6 = 20

		if (rows > 17) printf("\n");
		if (rows > 18) {
			printf_h(gt("Press %s to download and update"), (button_names[CFG.button_confirm.num]));
			printf("\n");
			if (rows > 20) {
				printf_h(gt("Press %s to update without meta.xml"), (button_names[CFG.button_other.num]));
				printf("\n");
			}
			printf_h(gt("Press %s to return"), (button_names[CFG.button_cancel.num]));
			printf("\n");
		} else {
			printf_h(gt("Press %s to download, %s to return"), (button_names[CFG.button_confirm.num]), (button_names[CFG.button_cancel.num]));
			printf("\n");
		}
		if (rows > 19) printf("\n");
		FgColor(CFG.color_inactive);
		printf_(gt("Current Version: %s"), CFG_VERSION);
		printf("\n");
		printf_(gt("App. Path: %s"), app_dir);
		DefaultColor();
		__console_flush(0);

		u32 buttons = Wpad_WaitButtonsRpt();
		menu_move(&menu, buttons);

		if (buttons & CFG.button_exit.mask) {
			Handle_Home(0);
		}
		if (buttons & CFG.button_cancel.mask) break;
		if (buttons & CFG.button_confirm.mask) {
			printf("\n\n");
			Download_Update(menu.current, app_path, 1);
			break;
		}
		if (buttons & CFG.button_other.mask) {
			printf("\n\n");
			Download_Update(menu.current, app_path, 0);
			break;
		}
	}
	printf("\n");
}

void Online_Update()
{
	char *updates_url = "http://wodebrew.googlecode.com/svn/cfg-loader/trunk/updates.txt";
	struct block file;
	file.data = NULL;

	DefaultColor();
	printf("\n");
	printf_(gt("Checking for updates..."));
	printf("\n");

	if (Init_Net()) {
		file = downloadfile(updates_url);
	}
	if (file.data == NULL || file.size == 0) {
		printf_(gt("Error downloading updates..."));
		printf("\n");
		sleep(3);
		return;
	}

	// null terminate
	char *newbuf;
	newbuf = realloc(file.data, file.size+4);
	if (newbuf) {
		newbuf[file.size] = 0;
		file.data = (void*)newbuf;
	} else {
		goto err;
	}
	// parse
	parse_updates((char*)file.data);
	SAFE_FREE(file.data);

	if (num_updates <= 0) {
		printf_(gt("No updates found."));
		printf("\n");
		goto out;
	}

	//printf("\n");
	//printf_("Available: %d updates.\n", num_updates);
	//sleep(1);

	Menu_Updates();
	return;

err:
	printf_(gt("Error downloading updates..."));
	printf("\n");
out:
	sleep(3);
	SAFE_FREE(file.data);
	return;
}




void Download_Titles()
{
	struct block file;
	file.data = NULL;
	char fname[100];
	char url[100];
	FILE *f;

	DefaultColor();
	printf("\n");
	
	if (!Init_Net()) goto out;

	printf_(gt("Downloading titles.txt ..."));
	printf("\n");
	extern char *get_cc();
	char *cc = get_cc();
	char * dbl = ((strlen(CFG.db_language) == 2) ? VerifyLangCode(CFG.db_language) : ConvertLangTextToCode(CFG.db_language));
	//if (strcmp(cc, "JA") ==0 || strcmp(cc, "KO") ==0) cc = "EN";
	strcpy(url, CFG.titles_url);
	str_replace(url, "{CC}", cc, sizeof(url));
	str_replace(url, "{DBL}", dbl, sizeof(url));
	printf_("%s\n", url);

	file = downloadfile(url);
	if (file.data == NULL || file.size == 0) {
		printf_(gt("Error downloading updates..."));
		printf("\n");
		sleep(3);
		return;
	}

	snprintf(D_S(fname),"%s/%s", USBLOADER_PATH, "titles.txt");
	printf_(gt("Saving: %s"), fname);
	printf("\n");
	f = fopen(fname, "wb");
	if (!f) {
		printf_(gt("Error opening: %s"), fname);
		printf("\n");
	} else {
		fwrite(file.data, 1, file.size, f);
		fclose(f);
		printf_(gt("Done."));
		printf("\n\n");
		printf_(gt("NOTE: loader restart is required\nfor the update to take effect."));
		printf("\n\n");
	}
out:
	printf_(gt("Press any button..."));
	Wpad_WaitButtonsCommon();
	SAFE_FREE(file.data);
	return;
}


