#include <nscheme/symbols.h>
#include <string.h>
#include <stdlib.h>
/*
 * TODO: find a way to garbage collect the symbol table
 */

typedef struct tree_node {
	struct tree_node *left;
	struct tree_node *right;

	const char *data;
} tree_node_t;

tree_node_t *tree_lookup( tree_node_t *tree, const char *symbol ){
	tree_node_t *ret = NULL;

	if ( tree ){
		int n = strcmp( tree->data, symbol );

		if ( n == 0 ){
			ret = tree;

		} else if ( n < 0 ){
			ret = tree_lookup( tree->left, symbol );

		} else {
			ret = tree_lookup( tree->right, symbol );
		}
	}

	return ret;
}

tree_node_t *tree_insert( tree_node_t *tree, const char *symbol ){
	tree_node_t *ret = NULL;

	if ( tree ){
		int n = strcmp( tree->data, symbol );
		ret = tree;

		if ( n < 0 ){
			tree->left = tree_insert( tree->left, symbol );

		} else if ( n > 0 ){
			tree->right = tree_insert( tree->right, symbol );
		}

	} else {
		ret = calloc( 1, sizeof( tree_node_t ));
		ret->data = symbol;
	}

	return ret;
}

static tree_node_t *symbol_tree = NULL;
// TODO: mutex here to lock accesses to symbol_tree

const char *lookup_symbol_address( const char *symbol ){
	tree_node_t *temp = tree_lookup( symbol_tree, symbol );

	if ( temp ){
		return temp->data;

	} else {
		return NULL;
	}
}

const char *store_symbol( const char *symbol ){
	tree_node_t *temp = tree_insert( symbol_tree, symbol );

	if ( !symbol_tree )
		symbol_tree = temp;

	return symbol;
}

const char *try_store_symbol( const char *symbol ){
	const char *ret = lookup_symbol_address( symbol );

	if ( !ret ){
		size_t len = strlen( symbol );
		char *temp = malloc( len + 1 );

		strncpy( temp, symbol, len + 1 );
		ret = temp;

		store_symbol( ret );
	}

	return ret;
}
