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
unsigned current_vpage = -1;
unsigned current_ppage = -1;

void mmuhandler() {
	// on recupere : vaddr, vpage
	unsigned vaddr, vpage, ppage;
	
	vaddr = _in(MMU_FAULT_ADDR);
	
	ppage = ppage_of_vaddr(current_process,vaddr);
	
	if (ppage == -1) {
		;
	}
	
	vpage = (vaddr >> 12) & 0xFF;

/*
Le handler d’interruptions associé à la MMU est donc appelé quand l’adresse accédée est fautive. Si elle est dans l’espace d’adressage virtuel, comme elle est fautive, elle ne correspond pas à la page qui est en actuellement en mémoire physique. On sauvegarde donc cette page dans le fichier de swap et on supprime le mapping dans la TLB pour cette page. Il s’agit ensuite de charger la page correspondant à l’adresse fautive et de mettre à jour la TLB en conséquence.

	//On sauvegarde donc cette page dans le fichier de swap
	if(current_vpage != -1 && current_ppage != -1){
		store_to_swap(current_vpage, current_ppage);
	}

	//On supprime le mapping dans la TLB pour cette page
	struct tlb_entry_s tlb_request;
 	tlb_request.tlb_vpage = current_vpage;
	_out(TLB_DEL_ENTRY, *(int*)&tlb_request);

	current_ppage = ppage;
	current_vpage = vpage;

	//Charger la page correspondant à l'adresse fautive
	fetch_from_swap(current_vpage, current_ppage);

	//Mettre à jour la TLB
	tlb_request.tlb_vpage = current_vpage;	
	tlb_request.tlb_ppage = 1;
	tlb_request.tlb_xwr = 7;
	tlb_request.tlb_valid = 1;

	_out(TLB_ADD_ENTRY, *(int*)&tlb_request);
*/
	struct tlb_entry_s tlb_request;

	// address pas dans tlb mais mapper
	if (vm_mapping[vpage].vm_mapped) { 
		tlb_request.tlb_vpage = vpage;	
		tlb_request.tlb_ppage = vm_mapping[vpage].vm_ppage;
		tlb_request.tlb_xwr = 7;
		tlb_request.tlb_valid = 1;

		_out(TLB_ADD_ENTRY, *(int*)&tlb_request);

	} else {
		// assurer le mapping de vm avec pm
		if(pm_mapping[ppage].pm_mapped) {
			store_to_swap(pm_mapping[next_ppage].pm_vpage, next_ppage);
			pm_mapping[next_ppage].pm_mapped = 0;
			vm_mapping[pm_mapping[next_ppage].pm_vpage].vm_mapped = 0;
			tlb_request.tlb_vpage = pm_mapping[next_ppage].pm_vpage;
			_out(TLB_DEL_ENTRY, *(int*)&tlb_request);
		}

		if(fetch_from_swap(vpage, next_ppage)){
			printf("fetch not found\n");
		}
		vm_mapping[vpage].vm_ppage = next_ppage;
		vm_mapping[vpage].vm_mapped = 1;
		pm_mapping[next_ppage].pm_vpage = vpage;
		pm_mapping[next_ppage].pm_mapped = 1;
		
	 	tlb_request.tlb_vpage = vpage;	
 		tlb_request.tlb_ppage = next_ppage;
 		tlb_request.tlb_xwr = 7;
 		tlb_request.tlb_valid = 1;
		
		_out(TLB_ADD_ENTRY, *(int*)&tlb_request);
		
		next_ppage = (next_ppage + 1);
		if(next_ppage == PM_PAGES) next_ppage = 1; 

	}

}

int ppage_of_vaddr(int process, unsigned vaddr) {
	
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

