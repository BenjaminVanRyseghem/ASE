#include <stdio.h>
#include <stdlib.h>

#include "include/hardware.h"
#include "swap.h"

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

struct vm_mapp_s {
	unsigned vm_ppage : 8;
	unsigned vm_mapped : 1;
};

static struct pm_mapp_s pm_mapping[PM_PAGES];
static struct vm_mapp_s vm_mapping[VM_PAGES];

static unsigned next_ppage = 1;
struct tlb_entry_s global_tlb;


void mmuhandler() {
	// on recupere : vaddr, vpage
	unsigned vaddr, vpage, ppage;
	
	printf("mmu handler !!\n");
	
	vaddr = _in(MMU_FAULT_ADDR);
	
	printf("tentative d’acces illegal a l’adresse %d\n", vaddr);
	
	ppage = ppage_of_vaddr(current_process,vaddr);
	
	if (ppage == -1) {
		printf("sa fait de la merde !!!");
	}
	
	vpage = (vaddr >> 12) & 0xFF;
	
	// address pas dans tlb mais mapper
	if (vm_mapping[vpage].vm_mapped) { 
		_out(TLB_ADD_ENTRY, /*(*/vpage/*, vm_mapping[vpage].vm_ppage)*/);
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

int ppage_of_vaddr(int process, unsigned vaddr) {
	
	printf("ppage_of_vaddr !!\n");
	
	// vaddr -> valide ?
	if (vaddr > END_VMEM || vaddr < BEGIN_VMEM)	return -1;
		
	// calculer le vpage -> 12 bits de poids fort
	int vpage  = (vaddr >> 12) & 0xFF;
	
	// vpage -> valide ?
	if (vpage > N/2 || vpage < 0) return -1;
	
	struct tlb_entry_s* tlb_entries;
	unsigned entries = _in(TLB_ENTRIES);
	tlb_entries = (struct tlb_entry_s*)(&entries);
	
	int i;
	for (i=0; i<TLB_SIZE; i++) {
		if (tlb_entries[i].tlb_vpage == vpage) {
			return tlb_entries[i].tlb_ppage;
		}
	}
	
	return -1;
}

