#define PAGE_SIZE 4

#define PM_PAGES 256 // 1<<8
#define PM_SIZE PAGE_SIZE*PM_PAGES
#define BEGIN_PMEM *(int*)&physical_memory
#define END_PMEM  *(int*)&physical_memory + PM_SIZE - 1

#define VM_PAGES 4096 // 1<<12
#define VM_SIZE PAGE_SIZE*VM_PAGES
#define BEGIN_VMEM *(int*)&virtual_memory
#define END_VMEM  *(int*)&virtual_memory + VM_SIZE - 1


char init_swap(const char *path);
char store_to_swap(int vpage, int ppage);
char fetch_from_swap(int vpage, int ppage);