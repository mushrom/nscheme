#ifndef _PARSE_H
#define _PARSE_H 1
#include <nscheme/values.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct parse_state {
	FILE *fp;

	scm_value_t next_token;
	bool has_next;

	unsigned linenum;
	unsigned charpos;
} parse_state_t;

// function prototypes defined in lex.c and parse.c
scm_value_t read_next_token( parse_state_t *state );
scm_value_t parse_list( parse_state_t *state );
scm_value_t parse_expression( parse_state_t *state );

parse_state_t *make_parse_state( FILE *fp );
void free_parse_state( FILE *fp );

#endif
