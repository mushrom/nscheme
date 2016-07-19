#include <nscheme/parse.h>

void debug_print( scm_value_t value ){
	if ( is_integer( value )){
		printf( "%lu", get_integer( value ));

	} else if ( is_pair( value )) {
		scm_pair_t *pair = get_pair( value );

		printf( "(" );
		debug_print( pair->car );
		printf( " . " );
		debug_print( pair->cdr );
		printf( ")" );

	} else if ( is_null( value )) {
		printf( "()" );

	} else {
		printf( "#<unknown>" );
	}
}

int main( int argc, char *argv[] ){
	parse_state_t *foo = make_parse_state( stdin );
	scm_value_t temp = parse_expression( foo );

	printf( "is integer? %s\n", is_integer( temp )? "true" : "false" );
	printf( "tagged: 0x%lx (%lu)\n", temp, temp );
	printf( "untagged: 0x%lx (%ld)\n", get_integer( temp ), get_integer( temp ));

	debug_print( temp );
	printf( "\n" );

	return 0;
}
