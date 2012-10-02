#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>
#include "tp3.h"
#include "hw.h"

#define LOOP 100000000

int lock = 0;

struct ctx_s {	
	void* ctx_esp;
	void* ctx_ebp;
	unsigned ctx_magic;
	func_t* ctx_f;
	void* ctx_arg;
	state_e ctx_state;
	char* ctx_stack; /* adresse la plus basse de la pile */
	unsigned ctx_size;
	struct ctx_s* ctx_next;
};

struct ctx_s* current_ctx = (struct ctx_s *) 0;
struct ctx_s* ring_head = (struct ctx_s *) 0;
struct ctx_s* return_ctx;

int init_ctx(struct ctx_s *ctx, int stack_size, func_t f, void *args){
	ctx->ctx_stack = (char*) malloc(stack_size);
	if ( ctx->ctx_stack == NULL) return 1;
	ctx->ctx_state = CTX_RDY;
	ctx->ctx_size = stack_size;
	ctx->ctx_f = f;
	ctx->ctx_arg = args;
	ctx->ctx_esp = &(ctx->ctx_stack[stack_size-16]);
	ctx->ctx_ebp = ctx->ctx_esp;
	ctx->ctx_magic = CTX_MAGIC;
	ctx->ctx_next = ctx;
	return 0;
}

int create_ctx(int size, func_t f, void * args){
	struct ctx_s* new_ctx = (struct ctx_s*) malloc(sizeof(struct ctx_s));
	assert(new_ctx);
	irq_disable();
	if(init_ctx(new_ctx, size, f, args)){ /* error */ return 1; }
			
	if(!ring_head){
		ring_head = new_ctx;
		new_ctx->ctx_next = new_ctx;
	}
	else {
		new_ctx->ctx_next = ring_head->ctx_next;
		ring_head->ctx_next = new_ctx;
	}
	irq_enable();
	return 0;
}

void start_current_ctx(){
	current_ctx->ctx_state = CTX_EXQ;
	(current_ctx->ctx_f)(current_ctx->ctx_arg);
	current_ctx->ctx_state = CTX_END;
	yield();
}

void run(){
	int i;
	setup_irq(TIMER_IRQ, &yield);
	start_hw();
	yield();
}

struct ctx_s *previous_ctx(struct ctx_s *ctx){
	struct ctx_s *result = ring_head;
	while(result->ctx_next != ctx){
		result = result->ctx_next;
		if (result == ring_head) break;
	}
	return result;
}

void switch_to_ctx(struct ctx_s *new_ctx){
	struct ctx_s *ctx = new_ctx;
	assert(ctx->ctx_magic == CTX_MAGIC);
	lock = 1;
	while(ctx->ctx_state == CTX_END){
		printf("Finished context encountered\n");
		if(ctx == ctx->ctx_next){
			/* return to main */
			printf("Return to main\n");
			__asm__ ("movl %0, %%esp\n" ::"r"(return_ctx->ctx_esp));
			__asm__ ("movl %0, %%ebp\n" ::"r"(return_ctx->ctx_ebp));
			return;
		}
		else {
			struct ctx_s *next = ctx->ctx_next;
			struct ctx_s* previous = previous_ctx(ctx);
			previous->ctx_next = next;
			/* here I should free the pointers but it's not working, God knows why */
			free(ctx->ctx_stack);
			free(ctx);
			
			ctx = next;	
		}
	}
	
	if(!current_ctx){
		return_ctx = (struct ctx_s*)malloc(sizeof(struct ctx_s));
		printf("Save return context\n");
		__asm__ ("movl %%esp, %0\n" :"=r"(return_ctx->ctx_esp));
		__asm__ ("movl %%ebp, %0\n" :"=r"(return_ctx->ctx_ebp));
	}
	else{
		__asm__ ("movl %%esp, %0\n" :"=r"(current_ctx->ctx_esp));
		__asm__ ("movl %%ebp, %0\n" :"=r"(current_ctx->ctx_ebp)); 
	}
	
	current_ctx = ctx;
	__asm__ ("movl %0, %%esp\n" ::"r"(current_ctx->ctx_esp));
	__asm__ ("movl %0, %%ebp\n" ::"r"(current_ctx->ctx_ebp));
	lock =0;
	if(current_ctx->ctx_state == CTX_RDY){
		start_current_ctx();
	}	 
}

void yield(){
	if(lock) return;
	if(!current_ctx){
		assert(ring_head);
		switch_to_ctx(ring_head);
	}
	else{
		switch_to_ctx(current_ctx->ctx_next);
	}
}

void f_ping(void *args) {
	int i;
	printf("A\n") ;
	for(i=0; i < LOOP ; i++);
	printf("B\n") ; 
	for(i=0; i < LOOP ; i++);
	printf("C\n") ; 
}


void f_pong(void *args) {
	int i=0;
	printf("1\n") ;
	for(i=0; i < LOOP ; i++);
	printf("2\n") ; 
	for(i=0; i < LOOP ; i++);
	printf("3\n") ; 
	for(i=0; i < LOOP ; i++);
	printf("4\n") ; 
	for(i=0; i < LOOP ; i++);
	printf("5\n") ; 
	for(i=0; i < LOOP ; i++);
	printf("6\n") ; 
}

int main(int argc, char*argv[]){
	create_ctx(16384, f_ping, NULL);
	create_ctx(16384, f_pong, NULL);
	run();
	printf("done\n");
	exit(EXIT_SUCCESS);
}