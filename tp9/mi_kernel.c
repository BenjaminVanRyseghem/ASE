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
	//_out(MMU_PROCESS,1);
	printf("main avant init hardware\n");
	init_hardware("etc/hardware.ini");
	printf("main apres init hardware\n");
	IRQVECTOR[SYSCALL_SWTCH_0] = switch_to_process0; 
	printf("plip\n");
	IRQVECTOR[SYSCALL_SWTCH_1] = switch_to_process1; 

	printf("before mask\n");
	_mask(0x0001);
	printf("main avant init\n");
	init();
}