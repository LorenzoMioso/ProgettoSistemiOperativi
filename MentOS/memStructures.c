struct zone {
    unsigned long free_pages;          // Number of free pages in the zone.
    free_area_t free_area[MAX_ORDER];  // buddy blocks (see next)
    page_t* zone_mem_map;              // pointer to first page descriptor
    uint32_t zone_start_pfn;           // Index of the first page frame
    unsigned long size;                // Total size of zone in pages
    char* name;                        // Name of the zone
};

struct free_area {
    list_head free_list;  // list collecting the first page of each free block.
    int nr_free;          // number of blocks of free pages
};

struct page_t {
    int _count;            // count : -1 = free, else not free
    unsigned int private;  // number of contiguos frame (order)
    struct list_head lru;
    //.. continue
};
