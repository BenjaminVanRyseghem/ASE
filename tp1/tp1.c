#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>

#define CTX_MAGIC 0xBABE


typedef int (func_t) (int);
struct ctx_s {
	void* ctx_esp;
	void* ctx_ebp;
	unsigned ctx_magic;
};

jmp_buf buffer; /* for experiment */
struct ctx_s *pctx;

/* movl src, dest */
/*asm("movl %esp, %0"
		: "=r" (i)		lvalues affectés
		: "r"    )		lvalues lues

) */

void displayCurrentInfo(){
	int i = 0;
	__asm__ ("movl %%esp, %0\n" :"=r"(i));
	printf("%%esp = %d\n", i);
	__asm__ ("movl %%ebp, %0\n" :"=r"(i));
	printf("%%ebp = %d\n", i);
}

int try(struct ctx_s *pctx, func_t *f, int arg){
	__asm__ ("movl %%esp, %0\n" :"=r"(pctx->ctx_esp));
	__asm__ ("movl %%ebp, %0\n" :"=r"(pctx->ctx_ebp));
	pctx->ctx_magic = CTX_MAGIC;
	
	return (*f)(arg);
}

int throw(struct ctx_s *pctx, int val){
	static int ret = 0;
	ret = val;
	
	assert(pctx->ctx_magic == CTX_MAGIC);
	
	__asm__ ("movl %0, %%esp\n" ::"r"(pctx->ctx_esp));
	__asm__ ("movl %0, %%ebp\n" ::"r"(pctx->ctx_ebp));

	return ret;
}

int mul(int depth){
	int i;
	switch (scanf("%d",&i)){
		case EOF 	: 	return 1;
		case 0		:	return mul(depth+1);
		case 1		:	if (i) 	{ 
							if (i==9) return 1;
							return i * mul(depth+1); 	} 
						else 	{ 
							/* longjmp(buffer, ~0); */
							throw (pctx, 0);		}
	}
	return 0;		
}

int main(){
	int product;
	displayCurrentInfo();
	/*if (!setjmp(buffer)){
		product = mul(0);
	}
	else { product = 0; } */
	pctx = (struct ctx_s*) malloc(sizeof(struct ctx_s));
	product = try(pctx, &mul, 0);
	
	printf("product = %d\n", product);
	return 0;
}