#include <nscheme/parse.h>
#include <nscheme/vm.h>

void debug_print( scm_value_t value ){
	if ( is_integer( value )){
		printf( "%lu", get_integer( value ));

	} else if ( is_boolean( value )){
		printf( "#%c", get_boolean( value )? 't' : 'f' );

	} else if ( is_pair( value )) {
		scm_pair_t *pair = get_pair( value );

		printf( "(" );
		debug_print( pair->car );
		printf( " . " );
		debug_print( pair->cdr );
		printf( ")" );

	} else if ( is_symbol( value )){
		printf( "%s [%p]", get_symbol( value ), get_symbol( value ));

	} else if ( is_null( value )) {
		printf( "()" );

	} else if ( is_closure( value )){
		scm_closure_t *clsr = get_closure( value );
		printf( "#<closure:%s @ %p>",
			((char *[]){"interpreted", "compiled"})[clsr->compiled],
			clsr );

	} else if ( is_run_type( value )){
		const char *strs[] = {
			"<none>", "lambda", "define", "define-syntax", "set!",
			"if", "eval",
		};

		printf( "#<runtime type:%s>", strs[get_run_type( value )]);

	} else {
		printf( "#<unknown>" );
	}
}

void repl( vm_t *vm, parse_state_t *input ){
	scm_value_t temp = 0;

	while ( !is_eof( temp )){
		printf( " >> " );
		fflush( stdout );

		temp = parse_expression( input );
		temp = vm_evaluate_expr( vm, temp );

		printf( " => " );
		debug_print( temp );
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
