#ifndef _NSCHEME_VALUES_H
#define _NSCHEME_VALUES_H 1

#include <stdint.h>
#include <stdbool.h>

/*
 *   summary of tagged pointers defined here:
 *
 *   integer | 0 0 <integer>
 *
 *      pair | 1 0 0 <address>
 *    vector | 0 1 0 <address>
 *    string | 1 1 0 <address>
 *   closure | 1 0 1 <address>
 *    symbol | 0 1 1 <address>
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

typedef bool (*scm_type_test_t)(scm_value_t);

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

static inline scm_value_t tag_symbol( const char *str ){
	return (scm_value_t)str | SCM_TYPE_SYMBOL;
}

static inline scm_value_t tag_closure( void *closure ){
	return (scm_value_t)closure | SCM_TYPE_CLOSURE;
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

static inline bool is_symbol( scm_value_t value ){
	return (value & SCM_MASK_HEAP) == SCM_TYPE_SYMBOL;
}

static inline bool is_closure( scm_value_t value ){
	return (value & SCM_MASK_HEAP) == SCM_TYPE_CLOSURE;
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

static inline const char *get_symbol( scm_value_t value ){
	return (const char *)(value & ~SCM_MASK_HEAP);
}

static inline void *get_closure( scm_value_t value ){
	return (void *)(value & ~SCM_MASK_HEAP);
}

#endif
