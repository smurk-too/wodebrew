#ifndef _FSTFILE_H_
#define _FSTFILE_H_

    typedef struct {
        u8 filetype;
        char name_offset[3];
        u32 fileoffset;
        u32 filelen;
    } __attribute__((packed)) FST_ENTRY;

    char *fstfiles(FST_ENTRY *fst, u32 index);
    char *fstfilename(u32 index);
    u32 fstfileoffset(u32 index);

#endif
