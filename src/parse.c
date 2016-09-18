#include <nscheme/parse.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/*
 * parser grammar overview:
 *
 * null  = ( )
 * token = number | character | symbol | vector | list | pair | null
 * list  = ( token ... )
 * expr  = token
 */

// Returns a cached token if one is available, or reads one from
// the parse stream otherwise
static inline scm_value_t get_next_token( parse_state_t *state ){
	scm_value_t ret = 0;

	if ( state->has_next ){
		ret = state->next_token;
		state->has_next = false;
		state->next_token = 0;

	} else {
		ret = read_next_token( state );
	}

	return ret;
}

// Similar to above, but doesn't consume the read/cached token
static inline scm_value_t peek_next_token( parse_state_t *state ){
	scm_value_t ret = 0;

	if ( state->has_next ){
		ret = state->next_token;

	} else {
		ret = read_next_token( state );
		state->has_next = true;
		state->next_token = ret;
	}

	return ret;
}

// Caches a token, which will be returned on the next call to get_next_token()
static inline void cache_next_token( parse_state_t *state, scm_value_t token ){
	state->next_token = token;
	state->has_next = true;
}

// checks to see if the next token matches the type(s) recognized by test(),
// and returns it if test() returns true, or a PARSE_TYPE_NONE token otherwise.
scm_value_t parse_accept( parse_state_t *state, scm_type_test_t test ){
	scm_value_t temp = get_next_token( state );

	if ( test( temp )){
		return temp;
	}

	cache_next_token( state, temp );
	return tag_parse_val( PARSE_TYPE_NONE );
}

// checks if the next token matches the types recognized by test().
// if test() returns false, the user is notified of an error and stuff happens.
scm_value_t parse_expect( parse_state_t *state, scm_type_test_t test, const char *msg ){
	scm_value_t temp = get_next_token( state );

	if ( test( temp )){
		return temp;

	} else {
		printf( "%s: expected %s near %u:%u\n", __func__, msg, state->linenum, state->charpos );
		puts( "TODO: handle this error" );
	}

	return tag_parse_val( PARSE_TYPE_NONE );
}

// macros for parse_accept, so parse functions read a bit nicer
#define ACCEPT(VAR, STATE, FUNC) \
	(( VAR = parse_accept((STATE), (FUNC))) != tag_parse_val( PARSE_TYPE_NONE ))

#define ACCEPT_AND_RETURN(STATE, FUNC) \
	{ \
		scm_value_t temp = parse_accept((STATE), (FUNC)); \
		\
		if ( temp != tag_parse_val( PARSE_TYPE_NONE )) { \
			return temp; \
		} \
	}

// Some extra type-checking functions specific to the parser
static inline bool is_left_paren( scm_value_t value ){
	return is_parse_val( value )
		&& get_parse_val( value ) == PARSE_TYPE_LEFT_PAREN;
}

static inline bool is_right_paren( scm_value_t value ){
	return is_parse_val( value )
		&& get_parse_val( value ) == PARSE_TYPE_RIGHT_PAREN;
}

static inline bool is_none_type( scm_value_t value ){
	return is_parse_val( value )
		&& get_parse_val( value ) == PARSE_TYPE_NONE;
}

scm_value_t parse_list( parse_state_t *state );

scm_value_t parse_token( parse_state_t *state ){
	ACCEPT_AND_RETURN( state, is_integer );
	ACCEPT_AND_RETURN( state, is_symbol );
	ACCEPT_AND_RETURN( state, is_eof );
	ACCEPT_AND_RETURN( state, is_boolean );

	if ( is_left_paren( peek_next_token( state ))) {
		return parse_list( state );
	}

	return tag_parse_val( PARSE_TYPE_NONE );
}

scm_value_t parse_list_tokens( parse_state_t *state ){
	scm_value_t temp = parse_token( state );

	if ( !is_none_type( temp )){
		return construct_pair( temp, parse_list_tokens( state ));

	} else {
		return SCM_TYPE_NULL;
	}
}

scm_value_t parse_list( parse_state_t *state ){
	scm_value_t temp = 0;

	parse_expect( state, is_left_paren, "left parenthesis" );

	//if ( is_right_paren( peek_next_token( state ))){
	if ( !is_none_type( parse_accept( state, is_right_paren ))){
		temp = SCM_TYPE_NULL;

	} else {
		temp = parse_list_tokens( state );
		parse_expect( state, is_right_paren, "right parenthesis" );
	}

	return temp;
}

scm_value_t parse_expression( parse_state_t *state ){
	if ( is_left_paren( peek_next_token( state ))) {
		return parse_list( state );

	} else {
		return parse_token( state );
	}
}

parse_state_t *make_parse_state( FILE *fp ){
	parse_state_t *ret = calloc( 1, sizeof( parse_state_t ));

	ret->fp = fp;

	return ret;
}

void free_parse_state( parse_state_t *state ){
	free( state );
}
