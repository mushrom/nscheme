#ifndef _NSCHEME_VALUES_H
#define _NSCHEME_VALUES_H 1

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/*
 *   summary of tagged pointers defined here:
 *
 *         integer | 0 0 <integer>
 *
 *            pair | 1 0 0 0 <address>
 *          vector | 0 1 0 0 <address>
 *          string | 1 1 0 0 <address>
 *         closure | 1 0 1 0 <address>
 *          symbol | 0 1 1 0 <address>
 *    syntax-rules | 1 1 1 0 <address>
 *    external ptr | 1 0 0 1 <address>
 *   other numbers | 0 1 0 1 <address> (floats, bigints, rationals, complex)
 *           TODO: not sure if these are final yet
 *                 could have a single array type and store the contained
 *                 type in there, if a need arises for more heap tags
 *     float-array | 1 1 0 1 <address>
 *       int-array | 1 0 1 1 <address>
 *      byte-array | 0 1 1 1 <address>
 *
 *            char | 1 1 1 1 0 0 0 0 <codepoint>
 *         boolean | 1 1 1 1 0 0 1 0 <boolean>
 *            null | 1 1 1 1 0 1 0 0
 *       parse val | 1 1 1 1 0 0 0 1 <type>
 *    runtime type | 1 1 1 1 0 0 1 1 <type>
 */

enum parse_types {
	PARSE_TYPE_NONE,
	PARSE_TYPE_LEFT_PAREN,
	PARSE_TYPE_RIGHT_PAREN,
	PARSE_TYPE_OCTOTHORPE,
	PARSE_TYPE_EOF,
	PARSE_TYPE_APOSTROPHE,
};

enum runtime_types {
	RUN_TYPE_NONE,
	RUN_TYPE_LAMBDA,
	RUN_TYPE_DEFINE,
	RUN_TYPE_DEFINE_SYNTAX,
	RUN_TYPE_SET,
	RUN_TYPE_IF,
	RUN_TYPE_BEGIN,
	RUN_TYPE_QUOTE,
	RUN_TYPE_EVAL,
	RUN_TYPE_SET_PTR,
	RUN_TYPE_SYNTAX_RULES,
};

enum token_type {
	SCM_TYPE_INTEGER      = 0x0,
	SCM_TYPE_PAIR         = 0x1,
	SCM_TYPE_VECTOR       = 0x2,
	SCM_TYPE_STRING       = 0x3,
	SCM_TYPE_CLOSURE      = 0x5,
	SCM_TYPE_SYMBOL       = 0x6,
	SCM_TYPE_SYNTAX_RULES = 0x7,
	SCM_TYPE_EXTERNAL_PTR = 0x9,
	SCM_TYPE_BIG_NUMBER   = 0xa,
	SCM_TYPE_FLOAT_ARRAY  = 0xb,
	SCM_TYPE_INT_ARRAY    = 0xd,
	SCM_TYPE_BYTE_ARRAY   = 0xe,

	SCM_TYPE_CHAR      = 0xf,
	SCM_TYPE_BOOLEAN   = 0x4f,
	SCM_TYPE_NULL      = 0x2f,
	SCM_TYPE_PARSE_VAL = 0x8f,
	SCM_TYPE_RUN_TYPE  = 0xcf,

	SCM_MASK_INTEGER   = 0x3,
	SCM_MASK_HEAP      = 0xf,
	SCM_MASK_CHAR      = 0xff,
	SCM_MASK_PARSE_VAL = 0xff,
	SCM_MASK_RUN_TYPE  = 0xff,
	SCM_MASK_BOOLEAN   = 0xff,
};

typedef uintptr_t scm_value_t;

typedef struct pair {
	scm_value_t car;
	scm_value_t cdr;
} scm_pair_t;

typedef struct syntax_rules {
	scm_pair_t *keywords;
	scm_pair_t *patterns;
} scm_syntax_rules_t;

typedef bool (*scm_type_test_t)(scm_value_t);

// tagging functions
static inline scm_value_t tag_integer(long int integer) {
	return (scm_value_t)integer << 2;
}

static inline scm_value_t tag_parse_val(unsigned type) {
	return (type << 8) | SCM_TYPE_PARSE_VAL;
}

static inline scm_value_t tag_run_type(unsigned type) {
	return (type << 8) | SCM_TYPE_RUN_TYPE;
}

static inline scm_value_t tag_pair(scm_pair_t *pair) {
	return (scm_value_t)pair | SCM_TYPE_PAIR;
}

static inline scm_value_t tag_heap_type(void *ptr, unsigned type) {
	return (scm_value_t)((uintptr_t)ptr | type);
}

static inline scm_value_t tag_symbol(const char *str) {
	return (scm_value_t)str | SCM_TYPE_SYMBOL;
}

static inline scm_value_t tag_closure(void *closure) {
	return (scm_value_t)closure | SCM_TYPE_CLOSURE;
}

static inline scm_value_t tag_boolean(bool boolean) {
	return (boolean << 8) | SCM_TYPE_BOOLEAN;
}

static inline scm_value_t tag_character(unsigned character) {
	return (character << 8) | SCM_TYPE_CHAR;
}

static inline scm_value_t construct_pair(scm_value_t car, scm_value_t cdr) {
	scm_pair_t *pair = calloc(1, sizeof(scm_pair_t));

	pair->car = car;
	pair->cdr = cdr;

	return tag_pair(pair);
}

// type testing functions
static inline bool is_integer(scm_value_t value) {
	return (value & SCM_MASK_INTEGER) == SCM_TYPE_INTEGER;
}

static inline bool is_parse_val(scm_value_t value) {
	return (value & SCM_MASK_PARSE_VAL) == SCM_TYPE_PARSE_VAL;
}

static inline bool is_run_type(scm_value_t value) {
	return (value & SCM_MASK_RUN_TYPE) == SCM_TYPE_RUN_TYPE;
}

static inline unsigned get_heap_type(scm_value_t value) {
	return value & SCM_MASK_HEAP;
}

static inline bool is_pair(scm_value_t value) {
	return get_heap_type(value) == SCM_TYPE_PAIR;
}

static inline bool is_syntax_rules(scm_value_t value) {
	return get_heap_type(value) == SCM_TYPE_SYNTAX_RULES;
}

static inline bool is_null(scm_value_t value) {
	return value == SCM_TYPE_NULL;
}

static inline bool is_symbol(scm_value_t value) {
	return (value & SCM_MASK_HEAP) == SCM_TYPE_SYMBOL;
}

static inline bool is_closure(scm_value_t value) {
	return (value & SCM_MASK_HEAP) == SCM_TYPE_CLOSURE;
}

static inline bool is_eof(scm_value_t value) {
	return is_parse_val(value)
	       && (value >> 8) == PARSE_TYPE_EOF;
}

static inline bool is_boolean(scm_value_t value) {
	return (value & SCM_MASK_BOOLEAN) == SCM_TYPE_BOOLEAN;
}

static inline bool is_character(scm_value_t value) {
	return (value & SCM_MASK_CHAR) == SCM_TYPE_CHAR;
}

// data retrieving functions
static inline long int get_integer(scm_value_t value) {
	return value >> 2;
}

static inline unsigned get_parse_val(scm_value_t value) {
	return value >> 8;
}

static inline unsigned get_run_type(scm_value_t value) {
	return value >> 8;
}

static inline void *get_heap_tagged_value(scm_value_t value) {
	return (void *)(value & ~SCM_MASK_HEAP);
}

static inline scm_pair_t *get_pair(scm_value_t value) {
	return get_heap_tagged_value(value);
}

static inline scm_syntax_rules_t *get_syntax_rules(scm_value_t value) {
	return get_heap_tagged_value(value);
}

static inline const char *get_symbol(scm_value_t value) {
	return (const char *)(value & ~SCM_MASK_HEAP);
}

static inline void *get_closure(scm_value_t value) {
	return (void *)(value & ~SCM_MASK_HEAP);
}

static inline bool get_boolean(scm_value_t value) {
	return value >> 8;
}

static inline unsigned get_character(scm_value_t value) {
	return value >> 8;
}

// composite functions
static inline bool is_special_form(scm_value_t value) {
	bool ret = false;

	if (is_run_type(value)) {
		unsigned type = get_run_type(value);

		ret = type == RUN_TYPE_LAMBDA
		   || type == RUN_TYPE_DEFINE
		   || type == RUN_TYPE_DEFINE_SYNTAX
		   || type == RUN_TYPE_SET
		   || type == RUN_TYPE_IF
		   || type == RUN_TYPE_BEGIN
		   || type == RUN_TYPE_QUOTE
		   || type == RUN_TYPE_SYNTAX_RULES
		   ;
	}

	return ret;
}

// pair accessor functions
// TODO: return an error value instead of SCM_TYPE_NULL
//       by default, or throw the error directly
static inline scm_value_t scm_car(scm_value_t value) {
	scm_value_t ret = SCM_TYPE_NULL;

	if (is_pair(value)) {
		scm_pair_t *pair = get_pair(value);
		ret = pair->car;
	}

	return ret;
}

static inline scm_value_t scm_cdr(scm_value_t value) {
	scm_value_t ret = SCM_TYPE_NULL;

	if (is_pair(value)) {
		scm_pair_t *pair = get_pair(value);
		ret = pair->cdr;
	}

	return ret;
}

#endif
