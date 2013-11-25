#if OPT_A3
#include "opt-A3.h"
#include <elf.h>
//#include <seg_table.h>

struct segment{
	vaddr_t addr;	
	//int flag;	// read-only for text-segment
	//int offset;	// page offset
	//int page_num; // number of pages
	struct segtable *t;
};
#endif
