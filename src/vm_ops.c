#include <nscheme/vm.h>
#include <nscheme/vm_ops.h>

bool vm_op_return( vm_t *vm, unsigned arg ){
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
