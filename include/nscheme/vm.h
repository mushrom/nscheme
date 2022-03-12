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

typedef struct vm {
	// current closure
	scm_closure_t *closure;
	scm_value_t *stack;
	vm_callframe_t *calls;

	// data for interpreter
	environment_t *env;
	scm_value_t ptr;

	// data for threaded interpreter
	unsigned ip;
	unsigned sp;
	unsigned callp;

	// general
	unsigned argnum;
	bool running;
	unsigned stack_size;
	unsigned calls_size;
	unsigned runmode;

	const char *errormsg;
} vm_t;

vm_t *vm_init(void);
void  vm_free(vm_t *vm);
void  vm_run(vm_t *vm);
void  vm_error(vm_t *vm, const char *msg);
void  vm_clear_error(vm_t *vm);

scm_value_t vm_evaluate_expr(vm_t *vm, scm_value_t expr);

#endif
