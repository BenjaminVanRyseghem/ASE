#ifndef _MATRIX_H_
#define _MATRIX_H_

#define MATRIX_SIZE 175
#define MATRIX_ADD 0
#define MATRIX_MUL 1
#define MATRIX_DFT 0xBABE
#define MATRIX_DFT2 0xCACA

typedef unsigned int matrix[MATRIX_SIZE][MATRIX_SIZE];

extern void matrix_init(matrix *m);
extern void matrix_add(matrix *dest, matrix *m1, matrix *m2);
extern void matrix_mult(matrix *dest, matrix *m1, matrix *m2);
void matrix_default_de_page(matrix *m);
void matrix_default_de_page2(matrix *m);

extern unsigned matrix_checksum(matrix *m);

#endif

