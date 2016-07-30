#ifndef _NSCHEME_ENV_H
#define _NSCHEME_ENV_H 1
#include <nscheme/values.h>

enum {
	ENV_TYPE_SYNTAX,
	ENV_TYPE_DATA,
};

typedef struct env_node {
	scm_value_t key;
	scm_value_t value;

	struct env_node *left;
	struct env_node *right;

	unsigned type;
} env_node_t; 

typedef struct environment {
	struct env_node *root;
	struct environment *last;
} environment_t;

environment_t *make_env( environment_t *last );


#endif
