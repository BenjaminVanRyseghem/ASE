#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>

#define CTX_MAGIC 0xBABE

typedef void (func_t) (void*);
typedef enum {CTX_RDY, CTX_EXQ, CTX_END} state_e;

struct ctx_s {	
	void* ctx_esp;
	void* ctx_ebp;
	unsigned ctx_magic;
	func_t* ctx_f;
	void* ctx_arg;
	state_e ctx_state;
	char* ctx_stack; /* adresse la plus basse de la pile */
	unsigned ctx_size;
};

struct ctx_s* current_ctx = (struct ctx_s*) 0;
struct ctx_s* return_ctx;

int init_ctx(struct ctx_s *ctx, int stack_size, func_t f, void *args){
	ctx->ctx_stack = (char*) malloc(stack_size);
	if ( ctx->ctx_stack == NULL) return 1;
	ctx->ctx_state = CTX_RDY;
	ctx->ctx_size = stack_size;
	ctx->ctx_f = f;
	ctx->ctx_arg = args;
	ctx->ctx_esp = &(ctx->ctx_stack[stack_size-sizeof(int)]);
	ctx->ctx_ebp = ctx->ctx_esp;
	ctx->ctx_magic = CTX_MAGIC;
	return 0;
}

void start_current_ctx(){
	current_ctx->ctx_state = CTX_EXQ;
	(current_ctx->ctx_f)(current_ctx->ctx_arg);
	current_ctx->ctx_state = CTX_END;
	__asm__ ("movl %0, %%esp\n" ::"r"(return_ctx->ctx_esp));
	__asm__ ("movl %0, %%ebp\n" ::"r"(return_ctx->ctx_ebp));
}

void switch_to_ctx(struct ctx_s *ctx){
	assert(ctx->ctx_magic == CTX_MAGIC);
	assert(ctx->ctx_state == CTX_RDY || ctx->ctx_state == CTX_EXQ);
	
	if(current_ctx == 0){
		return_ctx = (struct ctx_s*)malloc(sizeof(struct ctx_s));
		current_ctx = ctx;
		printf("First context called\n");
		__asm__ ("movl %%esp, %0\n" :"=r"(return_ctx->ctx_esp));
		__asm__ ("movl %%ebp, %0\n" :"=r"(return_ctx->ctx_ebp));
		__asm__ ("movl %0, %%esp\n" ::"r"(current_ctx->ctx_esp));
		__asm__ ("movl %0, %%ebp\n" ::"r"(current_ctx->ctx_ebp));
	}
	else{
		__asm__ ("movl %%esp, %0\n" :"=r"(current_ctx->ctx_esp));
		__asm__ ("movl %%ebp, %0\n" :"=r"(current_ctx->ctx_ebp));
		current_ctx = ctx;
		__asm__ ("movl %0, %%esp\n" ::"r"(current_ctx->ctx_esp));
		__asm__ ("movl %0, %%ebp\n" ::"r"(current_ctx->ctx_ebp));
	}
	if(current_ctx->ctx_state == CTX_RDY){
		start_current_ctx();
	}

}

void print_success(){
	printf("A context ended\n");
}
struct ctx_s ctx_ping; 
struct ctx_s ctx_pong;

void f_ping(void *arg);
void f_pong(void *arg);

int main(int argc, char*argv[]){
	if (init_ctx(&ctx_ping, 16384, f_ping, NULL)) exit(EXIT_FAILURE);
	if (init_ctx(&ctx_pong, 16384, f_pong, NULL)) exit(EXIT_FAILURE);
	switch_to_ctx(&ctx_ping);

	print_success();
	
	exit(EXIT_SUCCESS);
}


void f_ping(void *args) {
	printf("A\n") ;
	switch_to_ctx(&ctx_pong); 
	printf("B\n") ; 
	switch_to_ctx(&ctx_pong); 
	printf("C\n") ; 
	switch_to_ctx(&ctx_pong);
}


void f_pong(void *args) {
	printf("1\n") ;
	switch_to_ctx(&ctx_ping); 
	printf("2\n") ; 
	switch_to_ctx(&ctx_ping);
}