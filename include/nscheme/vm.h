#ifndef _NSCHEME_VM_H
#define _NSCHEME_VM_H 1
#include <nscheme/values.h>
#include <nscheme/env.h>
#include <stdbool.h>

// TODO: implement vm_error function to handle errors properly
//       consider supporting with-exception-handler from the start
/*
#define VM_ASSERT(cond, message)\
	if (!cond) { \
		fprintf(stderr, message); \
		exit(1); \
	}
	*/

enum {
	RUN_MODE_INTERP,
	RUN_MODE_COMPILED,
};

struct vm;

typedef bool (*vm_func)(struct vm *vm, uintptr_t arg);

typedef struct vm_op {
	vm_func func;
	uintptr_t arg;
} vm_op_t;

typedef struct scm_closure {
	// compiled instructions for vm
	vm_op_t *code;

	// array of variable references closed at compile time
	env_node_t **closures;

	// array of variable names corresponding to entries in `closures`
	scm_value_t *varnames;

	// the original definition of the function, used for debugging, printing,
	// and recompilation in the future
	scm_value_t definition;

	// true if the closure has been compiled to threaded code,
	// false otherwise.
	bool compiled;

	// how many times this closure has been called. this determines when
	// the JIT compiler will be called, and at which optimization levels.
	unsigned num_calls;

	union {
		// this struct will be used when `is_compiled` is true
		struct {
			// number of arguments the closure requires when called
			unsigned num_args;
			// the number of ops contained in `code[]`
			unsigned num_ops;
			// the number of closed variables in `closure`,
			unsigned num_closed;
			// number of stack slots required to call this procedure, not
			// including the arguments passed by the caller
			unsigned required_stack;
		};

		// this one is used otherwise
		struct {
			environment_t *env;
			scm_value_t args;
		};
	};
} scm_closure_t;

typedef struct vm_frame {
	scm_closure_t *closure;

	unsigned sp;
	unsigned argnum;
	unsigned runmode;

	union {
		// similar to above, ip is used when `runmode` is true
		unsigned ip;
		// and `ptr` is used when the tree walker is used, to keep track
		// of the next token to evaluate
		struct {
			scm_value_t ptr;
			environment_t *env;
		};
	};
} vm_callframe_t;

typedef struct scm_gc_context {
	// start of the heap
	uint8_t *base;
	// end of the heap
	uint8_t *end;
	// current end of the allocations in the heap
	uint8_t *allocend;

	// keeps track of whether the current marked bit is 1 or 0
	// alternates each GC collection cycle
	int current_mark;
} vm_gc_context_t;

typedef struct vm_handle {
	scm_value_t value;
	bool used;
	// TODO: call, destructor functions
} vm_handle_t;

typedef struct vm_handle_stack {
	vm_handle_t *slots;
	int *avail;
	size_t num_avail;
	size_t max_avail;
} vm_handle_stack_t;

typedef struct vm {
	// data for threaded interpreter
	unsigned ip;
	unsigned sp;
	unsigned callp;
	bool running;

	// current closure
	scm_closure_t *closure;
	scm_value_t *stack;
	vm_callframe_t *calls;

	// data for interpreter
	environment_t *env;
	scm_value_t ptr;

	// general
	unsigned argnum;
	unsigned stack_size;
	unsigned calls_size;
	unsigned runmode;

	vm_gc_context_t gc;
	const char *errormsg;

	// data handles, used to keep explicit references to values when they're
	// not otherwise part of the VM state
	// e.g. for macro generation, external callers, etc
	vm_handle_stack_t handles;
} vm_t;

vm_t *vm_init(void);
void  vm_free(vm_t *vm);
void  vm_run(vm_t *vm);
void  vm_error(vm_t *vm, const char *msg);
void  vm_clear_error(vm_t *vm);

void  *vm_alloc(vm_t *vm, size_t n);
void   gc_init(vm_gc_context_t *gc, size_t initial_size);
void  *gc_alloc(vm_gc_context_t *gc, size_t n);
size_t gc_collect_vm(vm_gc_context_t *gc, vm_t *vm);

void vm_handles_init(vm_handle_stack_t *stack, size_t initial_size);
int  vm_handle_alloc(vm_t *vm);
void vm_handle_free(vm_t *vm, int handle);
bool vm_handle_valid(vm_t *vm, int handle);
scm_value_t vm_handle_get(vm_t *vm, int handle);
void        vm_handle_set(vm_t *vm, int handle, scm_value_t);

scm_value_t vm_evaluate_expr(vm_t *vm, scm_value_t expr);

#endif
