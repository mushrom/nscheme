#include <nscheme/env.h>
#include <stdlib.h>
#include <stdio.h>

environment_t *env_create( environment_t *last ){
	environment_t *ret = calloc(1, sizeof( environment_t ));

	ret->last = last;

	return ret;
}

void env_set( environment_t *env, unsigned type, scm_value_t key, scm_value_t value ){
	env_node_t *node = env->root;

	if ( !env->root ){
		env->root = node = calloc(1, sizeof( env_node_t ));

	} else {
		env_node_t *temp = env->root;

		while ( temp && key != node->key ){
			node = temp;
			temp = (key < temp->key)? temp->left : temp->right;
		}

		if ( key < node->key ){
			node->left = calloc(1, sizeof( env_node_t ));
			node = node->left;

		} else if ( key > node->key ){
			node->right = calloc(1, sizeof( env_node_t ));
			node = node->right;
		}
	}

	node->type  = type;
	node->key   = key;
	node->value = value;
}

env_node_t *env_find( environment_t *env, scm_value_t key ){
	env_node_t *ret = env->root;

	while ( ret && key != ret->key ){
		ret = (key < ret->key)? ret->left : ret->right;
	}

	return ret;
}
