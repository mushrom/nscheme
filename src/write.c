#include <nscheme/vm.h>
#include <nscheme/write.h>
#include <stdio.h>

static inline void write_list( scm_pair_t *pair ){
	printf( "(" );

	for (;;) {
		write_value( pair->car );

		if ( is_pair( pair->cdr )){
			pair = get_pair( pair->cdr );
			printf( " " );

		} else if ( !is_null( pair->cdr )){
			printf( " . " );
			write_value( pair->cdr );
			break;

		} else {
			break;
		}
	}

	printf( ")" );
}

void write_value( scm_value_t value ){
	if ( is_integer( value )){
		printf( "%lu", get_integer( value ));

	} else if ( is_boolean( value )){
		printf( "#%c", get_boolean( value )? 't' : 'f' );

	} else if ( is_character( value )){
		printf( "#\\%c", get_character( value ));

	} else if ( is_pair( value )) {
		scm_pair_t *pair = get_pair( value );
		write_list( pair );

	} else if ( is_symbol( value )){
		printf( "%s", get_symbol( value ));

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
			"if", "begin", "eval",
		};

		printf( "#<runtime type:%s>", strs[get_run_type( value )]);

	} else {
		printf( "#<unknown>" );
	}
}
