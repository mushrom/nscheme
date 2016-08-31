#include <nscheme/vm.h>
#include <nscheme/vm_ops.h>
#include <nscheme/env.h>
#include <stdlib.h>
#include <stdio.h>

extern void debug_print( scm_value_t value );

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

			//env_node_t *foo = env_find( vm->closure->env, pair->car );
			env_node_t *foo = env_find( vm->env, pair->car );

			if ( foo ){
				printf( "        - found as %p: ", foo );
				debug_print( foo->value );
				printf( "\n" );
				vm_stack_push( vm, foo->value );

			} else {
				printf( "        - not found\n" );
				vm_stack_push( vm, pair->car );
			}

		} else {
			vm_stack_push( vm, pair->car );
		}

	} else if ( is_null( vm->ptr )){
		vm_call_apply( vm );

		// just continue without returning if `vm_call_apply` invoked
		// a compiled procedure
		if ( !vm->closure->is_compiled ){
			vm_call_return( vm );
		}

	} else {
		// todo: error
		vm->running = false;
		vm->stack[vm->sp] = vm->ptr;
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

static scm_closure_t *root_closure = NULL;

/*
 * This function assumes that the VM has stack pointers that are all zero
 */
scm_value_t vm_evaluate_expr( vm_t *vm, scm_value_t expr ){
	vm->ptr = expr;
	vm->running = true;
	vm->closure = root_closure;
	vm->closure->definition = expr;
	vm->argnum = 0;

	vm_run( vm );

	return vm->stack[0];
}

#include <nscheme/symbols.h>
#include <string.h>

static environment_t *vm_r7rs_environment( void ){
	static environment_t *ret = NULL;

	if ( !ret ){
		ret = env_create( NULL );
	}

	return ret;
}

vm_t *vm_init( void ){
	vm_t *ret = calloc( 1, sizeof( vm_t ));
	//scm_closure_t *root_closure = calloc( 1, sizeof( scm_closure_t ));
	root_closure = calloc( 1, sizeof( scm_closure_t ));

	ret->stack_size = 0x1000;
	ret->calls_size = 0x1000;
	ret->stack = calloc( 1, sizeof( scm_value_t[ret->stack_size] ));
	ret->calls = calloc( 1, sizeof( vm_callframe_t[ret->calls_size] ));
	ret->closure = root_closure;
	ret->env = vm_r7rs_environment( );

	scm_closure_t *meh = calloc( 1, sizeof( scm_closure_t ) + sizeof( vm_op_t[2] ));
	meh->code[0].func = vm_op_add;
	meh->code[1].func = vm_op_return;
	meh->is_compiled = true;

	scm_value_t foo  = tag_symbol( store_symbol( strdup( "+" )));
	scm_value_t clsr = tag_closure( meh );
	env_set( ret->env, ENV_TYPE_DATA, foo, clsr );

	return ret;
}
