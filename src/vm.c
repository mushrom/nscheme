#include <nscheme/vm.h>
#include <stdlib.h>

void run_vm_compiled( vm_t *vm ){
	vm_op_t *code = vm->closure->code;

}

void run_vm_interpreter( vm_t *vm ){

}

void run_vm( vm_t *vm ){
	if ( vm->closure->is_compiled ){
		run_vm_compiled( vm );

	} else {
		run_vm_interpreter( vm );
	}
}

vm_t *init_vm( scm_closure_t *init ){
	vm_t *ret = calloc( 1, sizeof( vm_t ));

	ret->stack_size = 0x1000;
	ret->calls_size = 0x1000;
	ret->stack = calloc( 1, sizeof( scm_value_t[ret->stack_size] ));
	ret->calls = calloc( 1, sizeof( vm_callframe_t[ret->calls_size] ));

	ret->closure = init;

	return ret;
}
