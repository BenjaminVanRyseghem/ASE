#include <stdio.h>
#include <stdlib.h>

#include "mmuhandler.h"
#include "mi_syscall.h"
#include "swap.h"
#include "include/hardware.h"
		
void switch_to_process0(void) {
	current_process = 0;
	_out(MMU_CMD, MMU_RESET);
}

void switch_to_process1(void) {
	current_process = 1;
	_out(MMU_CMD, MMU_RESET);
}

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
	IRQVECTOR[SYSCALL_SWTCH_0] = switch_to_process0; 
	IRQVECTOR[SYSCALL_SWTCH_1] = switch_to_process1; 
	

	_mask(0x1001);
	init();
}
