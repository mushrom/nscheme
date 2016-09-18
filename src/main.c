#include <nscheme/parse.h>
#include <nscheme/vm.h>
#include <nscheme/write.h>

void repl( vm_t *vm, parse_state_t *input ){
	scm_value_t temp = 0;

	while ( !is_eof( temp )){
		printf( " >> " );
		fflush( stdout );

		temp = parse_expression( input );
		temp = vm_evaluate_expr( vm, temp );

		printf( " => " );
		write_value( temp );
		printf( "\n" );
		fflush( stdout );
	}
}

int main( int argc, char *argv[] ){
	parse_state_t *foo = make_parse_state( stdin );
	/*
	scm_value_t temp = parse_expression( foo );

	printf( "is integer? %s\n", is_integer( temp )? "true" : "false" );
	printf( "tagged: 0x%lx (%lu)\n", temp, temp );
	printf( "untagged: 0x%lx (%ld)\n", get_integer( temp ), get_integer( temp ));

	debug_print( temp );
	printf( "\n" );
	*/

	vm_t *vm = vm_init( );
	repl( vm, foo );
	vm_free( vm );

	return 0;
}
