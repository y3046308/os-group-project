#include "opt-A3.h"
#include <addrspace.h>
#if OPT_A3

typedef enum _frame_state{
//	FREE, DIRTY, FIXED, CLEAN,
	FREE, USED,
} frame_state;

struct entry {
	paddr_t pa; // physical address
	size_t size;	// size
	frame_state state;
	unsigned int fnum; // frame number
	unsigned int pagenum;
};

int coremap_size;
struct entry **coremap;	// table of pages

void init_coremap(void);
paddr_t getppages(unsigned long npages);
void free_pages(paddr_t paddr);
#endif
