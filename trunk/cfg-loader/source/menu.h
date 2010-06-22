#ifndef _MENU_H_
#define _MENU_H_

#include "disc.h" // discHdr

/* Prototypes */
void Menu_Boot();
int Menu_Boot_Options(struct discHdr *header);
s32 __Menu_GetEntries(void);
void Menu_Loop(void);
void Menu_Options(void);
void Menu_Partition(bool must_select);
void Handle_Home(int disable_screenshot);
void Online_Update();
void Download_Titles();
void Menu_Cheats(struct discHdr *header);
int  Menu_PrintWait();
bool Menu_Confirm(const char *msg);

void __Menu_ShowGameInfo(bool showfullinfo, u8 *id); // Lustar
char *skip_sort_ignore(char *s);

extern struct discHdr *gameList;
extern s32 gameCnt, gameSelected, gameStart;
extern s32 all_gameCnt;

void __Menu_ScrollStartList();

extern struct discHdr *filter_gameList;

struct Menu
{
	int num_opt;
	int current;
	int line_count;
	char *active;
	int active_size;
	int window_size;
	int window_begin;
	int window_items;
	int window_pos;
};

void menu_init(struct Menu *m, int num_opt);
void menu_begin(struct Menu *m);
int  menu_mark(struct Menu *m);
void menu_move(struct Menu *m, int buttons);

void menu_init_active(struct Menu *m, char *active, int active_size);
void menu_jump_active(struct Menu *m);
void menu_move_cap(struct Menu *m);
void menu_move_wrap(struct Menu *m);
void menu_move_adir(struct Menu *m, int dir);
void menu_move_active(struct Menu *m, int buttons);

void menu_window_begin(struct Menu *m, int size, int num_items);
bool menu_window_mark(struct Menu *m);
void menu_window_end(struct Menu *m, int cols);

#define MENU_MARK() menu_mark(&menu)

#endif

