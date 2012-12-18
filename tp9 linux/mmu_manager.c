#include <stdio.h>
#include <stdlib.h>

#include "mmuhandler.h"
#include "mi_syscall.h"
#include "swap.h"
#include "include/hardware.h"
	
extern void user_process();
extern int default_de_page;

int main() {
	_out(MMU_PROCESS,1);
	if(init_swap("swap")) {
		fprintf(stderr, "Error in swap initialization\n");
		exit(EXIT_FAILURE);
	}
	if(init_hardware("etc/hardware.ini") == 0) {
		fprintf(stderr, "Error in hardware initialization\n");
		exit(EXIT_FAILURE);
	}
	IRQVECTOR[MMU_IRQ] = mmuhandler;
	IRQVECTOR[DFT_PAG] = print_default_page;
	IRQVECTOR[RST_DFT_PAG] = reset_default_page;

	_mask(0x1001);
	user_process();
	return 0;
}
