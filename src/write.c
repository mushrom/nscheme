#include <nscheme/vm.h>
#include <nscheme/write.h>
#include <stdio.h>

void write_value( scm_value_t value ){
	if ( is_integer( value )){
		printf( "%lu", get_integer( value ));

	} else if ( is_boolean( value )){
		printf( "#%c", get_boolean( value )? 't' : 'f' );

	} else if ( is_character( value )){
		printf( "#\\%c", get_character( value ));

	} else if ( is_pair( value )) {
		scm_pair_t *pair = get_pair( value );

		printf( "(" );
		write_value( pair->car );
		printf( " . " );
		write_value( pair->cdr );
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
