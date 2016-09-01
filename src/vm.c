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

static scm_closure_t *vm_make_closure( scm_value_t args,
                                       scm_value_t body,
                                       environment_t *env )
{
	scm_closure_t *ret = calloc( 1, sizeof( scm_closure_t ));

	ret->definition  = body;
	ret->args        = args;
	ret->is_compiled = false;

	return ret;
}

static bool is_special_form( scm_value_t value ){
	bool ret = false;

	if ( is_run_type( value )){
		unsigned type = get_run_type( value );

		ret = type == RUN_TYPE_LAMBDA
		   || type == RUN_TYPE_DEFINE
		   || type == RUN_TYPE_DEFINE_SYNTAX
		   || type == RUN_TYPE_SET;
	}

	return ret;
}

static bool is_valid_lambda( scm_pair_t *pair ){
	return (is_pair( pair->car ) || is_null( pair->car ))
		&& is_pair( pair->cdr );
}

static void vm_handle_sform( vm_t *vm, scm_value_t form, scm_value_t expr ){
	unsigned type = get_run_type( form );

	switch ( type ){
		case RUN_TYPE_LAMBDA:
			{
				if ( !is_pair( expr )){
					puts( "errrrorr! in lambda expansion" );
				}

				scm_pair_t *pair = get_pair( expr );

				if ( is_valid_lambda( pair )){
					scm_value_t args = pair->car;
					scm_value_t body = pair->cdr;
					scm_closure_t *tmp = vm_make_closure( args, body, vm->env );

					vm_stack_push( vm, tag_closure( tmp ));
					vm_call_return( vm );

				} else {
					puts( "an error! TODO: better error reporting" );
				}
			}

			break;

		case RUN_TYPE_DEFINE:
			if ( !is_pair( expr )){
				puts( "error in define!" );
			}

			scm_pair_t *pair = get_pair( expr );

			vm_stack_push( vm, vm_func_intern_set( ));

			if ( is_symbol( pair->car )){
				vm_stack_push( vm, pair->car );
				vm->ptr = pair->cdr;

			} else if ( is_pair( pair->car )){
				scm_pair_t *temp = get_pair( pair->car );
				scm_closure_t *clsr =
					vm_make_closure( temp->cdr, pair->cdr, vm->env );

				vm_stack_push( vm, temp->car );
				vm_stack_push( vm, tag_closure( clsr ));

				vm->ptr = SCM_TYPE_NULL;
			}

			break;

		default:
			printf( "unknown type in special form evaluation\n" );
			break;
	}
}

static inline void vm_step_interpreter( vm_t *vm ){
	if ( is_pair( vm->ptr )){
		scm_pair_t *pair = get_pair( vm->ptr );

		vm->ptr = pair->cdr;

		if ( is_pair( pair->car )){
			printf( "    doing a call, currently have %u args\n", vm->argnum );
			vm_call_eval( vm, pair->car );

		} else if ( is_symbol( pair->car )){
			printf( "    doing symbol lookup for '%s'... ", get_symbol( pair->car ));

			env_node_t *foo = env_find( vm->env, pair->car );

			if ( foo ){
				printf( "found, " );
				debug_print( foo->value );
				printf( "\n" );

				if ( is_special_form( foo->value )){
					vm_handle_sform( vm, foo->value, pair->cdr );

				} else {
					vm_stack_push( vm, foo->value );
				}

			} else {
				printf( "not found\n" );
				vm_stack_push( vm, pair->car );
			}

		} else {
			vm_stack_push( vm, pair->car );
		}

	} else if ( is_null( vm->ptr )){
		vm_call_apply( vm );

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

static environment_t *vm_r7rs_environment( void ){
	static environment_t *ret = NULL;

	if ( !ret ){
		ret = env_create( NULL );
	}

	return ret;
}

/*
 * This function assumes that the VM has stack pointers that are all zero
 */
scm_value_t vm_evaluate_expr( vm_t *vm, scm_value_t expr ){
	vm->ptr = expr;
	vm->running = true;
	vm->closure = root_closure;
	vm->closure->definition = expr;
	vm->argnum = 0;
	vm->env = vm_r7rs_environment( );

	vm_run( vm );

	return vm->stack[0];
}

#include <nscheme/symbols.h>
#include <string.h>

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

	// TODO: find some place to put environment init stuff
	scm_value_t foo  = tag_symbol( store_symbol( strdup( "+" )));
	scm_value_t clsr = tag_closure( meh );
	env_set( ret->env, ENV_TYPE_DATA, foo, clsr );

	foo = tag_symbol( store_symbol( strdup( "lambda" )));
	env_set( ret->env, ENV_TYPE_INTERNAL, foo, tag_run_type( RUN_TYPE_LAMBDA ));

	foo = tag_symbol( store_symbol( strdup( "define" )));
	env_set( ret->env, ENV_TYPE_INTERNAL, foo, tag_run_type( RUN_TYPE_DEFINE ));

	foo = tag_symbol( store_symbol( strdup( "define-syntax" )));
	env_set( ret->env, ENV_TYPE_INTERNAL, foo, tag_run_type( RUN_TYPE_DEFINE_SYNTAX ));

	foo = tag_symbol( store_symbol( strdup( "set!" )));
	env_set( ret->env, ENV_TYPE_INTERNAL, foo, tag_run_type( RUN_TYPE_SET ));

	return ret;
}
