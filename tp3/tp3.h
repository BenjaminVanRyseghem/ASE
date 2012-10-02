#define CTX_MAGIC 0xBABE

typedef void (func_t) (void*);
typedef enum {CTX_RDY, CTX_EXQ, CTX_END} state_e;


int create_ctx(int size, func_t f, void * args);
void yield();
void run();
void irq_disable();
void irq_enable();