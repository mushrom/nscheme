#ifndef _PARSE_H
#define _PARSE_H 1
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

/*
 *   quick summary of tagged pointers defined here:
 *
 *   integer | 0 0 <integer>
 *
 *      pair | 1 0 0 <address>
 *    vector | 0 1 0 <address>
 *    string | 1 1 0 ... <address>
 *   closure | 1 0 1 ... <address>
 *    symbol | 0 1 1 ... <address>
 *
 *      char | 1 1 1 1 0 0 0 0 <codepoint>
 *   boolean | 1 1 1 1 0 0 1 <boolean>
 *      null | 1 1 1 1 0 1
 * parse val | 1 1 1 1 0 0 0 1 <type>
 */

enum parse_types {
	PARSE_TYPE_NONE,
	PARSE_TYPE_LEFT_PAREN,
	PARSE_TYPE_RIGHT_PAREN,
	PARSE_TYPE_OCTOTHORPE,
	PARSE_TYPE_PERIOD,
	PARSE_TYPE_EOF,
};

enum token_type { 
	SCM_TYPE_INTEGER   = 0x0,
	SCM_TYPE_PAIR      = 0x1,
	SCM_TYPE_VECTOR    = 0x2,
	SCM_TYPE_STRING    = 0x3,
	SCM_TYPE_CLOSURE   = 0x5,
	SCM_TYPE_SYMBOL    = 0x6,

	SCM_TYPE_CHAR      = 0xf,
	SCM_TYPE_BOOLEAN   = 0x4f,
	SCM_TYPE_NULL      = 0x2f,
	SCM_TYPE_PARSE_VAL = 0x8f,

	SCM_MASK_INTEGER   = 0x3,
	SCM_MASK_HEAP      = 0x7,
	SCM_MASK_CHAR      = 0xff,
	SCM_MASK_PARSE_VAL = 0xff,
	SCM_MASK_BOOLEAN   = 0x7f,
};

typedef uintptr_t scm_value_t;

typedef struct pair {
	scm_value_t car;
	scm_value_t cdr;
} scm_pair_t;

typedef struct parse_state {
	FILE *fp;

	scm_value_t next_token;
	bool has_next;

	unsigned linenum;
	unsigned charpos;
} parse_state_t;

typedef bool (*scm_type_test_t)(scm_value_t);

// function prototypes defined in lex.c and parse.c
scm_value_t read_next_token( parse_state_t *state );
scm_value_t parse_list( parse_state_t *state );
scm_value_t parse_expression( parse_state_t *state );

parse_state_t *make_parse_state( FILE *fp );
void free_parse_state( FILE *fp );

// tagging functions
static inline scm_value_t tag_integer( long int integer ){
	return (scm_value_t)integer << 2;
}

static inline scm_value_t tag_parse_val( unsigned type ){
	return (type << 8) | SCM_TYPE_PARSE_VAL;
}

static inline scm_value_t tag_pair( scm_pair_t *pair ){
	return (scm_value_t)pair | SCM_TYPE_PAIR;
}

// type testing functions
static inline bool is_integer( scm_value_t value ){
	return (value & SCM_MASK_INTEGER) == SCM_TYPE_INTEGER;
}

static inline bool is_parse_val( scm_value_t value ){
	return (value & SCM_MASK_PARSE_VAL) == SCM_TYPE_PARSE_VAL;
}

static inline bool is_pair( scm_value_t value ){
	return (value & SCM_MASK_HEAP) == SCM_TYPE_PAIR;
}

static inline bool is_null( scm_value_t value ){
	return value == SCM_TYPE_NULL;
}

// data retrieving functions
static inline long int get_integer( scm_value_t value ){
	return value >> 2;
}

static inline unsigned get_parse_val( scm_value_t value ){
	return value >> 8;
}

static inline scm_pair_t *get_pair( scm_value_t value ){
	return (scm_pair_t *)(value & ~SCM_MASK_HEAP);
}

#endif
