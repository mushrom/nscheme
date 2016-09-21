#include <nscheme/vm.h>
#include <nscheme/vm_ops.h>
#include <nscheme/parse.h>
#include <nscheme/compiler.h>
#include <nscheme/write.h>

#include <stdlib.h>

static scm_value_t vm_func_return_last( void ){
	static scm_closure_t *ret = NULL;

	if ( !ret ){
		ret = calloc( 1, sizeof( scm_closure_t ));
		ret->code = calloc( 1, sizeof( vm_op_t[1] ));

		ret->compiled = true;
		ret->code[0].func = vm_op_return_last;
	}

	return tag_closure( ret );
}

scm_value_t vm_func_intern_define( void ){
	static scm_closure_t *ret = NULL;

	if ( !ret ){
		ret = calloc( 1, sizeof( scm_closure_t ));
		ret->code = calloc( 1, sizeof( vm_op_t[2] ));

		ret->compiled = true;
		ret->code[0].func = vm_op_intern_define;
		ret->code[1].func = vm_op_return;
	}

	return tag_closure( ret );
}

scm_value_t vm_func_intern_set( void ){
	static scm_closure_t *ret = NULL;

	if ( !ret ){
		ret = calloc( 1, sizeof( scm_closure_t ));
		ret->code = calloc( 1, sizeof( vm_op_t[2] ));

		ret->compiled = true;
		ret->code[0].func = vm_op_intern_set;
		ret->code[1].func = vm_op_return;
	}

	return tag_closure( ret );
}

scm_value_t vm_func_intern_if( void ){
	static scm_closure_t *ret = NULL;

	if ( !ret ){
		ret = calloc( 1, sizeof( scm_closure_t ));
		ret->code = calloc( 1, sizeof( vm_op_t[2] ));

		ret->compiled = true;
		ret->code[0].func = vm_op_intern_if;
		ret->code[1].func = vm_op_return;
	}

	return tag_closure( ret );
}

static void vm_load_lambda_args( vm_t *vm, unsigned argnum, scm_value_t args ){
	scm_value_t arg = args;
	unsigned i = 1;

	while ( is_pair( arg )){
		scm_pair_t *pair = get_pair( arg );
		scm_value_t sym = pair->car;

		env_set( vm->env, sym, vm->stack[vm->sp + i++] );

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
		/*
		printf( "    applying %s closure: %p\n",
			((char *[]){"interpreted", "compiled"})[clsr->compiled],
			clsr );
			*/

		vm->closure = clsr;

		if ( clsr->compiled ){
			vm->runmode = RUN_MODE_COMPILED;
			vm->ip = 0;

		} else {
			unsigned called_args = vm->argnum;

			vm->runmode = RUN_MODE_INTERP;
			vm->env = env_create( clsr->env );
			vm->ptr = clsr->definition;
			vm->sp -= vm->argnum;
			vm->argnum = 0;
			clsr->num_calls++;

			if ( clsr->num_calls == 3 ){
				vm_compile_closure( vm, clsr );
			}

			vm_load_lambda_args( vm, called_args, clsr->args );
			vm_stack_push( vm, vm_func_return_last( ));
		}

	} else if ( func == tag_run_type( RUN_TYPE_SET_PTR )){
		if ( vm->argnum != 2 ){
			puts( "    can't eval with given arguments" );
		}

		scm_value_t ptr = vm_stack_pop( vm );
		vm_stack_pop( vm );
		vm->ptr = ptr;

	} else {
		printf( "    dunno how to apply " );
		write_value( func );
		printf( "\n" );
		vm->running = false;
	}
}

bool vm_op_return( vm_t *vm, uintptr_t arg ){
	vm_call_return( vm );

	return false;
}

bool vm_op_return_last( vm_t *vm, uintptr_t arg ){
	vm->stack[vm->sp - vm->argnum] = vm_stack_pop( vm );
	vm_call_return( vm );

	return false;
}

bool vm_op_jump( vm_t *vm, uintptr_t arg ){
	vm->ip = arg;

	return false;
}

bool vm_op_jump_if_false( vm_t *vm, uintptr_t arg ){
	scm_value_t value = vm_stack_pop( vm );

	if ( value == tag_boolean( false )){
		vm->ip = arg;
		return false;
	}

	return true;
}

bool vm_op_closure_ref( vm_t *vm, uintptr_t arg ){
	env_node_t *var = vm->closure->closures[arg];

	vm_stack_push( vm, var->value );

	return true;
}

bool vm_op_stack_ref( vm_t *vm, uintptr_t arg ){
	// TODO: maybe find another way to do this that doesn't require
	//       the temporary index variable, eg. storing the 'base'
	//       stack index from the call frame in the vm struct
	uintptr_t index   = vm->sp - vm->argnum + arg;
	scm_value_t value = vm->stack[index];

	vm_stack_push( vm, value );

	return true;
}

bool vm_op_push_const( vm_t *vm, uintptr_t arg ){
	vm_stack_push( vm, arg );
	return true;
}

bool vm_op_do_call( vm_t *vm, uintptr_t arg ){
	vm_callframe_t *frame = vm->calls + vm->callp++;

	unsigned start  = vm->sp - vm->argnum + arg;
	unsigned argnum = vm->sp - start;

	frame->closure = vm->closure;
	frame->ip      = vm->ip + 1;
	frame->sp      = start;
	frame->argnum  = vm->argnum - argnum;
	frame->runmode = vm->runmode;

	vm->argnum = argnum;

	vm_call_apply( vm );

	return false;
}

bool vm_op_do_tailcall( vm_t *vm, uintptr_t arg ){
	unsigned newstart  = vm->sp - vm->argnum + arg;
	unsigned newargnum = vm->sp - newstart;
	unsigned start     = vm->sp - vm->argnum;

	for ( unsigned i = 0; i < newargnum; i++ ){
		vm->stack[start + i] = vm->stack[newstart + i];
	}

	vm->sp     = start + newargnum;
	vm->argnum = newargnum;
	vm->ip     = 0;

	vm_call_apply( vm );

	return false;
}

bool vm_op_add( vm_t *vm, uintptr_t arg ){
	scm_value_t sum = 0;

	for ( uintptr_t args = vm->argnum - 1; args; args-- ){
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

bool vm_op_sub( vm_t *vm, uintptr_t arg ){
	scm_value_t sum = vm->stack[vm->sp - vm->argnum + 1];

	for ( uintptr_t args = vm->argnum - 2; args; args-- ){
		sum -= vm_stack_pop( vm );
	}

	vm_stack_pop( vm );
	vm_stack_pop( vm );
	vm_stack_push( vm, sum );

	return true;
}

bool vm_op_mul( vm_t *vm, uintptr_t arg ){
	uintptr_t sum = 1;

	for ( uintptr_t args = vm->argnum - 1; args; args-- ){
		sum *= get_integer( vm_stack_pop( vm ));
	}

	vm_stack_pop( vm );
	vm_stack_push( vm, tag_integer( sum ));

	return true;
}

bool vm_op_div( vm_t *vm, uintptr_t arg ){
	uintptr_t sum = vm->stack[vm->sp - vm->argnum + 1];

	for ( uintptr_t args = vm->argnum - 2; args; args-- ){
		uintptr_t temp = vm_stack_pop( vm );

		if ( temp ){
			sum /= temp;

		} else {
			puts(    "divide by zero! TODO: error" );

		}
	}

	vm_stack_pop( vm );
	vm_stack_pop( vm );
	vm_stack_push( vm, tag_integer( sum ));

	return true;
}

bool vm_op_cons( vm_t *vm, uintptr_t arg ){
	if ( vm->argnum != 3 ){
		puts( "not enough args man" );
		return true;
	}

	scm_value_t cdr = vm_stack_pop( vm );
	scm_value_t car = vm_stack_pop( vm );

	vm_stack_pop( vm );
	vm_stack_push( vm, construct_pair( car, cdr ));

	return true;
}

bool vm_op_car( vm_t *vm, uintptr_t arg ){
	if ( vm->argnum != 2 ){
		puts( "cargs" );
		return true;
	}

	scm_value_t value = vm_stack_pop( vm );
	vm_stack_pop( vm );

	if ( !is_pair( value )){
		puts( "car does not abide, man" );
		return true;
	}

	scm_pair_t *pair = get_pair( value );
	vm_stack_push( vm, pair->car );

	return true;
}

bool vm_op_cdr( vm_t *vm, uintptr_t arg ){
	if ( vm->argnum != 2 ){
		puts( "cdargs" );
		return true;
	}

	scm_value_t value = vm_stack_pop( vm );
	vm_stack_pop( vm );

	if ( !is_pair( value )){
		puts( "cdr does not abide, man" );
		return true;
	}

	scm_pair_t *pair = get_pair( value );
	vm_stack_push( vm, pair->cdr );

	return true;
}

bool vm_op_lessthan( vm_t *vm, uintptr_t arg ){
	if ( vm->argnum != 3 ){
		puts( "not enough args man" );
		return true;
	}

	scm_value_t op1 = vm_stack_pop( vm );
	scm_value_t op2 = vm_stack_pop( vm );

	vm_stack_pop( vm );
	vm_stack_push( vm, tag_boolean( op2 < op1 ));

	return true;
}

bool vm_op_equal( vm_t *vm, uintptr_t arg ){
	if ( vm->argnum != 3 ){
		puts( "not enough args man" );
		return true;
	}

	scm_value_t op1 = vm_stack_pop( vm );
	scm_value_t op2 = vm_stack_pop( vm );

	vm_stack_pop( vm );
	vm_stack_push( vm, tag_boolean( op2 == op1 ));

	return true;
}

bool vm_op_greaterthan( vm_t *vm, uintptr_t arg ){
	if ( vm->argnum != 3 ){
		puts( "not enough args man" );
		return true;
	}

	scm_value_t op1 = vm_stack_pop( vm );
	scm_value_t op2 = vm_stack_pop( vm );

	vm_stack_pop( vm );
	vm_stack_push( vm, tag_boolean( op2 > op1 ));

	return true;
}

bool vm_op_intern_define( vm_t *vm, uintptr_t arg ){
	if ( vm->argnum != 3 ){
		puts( "not enough args man" );
		return true;
	}

	scm_value_t datum = vm_stack_pop( vm );
	scm_value_t sym   = vm_stack_pop( vm );

	if ( !is_symbol( sym )){
		puts( "expected symbol" );
		return true;
	}

	env_set( vm->env, sym, datum );

	return true;
}

bool vm_op_intern_set( vm_t *vm, uintptr_t arg ){
	if ( vm->argnum != 3 ){
		puts( "not enough args man" );
		return true;
	}

	scm_value_t datum = vm_stack_pop( vm );
	scm_value_t sym   = vm_stack_pop( vm );

	if ( !is_symbol( sym )){
		puts( "expected symbol" );
		return true;
	}

	env_set_recurse( vm->env, sym, datum );

	return true;
}

bool vm_op_intern_if( vm_t *vm, uintptr_t arg ){
	if ( vm->argnum != 4 ){
		printf( "if: not enough args, have %u but expected 4\n", vm->argnum );
		return true;
	}

	scm_value_t test       = vm_stack_pop( vm );
	scm_value_t false_path = vm_stack_pop( vm );
	scm_value_t true_path  = vm_stack_pop( vm );

	vm_stack_pop( vm );

	if ( test == tag_boolean( false )) {
		vm_stack_push( vm, false_path );

	} else {
		vm_stack_push( vm, true_path );
	}

	return true;
}

bool vm_op_display( vm_t *vm, uintptr_t arg ){
	if ( vm->argnum != 2 ){
		printf( "display: expected 2 args but have %u\n", vm->argnum );
		return true;
	}

	scm_value_t value = vm_stack_pop( vm );
	write_value( value );

	return true;
}

bool vm_op_newline( vm_t *vm, uintptr_t arg ){
	putchar( '\n' );

	return true;
}

bool vm_op_read( vm_t *vm, uintptr_t arg ){
	parse_state_t *input = make_parse_state( stdin );
	scm_value_t value = parse_expression( input );

	vm_stack_pop( vm );
	vm_stack_push( vm, value );

	free_parse_state( input );

	return true;
}
