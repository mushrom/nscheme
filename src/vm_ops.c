#include <nscheme/vm.h>
#include <nscheme/vm_ops.h>
#include <stdlib.h>

extern void debug_print( scm_value_t value );

static scm_value_t vm_func_return_last( void ){
	static scm_closure_t *ret = NULL;

	if ( !ret ){
		ret = calloc( 1, sizeof( scm_closure_t ) + sizeof( vm_op_t ));

		ret->is_compiled = true;
		ret->code[0].func = vm_op_return_last;
	}

	return tag_closure( ret );
}

static void vm_load_lambda_args( vm_t *vm, unsigned argnum, scm_value_t args ){
	scm_value_t arg = args;
	unsigned i = 1;

	while ( is_pair( arg )){
		scm_pair_t *pair = get_pair( arg );
		scm_value_t sym = pair->car;

		env_set( vm->env, ENV_TYPE_DATA, sym, vm->stack[vm->sp + i++] );

		arg = pair->cdr;
	}

	if ( i != argnum ){
		puts( "    error: not enough or too many arguments to function" );
	}
}

void vm_call_apply( vm_t *vm ){
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
			printf(
				"       - closure already called %u times\n",
				clsr->num_calls );

			unsigned called_args = vm->argnum;

			vm->env = env_create( vm->env );
			vm->ptr = clsr->definition;
			vm->sp -= vm->argnum;
			vm->argnum = 0;
			clsr->num_calls++;

			vm_load_lambda_args( vm, called_args, clsr->args );
			vm_stack_push( vm, vm_func_return_last( ));
		}

	} else {
		printf( "    dunno how to apply " );
		debug_print( func );
		printf( "\n" );
		vm->running = false;
	}
}

bool vm_op_return( vm_t *vm, unsigned arg ){
	vm_call_return( vm );

	return false;
}

bool vm_op_return_last( vm_t *vm, unsigned arg ){
	vm->stack[vm->sp - vm->argnum] = vm_stack_pop( vm );
	vm_call_return( vm );

	return false;
}

bool vm_op_add( vm_t *vm, unsigned arg ){
	scm_value_t sum = 0;

	puts( "got here" );

	for ( unsigned args = vm->argnum - 1; args; args-- ){
		// no untagging/retagging needed because the lower bits
		// of tagged integers are 0b00
		sum += vm_stack_pop( vm );
	}

	// remove the function on stack
	vm_stack_pop( vm );

	// and finally leave the return value
	vm_stack_push( vm, sum );

	return true;
}

bool vm_op_sub( vm_t *vm, unsigned arg );
bool vm_op_mul( vm_t *vm, unsigned arg );
bool vm_op_div( vm_t *vm, unsigned arg );

bool vm_op_jump( vm_t *vm, unsigned arg );
bool vm_op_lessthan( vm_t *vm, unsigned arg );
bool vm_op_equal( vm_t *vm, unsigned arg );
bool vm_op_greaterthan( vm_t *vm, unsigned arg );
