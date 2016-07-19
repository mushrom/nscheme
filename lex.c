#include "parse.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define WHITESPACE " \t\v\n"
#define DIGITS     "0123456789"
#define ALPHABET   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

static inline bool matches( int c, const char *str ){
	bool ret = false;
	unsigned i;

	for ( i = 0; str[i]; i++ ){
		if ( c == str[i] ){
			ret = true;
			break;
		}
	}

	return ret;
}

scm_value_t read_number( parse_state_t *state );
scm_value_t read_symbol( parse_state_t *state );

scm_value_t read_next_token( parse_state_t *state ){
	scm_value_t ret = 0;
	int c;
	
	for (;;) {
		c = fgetc( state->fp );

		if ( feof( state->fp )){
			return tag_parse_val( PARSE_TYPE_EOF );

		} else if ( c == '\n' ){
			state->linenum++;
			state->charpos = 0;

		} else if ( c == '(' ){
			return tag_parse_val( PARSE_TYPE_LEFT_PAREN );

		} else if ( c == ')' ){
			return tag_parse_val( PARSE_TYPE_RIGHT_PAREN );

		} else if ( matches( c, WHITESPACE )){
			state->charpos++;

		} else if ( matches( c, DIGITS )){
			ungetc( c, state->fp );
			return read_number( state );

		} else if ( matches( c, ALPHABET )){
			ungetc( c, state->fp );
			return read_symbol( state );

		} else {
			puts( "error!" );
			// TODO: error out here
		}
	}

	return ret;
}

scm_value_t read_number( parse_state_t *state ){
	long int sum = 0;
	int c = fgetc( state->fp );

	while ( matches( c, DIGITS )){
		sum *= 10;
		sum += c - '0';

		c = fgetc( state->fp );
	}

	ungetc( c, state->fp );

	return tag_integer( sum );
}

scm_value_t read_symbol( parse_state_t *state ){
	return 0;
}
