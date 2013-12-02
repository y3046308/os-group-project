#include "opt-A3.h"
#include <types.h>
#include <vnode.h>
#include <synch.h>
#include <pt.h>
#if OPT_A3

#define MAX_SIZE 9 * 1024 * 1024	// maximum swapfile size
/*
struct swap{
	bool occupied;
//	struct pte* entry;
};*/
typedef bool swap;
void swap_initialize(void);
void read_from_swap(paddr_t pfn, int index);
int write_to_swap(void);

int swap_num;	// total number of pages in swapfile
struct vnode* swapfile;
struct lock* swapLock;
swap* swap_partition;
#endif
