#include <nscheme/vm.h>
#include <stdlib.h>
#include <stdio.h>

extern void debug_print( scm_value_t value );

static inline void vm_stack_push( vm_t *vm, scm_value_t value ){
	vm->stack[vm->sp++] = value;
	vm->argnum++;
}

static inline void vm_call_eval( vm_t *vm, scm_value_t ptr ){
	vm_callframe_t *frame = vm->calls + vm->callp++;

	frame->closure = vm->closure;
	frame->sp      = vm->sp;
	frame->argnum  = vm->argnum;
	frame->ptr     = vm->ptr;

	vm->argnum = 0;
	vm->ptr    = ptr;
}

static inline void vm_call_return( vm_t *vm ){
	vm_callframe_t *frame = vm->calls + --vm->callp;

	vm->sp      = frame->sp + 1;
	vm->closure = frame->closure;
	vm->argnum  = frame->argnum + 1;
	vm->ptr     = frame->ptr;
}

static inline void vm_call_apply( vm_t *vm ){
	scm_value_t func = vm->stack[vm->sp - vm->argnum];

	if ( is_symbol( func )){
		printf( "    returning from call, function: %s\n",
				get_symbol( func ));
	} else {
		printf( "    dunno what type this thing is...\n" );
	}
}

static inline void vm_step_compiled( vm_t *vm ){
	vm_op_t *code = vm->closure->code + vm->ip;

	vm->ip += code->func( vm, code->arg );
}

static inline void vm_step_interpreter( vm_t *vm ){
	if ( is_pair( vm->ptr )){
		printf( "    got pair: " );
		debug_print( vm->ptr );
		printf( "\n" );

		scm_pair_t *pair = get_pair( vm->ptr );

		vm->ptr = pair->cdr;

		if ( is_pair( pair->car )){
			printf( "    doing a call, currently have %u args\n", vm->argnum );
			vm_call_eval( vm, pair->car );

		} else if ( is_symbol( pair->car )){
			printf( "    doing symbol lookup for %s\n", get_symbol( pair->car ));
			vm_stack_push( vm, pair->car );

		} else {
			vm_stack_push( vm, pair->car );
		}

	} else if ( is_null( vm->ptr )){
		vm_call_apply( vm );

		if ( vm->callp > 0 ){
			vm_call_return( vm );

		} else {
			puts( "    end of interpretation reached, thank you for flying nscheme" );
			puts( "    stack dump:" );

			for ( ; vm->sp; vm->sp-- ){
				printf( "      %u: ", vm->sp - 1 );
				debug_print( vm->stack[vm->sp - 1] );
				printf( "\n" );
			}

			vm->running = false;
		}

	} else {
		// todo: error
		vm->running = false;
		vm->stack[vm->sp] = vm->ptr;
		//puts( "wut" );
	}
}

void vm_run( vm_t *vm ){
	//void (*stepfuncs[2])(vm_t *) = { step_vm_interpreter, step_vm_compiled };

	while ( vm->running ){
		if ( vm->closure->is_compiled ){
			vm_step_compiled( vm );
		} else {
			vm_step_interpreter( vm );
		}

		// TODO: do some benchmarks to see if indexing into an array of function
		//       pointers is faster than branching, once things are working
		//stepfuncs[vm->closure->is_compiled]( vm );
	}
}

/*
 * This function assumes that the VM is in a freshly initialized state, ie.
 * nothing is currently running, all the stack pointers are zero, etc,
 * with the exception of the environment 
 *
 * Bad things will happen if this isn't true.
 */
scm_value_t vm_evaluate_expr( vm_t *vm, scm_value_t expr ){
	vm->ptr = expr;
	vm->running = true;
	vm->closure->is_compiled = false;
	vm->closure->definition = expr;
	vm->argnum = 0;

	vm_run( vm );

	return vm->stack[0];
}

vm_t *vm_init( void ){
	vm_t *ret = calloc( 1, sizeof( vm_t ));
	scm_closure_t *root_closure = calloc( 1, sizeof( scm_closure_t ));

	ret->stack_size = 0x1000;
	ret->calls_size = 0x1000;
	ret->stack = calloc( 1, sizeof( scm_value_t[ret->stack_size] ));
	ret->calls = calloc( 1, sizeof( vm_callframe_t[ret->calls_size] ));
	ret->closure = root_closure;

	return ret;
}
