#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>
#include "tp4.h"
#include "hw.h"

#define LOOP 100000000
#define N 100

object_t stack[N];
int next_index = 0;

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
	struct ctx_s* ctx_sem_next;
};

struct sem_s {
	char* sem_name;
	int sem_cpt;
	struct ctx_s* sem_head;
};

struct ctx_s* current_ctx = (struct ctx_s *) 0;
struct ctx_s* ring_head = (struct ctx_s *) 0;
struct ctx_s* return_ctx;
struct sem_s mutex, vide, plein;

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

void switch_to_ctx(struct ctx_s *new_ctx){
	struct ctx_s *ctx = new_ctx;
	assert(ctx->ctx_magic == CTX_MAGIC);
	irq_disable();
	while(ctx->ctx_state == CTX_END || ctx->ctx_state == CTX_STP){
		if(ctx->ctx_state == CTX_END) printf("Finished context encountered\n");
		if(ctx->ctx_state == CTX_STP) printf("Frozen context encountered\n");
		if(ctx == ctx->ctx_next){
			/* return to main */
			free(ctx->ctx_stack);
			free(ctx);
			printf("Return to main\n");
			__asm__ ("movl %0, %%esp\n" ::"r"(return_ctx->ctx_esp));
			__asm__ ("movl %0, %%ebp\n" ::"r"(return_ctx->ctx_ebp));
			return;
		}
		else {
			struct ctx_s *next = ctx->ctx_next;
			if (ctx->ctx_state == CTX_END){
				current_ctx->ctx_next = next;
				free(ctx->ctx_stack);
				free(ctx);
			}
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
	irq_enable();
	if(current_ctx->ctx_state == CTX_RDY){
		start_current_ctx();
	}	 
}

void yield(){
	if(!current_ctx){
		assert(ring_head);
		switch_to_ctx(ring_head);
	}
	else{
		switch_to_ctx(current_ctx->ctx_next);
	}
}


void freeze_current_in(struct sem_s *sem){
	irq_disable();
	current_ctx->ctx_state = CTX_STP;
	current_ctx->ctx_sem_next = sem->sem_head;
	sem->sem_head = current_ctx;
	irq_enable();
	yield();
}

void unfreeze_current_in(struct sem_s *sem){
	irq_disable();
	struct ctx_s* ctx = sem->sem_head;
	ctx->ctx_state = CTX_EXQ;
	sem->sem_head=ctx->ctx_sem_next;
	irq_enable();
/*	switch_to_ctx(ctx); */
}

void sem_init(struct sem_s *sem, unsigned int val, char* name){
	sem->sem_cpt = val;
	sem->sem_head = NULL;
	sem->sem_name = name;
}

void sem_up(struct sem_s *sem){
	irq_disable();
	sem->sem_cpt++;
	if(sem->sem_cpt <= 0){
		printf("UNFREEZE MOTHAFUCKA: %s\n", sem->sem_name);
		unfreeze_current_in(sem);
	}
	irq_enable();
}

void sem_down(struct sem_s *sem){
	irq_disable();
	sem->sem_cpt--;
	if(sem->sem_cpt < 0){
		printf("FREEZE MOTHAFUCKA: %s\n", sem->sem_name);
		freeze_current_in(sem);
	}
	irq_enable();
}

void produce_object(object_t* object){
	object->value=1;
}

void use_object(object_t object){
	object.value = 0;
}

void push_object(object_t object){
	next_index++;
	if(next_index>N){
		printf("FAIL!\n");
	}
}

void pull_object(object_t* object){
	next_index--;
	if(next_index<0){
		printf("FAIL!\n");
	}
}

void producer(){
	object_t object;
	while(1){
		printf("produce: %d/%d\n", next_index, vide.sem_cpt);
		produce_object(&object);
		sem_down(&vide);
		sem_down(&mutex);
		push_object(object);
		sem_up(&mutex);
		sem_up(&plein);
	}
}

void consumer(){
	object_t object;
	while(1){
		printf("consume: %d/%d\n", next_index, plein.sem_cpt);
		sem_down(&plein);
		sem_down(&mutex);
		pull_object(&object);
		sem_up(&mutex);
		sem_up(&vide);
		use_object(object);
	}
}

int main(int argc, char*argv[]){
	sem_init(&mutex, 1, "mutex"); 
	sem_init(&vide, N, "vide"); 
	sem_init(&plein, 0, "plein");

	create_ctx(16384, &producer, NULL);
	create_ctx(16384, &consumer, NULL);
	run();
	printf("done\n");
	exit(EXIT_SUCCESS);
}
