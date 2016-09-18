#include <nscheme/parse.h>
#include <nscheme/symbols.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define WHITESPACE " \t\v\n"
#define DIGITS     "0123456789"
#define ALPHABET   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define SYMBOLS    "<>+-*/:?^%&@!_=|"

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

		} else if ( c == ';' ){
			while ( c != '\n' && !feof( state->fp )){
				c = fgetc( state->fp );
			}

			ungetc( c, state->fp );

		} else if ( c == '(' ){
			return tag_parse_val( PARSE_TYPE_LEFT_PAREN );

		} else if ( c == ')' ){
			return tag_parse_val( PARSE_TYPE_RIGHT_PAREN );

		} else if ( matches( c, WHITESPACE )){
			state->charpos++;

		} else if ( matches( c, DIGITS )){
			ungetc( c, state->fp );
			return read_number( state );

		} else if ( matches( c, ALPHABET SYMBOLS)){
			ungetc( c, state->fp );
			return read_symbol( state );

		} else if ( c == '#' ){
			int next = fgetc( state->fp );

			if ( next == 't' || next == 'f' ){
				return tag_boolean( next == 't' );

			} else {
				ungetc( c, state->fp );
				return tag_parse_val( PARSE_TYPE_OCTOTHORPE );
			}

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
	const char *ret;
	unsigned i = 0;
	int c = fgetc( state->fp );
	char buf[32];

	while ( matches(c, ALPHABET DIGITS SYMBOLS) && i + 1 < sizeof(buf)){
		buf[i++] = c;
		c = fgetc( state->fp );
	}

	buf[i] = 0;
	ungetc( c, state->fp );

	if ( !( ret = lookup_symbol_address( buf ))){
		ret = strdup( buf );
		store_symbol( ret );
	}

	return tag_symbol( ret );
}
