#ifndef _NSCHEME_VM_OPS_H
#define _NSCHEME_VM_OPS_H 1

#include <nscheme/vm.h>
#include <stdio.h>

static inline void vm_stack_push( vm_t *vm, scm_value_t value ){
	vm->stack[vm->sp++] = value;
	vm->argnum++;
}

static inline scm_value_t vm_stack_pop( vm_t *vm ){
	vm->argnum--;
	return vm->stack[--vm->sp];
}

// this routine will always be called from an interpreting context,
// a compiled closure will call a different procedure
//
// TODO: insert routine compiled closures will call, for reference
static inline void vm_call_eval( vm_t *vm, scm_value_t ptr ){
	vm_callframe_t *frame = vm->calls + vm->callp++;

	frame->closure = vm->closure;
	frame->sp      = vm->sp;
	frame->argnum  = vm->argnum;
	frame->ptr     = vm->ptr;
	frame->env     = vm->env;

	vm->argnum = 0;
	vm->ptr    = ptr;
}

static inline void vm_call_return( vm_t *vm ){
	if ( vm->callp > 0 ){
		vm_callframe_t *frame = vm->calls + --vm->callp;

		vm->sp      = frame->sp + 1;
		vm->closure = frame->closure;
		vm->argnum  = frame->argnum + 1;

		if ( vm->closure->is_compiled ){
			vm->ip = frame->ip;

		} else {
			vm->ptr = frame->ptr;
			vm->env = frame->env;
		}

	} else {
		vm->running = false;
		vm->sp = 0;
	}
}

static inline void vm_call_apply( vm_t *vm ){
	scm_value_t func = vm->stack[vm->sp - vm->argnum];

	if ( is_closure( func )){
		scm_closure_t *clsr = get_closure( func );
		printf( "    applying closure: %p\n", clsr );
		printf( "      - is %s\n",
			((char *[]){"interpreted", "compiled"})[clsr->is_compiled]);

		vm->closure = clsr;

		if ( clsr->is_compiled ){
			vm->ip = 0;

		} else {
			puts( "    doing things to evaluate interpreted lambda" );
			vm->env = env_create( vm->env );
			vm->ptr = clsr->definition;
			vm->sp -= vm->argnum;
			vm->argnum = 0;
		}

	} else {
		printf( "    dunno how to apply " );
		debug_print( func );
		printf( "\n" );
		vm->running = false;
	}
}

bool vm_op_return( vm_t *vm, unsigned arg );

bool vm_op_add( vm_t *vm, unsigned arg );
bool vm_op_sub( vm_t *vm, unsigned arg );
bool vm_op_mul( vm_t *vm, unsigned arg );
bool vm_op_div( vm_t *vm, unsigned arg );

bool vm_op_jump( vm_t *vm, unsigned arg );
bool vm_op_lessthan( vm_t *vm, unsigned arg );
bool vm_op_equal( vm_t *vm, unsigned arg );
bool vm_op_greaterthan( vm_t *vm, unsigned arg );

#endif
