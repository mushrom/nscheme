#ifndef _NSCHEME_ENV_H
#define _NSCHEME_ENV_H 1
#include <nscheme/values.h>

typedef struct env_node {
	scm_value_t key;
	scm_value_t value;

	struct env_node *left;
	struct env_node *right;
} env_node_t; 

typedef struct environment {
	struct env_node *root;
	struct environment *last;
} environment_t;

environment_t *env_create( environment_t *last );
void env_set( environment_t *env, scm_value_t key, scm_value_t value );
void env_set_recurse( environment_t *env, scm_value_t key, scm_value_t value );
env_node_t *env_find( environment_t *env, scm_value_t key );
env_node_t *env_find_recurse( environment_t *env, scm_value_t key );

#endif
