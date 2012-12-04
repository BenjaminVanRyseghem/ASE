#include <stdio.h>
#include <stdlib.h>
#include "include/hardware.h"
#include ""

#define PAGE_SIZE 4

#define PM_PAGES 256 // 1<<8
#define PM_SIZE PAGE_SIZE*PM_PAGES
#define BEGIN_PMEM *(int*)&physical_memory
#define END_PMEM  *(int*)&physical_memory + PM_SIZE - 1

#define VM_PAGES 4096 // 1<<12
#define VM_SIZE PAGE_SIZE*VM_PAGES
#define BEGIN_VMEM *(int*)&virtual_memory
#define END_VMEM  *(int*)&virtual_memory + VM_SIZE - 1

struct tlb_entry_s {
	unsigned tlb_dummy : 8; // un chanmp de 8 bit
	unsigned tlb_vpage : 12;
	unsigned tlb_ppage : 8;
	unsigned tlb_xwr : 3;
	unsigned tlb_valid : 1;
};

struct pm_mapp_s {
	unsigned pm_vpage : 12;
	unsigned pm_mapped : 1;
};

static struct pm_mapp_s pm_mapping[PM_PAGES];

struct vm_mapp_s {
	unsigned vm_ppage : 8;
	unsigned vm_mapped : 1;
};

static struct vm_mapp_s vm_mapping[VM_PAGES];

static unsigned next_ppage = 1;

struct tlb_entry_s global_tlb;
int current_process;
static unsigned ppage_mapping;


mmuhandler() {
	printf("tentative d’acces illegal a l’adresse %d\n", _in(MMU_FAULT_ADDR));
	
	//////////////////////////
	// struct tlb_entry_s tlb;
	// tlb.tlb_vpage = vpage;
	// tlb.tlb_ppage = ppage_of_addr(current_ctx, vaddr);
	// 
	// if (tlb.ppage == -1) {
	// 	fprintf(stderr,"segmentation fault");
	// 	return;
	// }
	// 
	// tlb.tlb_xwr = 7;
	// tlb.tlb_valid = 1;
	// 
// _out(TLB_ADD_ENTRY, tlb);
	/////////////////////////////////
	
	/////////////////////////////////////
	// store_to_swap(vp, pp);
	// 	fetch_from_swap(vp, pp);
	// 	vaddr = _in(MMU_FAULT_ADDR);
	// 	vpage = (vaddr >> 12) & 0xFF;
	// 	if virtual_memory <= vaddr <= virtual_mmeory+VM_SIZE+1
	// 	store_to_swap(ppage_mapping, 1);
	// 	fetch_from_swap(vp, 1);
	// 	
	// 	struct tlb_entry_s tlb_suppr;
	// 	tlb_suppr.ppage = 1;
	// 	
	// 	_out(TLB_DEL_ENTRY, tlb_suppr/* <- tlb_page*/);
	// 	
	// 	struct tlb_entry_s tlb_add;
	// 	tlb_add.tbl_vpage = vpage;
	// 	tlb_add.acces = 7;
	// 	tlb_add.valid = 1;
	// 	
	// 	_out(TLB_ADD_ENTRY, tlb_add);
	// 	
	// 	ppage_mapping = vpage;
	//////////////////////////////////////
	
	// on recupere : vaddr, vpage
	unsigned vaddr, vpage, ppage;
	// address pas dans tlb mais mapper
	
	
	if (vm_mapping[vpage].vm_mapped) { 
		_out(TLB_ADD_ENTRY, (vpage, vm_mapping[vpage].vm_ppage));
	} else {
		// assurer le mapping de vm avec pm
		if(pm_mapping[ppage].pm_mapped) {
			store_to_swap(pm_mapping[next_ppage].pm_vpage, next_ppage);
			pm_mapping[next_ppage].pm_mapped = 0;
			vm_mapping[pm_mapping[next_ppage].pm_vpage].vm_mapped = 0;
			_out(TLB_DEL_ENTRY,next_ppage);
		}
		
		fetch_from_swap(vpage, next_ppage);
		vm_mapping[vpage].vm_ppage = next_ppage;
		vm_mapping[vpage].vm_mapped = 1;
		pm_mapping[next_ppage].pm_vpage = vpage;
		pm_mapping[next_ppage].pm_mapped = 1;
		
		 struct tlb_entry_s tlb;
		 		tlb.tlb_vpage = vpage;	
		 		tlb.tlb_ppage = next_ppage;
		 		tlb.tlb_xwr = 7;
		 		tlb.tlb_valid = 1;
		
		_out(TLB_ADD_ENTRY, *(int*)&tlb);
		
		next_ppage = (next_ppage + 1) % PM_PAGES; 
	}
}



// Q 3.2
static int ppage_of_vaddr(int process, unsigned vaddr) {
	// vaddr -> valide ?
	if (vaddr > END_VMEM || vaddr < BEGIN_VMEM)
		return -1;
		
	// calculer le vpage -> 12 bits de poids fort
	int vpage  = (vaddr >> 12) & 0xFF;
	
	// vpage -> valide ?
	if (vpage > PM_PAGES/*N/2 ??*/ || vpage < 0) return -1;
	
	//process
	if (current_process) global_tlb.tlb_ppage = global_tlb.tlb_vpage+1;
	else				 global_tlb.tlb_ppage = global_tlb.tlb_vpage/*+N/2*/+1;	 
		
	return 0;
}







int main(int argc, char **argv) {
	char *ptr;
	//... /* init_hardware() */
	
	// on ajoute la fonction mmuhandler au MMU_IRQ;
	IRQVECTOR[MMU_IRQ] = mmuhandler; 
	
	// les signaux d'interuption ne se font qu'au niveau 1;
	_mask(1);
	
	// ecrire c a l'adresse 0
    ptr = (char*)0;
	*ptr = 'c';
}