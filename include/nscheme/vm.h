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

struct vm;

typedef bool (*vm_func)( struct vm *vm, unsigned arg );

typedef struct vm_op {
	vm_func func;
	unsigned long arg;
} vm_op_t;

typedef struct scm_closure {
	// array of variable references closed at compile time
	scm_value_t *closures;

	// array of variable names corresponding to entries in `closures`
	scm_value_t *varnames;

	// the original definition of the function, used for debugging, printing,
	// and recompilation in the future
	scm_value_t definition;

	// true if the closure has been compiled to threaded code,
	// false otherwise.
	bool is_compiled;

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
		};
	};

	vm_op_t code[];
} scm_closure_t;

typedef struct vm_frame {
	scm_closure_t *closure;

	unsigned sp;
	unsigned argnum;

	union {
		// similar to above, ip is used when `closure->is_compiled` is true
		unsigned ip;
		// and `ptr` is used when the tree walker is used, to keep track
		// of the next token to evaluate
		scm_value_t ptr;
	};
} vm_callframe_t;

typedef struct vm {
	// current closure
	scm_closure_t *closure;
	scm_value_t *stack;
	vm_callframe_t *calls;

	// data for interpreter
	scm_value_t ptr;
	environment_t *env;

	// data for threaded interpreter
	unsigned ip;
	unsigned sp;
	unsigned callp;

	// general
	unsigned argnum;
	bool running;
	unsigned stack_size;
	unsigned calls_size;
} vm_t;

vm_t *vm_init( void );
void  vm_free( vm_t *vm );
void  vm_run( vm_t *vm );

scm_value_t vm_evaluate_expr( vm_t *vm, scm_value_t expr );

#endif
