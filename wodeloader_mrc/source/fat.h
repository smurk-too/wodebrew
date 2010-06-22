#ifndef _FAT_H_
#define _FAT_H_

/* Prototypes */
s32 Fat_MountSDHC(void);
s32 Fat_UnmountSDHC(void);
s32 Fat_ReadFile(const char *, void **);
bool Fat_CheckDir(const char *);
bool Fat_CheckFile(const char *);
s32 Fat_SaveFile(const char *, void **,int);

#endif
