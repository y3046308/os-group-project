enum _pstate_t {
    free,
    dirty,
    fixed,
    clean
};
typedef enum _pstate_t pstate_t;


struct page{
    struct addrspace* as;
    vaddr_t vaddr;
    pstate_t state;
};

struct pte{
    paddr_t pfn;
    int valid;
    int dirty;
};
