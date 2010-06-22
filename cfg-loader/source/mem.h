#ifndef _MEM_H
#define _MEM_H

#define MAX_MEM_BLK 1024

typedef struct
{
	void  *ptr;
	int   size;
} mem_blk;

typedef struct
{
	int num;
	mem_blk list[MAX_MEM_BLK];
} blk_list;

typedef struct
{
	void *start;
	int size;
	blk_list used_list;
	blk_list free_list;
} heap;

typedef struct
{
	int size;
	int used;
	int free;
	void *highptr; // end of last used block
} heap_stats;

// mem_blk
mem_blk* blk_find_size(blk_list *bl, int size);
mem_blk* blk_find_ptr(blk_list *bl, void *ptr);
mem_blk* blk_find_ptr_end(blk_list *bl, void *ptr);
mem_blk* blk_find_ptr_after(blk_list *bl, void *ptr);
mem_blk* blk_insert(blk_list *bl, mem_blk *b);
void     blk_remove(blk_list *bl, mem_blk *b);
mem_blk* blk_merge_add(blk_list *list, mem_blk *ab);

// heap
void* heap_alloc(heap *h, int size);
int   heap_free(heap *h, void *ptr);
void* heap_realloc(heap *h, void *ptr, int size);
void  heap_init(heap *h, void *ptr, int size);
int   heap_ptr_inside(heap *h, void *ptr);
void  heap_stat(heap *h, heap_stats *s);

// mem
void  mem_init();
void* mem1_alloc(int size);
void* mem1_realloc(void *ptr, int size);
void* mem2_alloc(int size);
void* mem_alloc(int size);
void* mem_realloc(void *ptr, int size);
void  mem_free(void *ptr);
void  mem_stat();
void  mem_stat_str(char * buffer);

// ogc sys
extern void* SYS_GetArena2Lo();
extern void* SYS_GetArena2Hi();
extern void* SYS_AllocArena2MemLo(u32 size,u32 align);
extern void* __SYS_GetIPCBufferLo();
extern void* __SYS_GetIPCBufferHi();

// util
void* LARGE_memalign(size_t align, size_t size);
void* LARGE_alloc(size_t size);
void* LARGE_realloc(void *ptr, size_t size);
void  LARGE_free(void *ptr);
void  memstat2();
void  util_init();
void  util_clear();

#define SAFE_FREE(P) if(P){mem_free(P);P=NULL;}

#endif

