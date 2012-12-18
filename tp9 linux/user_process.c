#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "include/hardware.h"
#include "matrix.h"

extern void *virtual_memory;

//#define MATRIX_OP MATRIX_ADD
#define MATRIX_OP MATRIX_MUL
//#define MATRIX_OP MATRIX_DFT2


void user_process() {
	unsigned short timestamp = (unsigned)time(NULL);
	matrix *matrix1 = (matrix*)virtual_memory;
	matrix *matrix2 = ((matrix*)virtual_memory) + 1;
	matrix *matrix3 = ((matrix*)virtual_memory) + 2;
	
	srand(timestamp);

	printf("[Starting user process]\n");

	/* print some informations */
	printf("matrices size: %dx%d\n", MATRIX_SIZE, MATRIX_SIZE);
	printf("vm used: %5d pages\n", 3 * sizeof(matrix) / PAGE_SIZE);
	printf("pm space: %5d pages\n", 1 << 8);
	printf("vm space: %5d pages\n", 1 << 12);
	/* init matrices */
	printf("initializing matrices\n");
	matrix_init(matrix1);
	printf("init 1 done\n");
	
	matrix_init(matrix2);
	printf("init 2 done\n");
	_int(DFT_PAG);
	_int(RST_DFT_PAG);
#if MATRIX_OP == MATRIX_ADD
	/* add matrices */
	printf("adding VM matrices ");fflush(stdout);
	matrix_add(matrix3, matrix1, matrix2);

#elif MATRIX_OP == MATRIX_MUL
	/* multiply matrices */
	printf("multiplying matrices ");fflush(stdout);
	matrix_mult(matrix3, matrix1, matrix2);

#elif MATRIX_OP == MATRIX_DFT
	printf("Calculating default page matrices ");fflush(stdout);
	matrix_default_de_page(matrix1);

#elif MATRIX_OP == MATRIX_DFT2
	printf("Calculating default page matrices ");fflush(stdout);
	matrix_default_de_page2(matrix1);

#endif
	_int(DFT_PAG);
	printf("timestamp: 0x%04x, ", timestamp);
	printf("operation: %d, ", MATRIX_OP);
	printf("checksum: 0x%04x\n", matrix_checksum(matrix3));
}

