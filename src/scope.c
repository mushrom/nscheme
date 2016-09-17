#include <nscheme/compiler.h>

static const char *loc_strs[] = {
	"parameter",
	"mutable parameter",
	"closure",
	"local",
	"mutable local",
};

scope_node_t *scope_find( scope_t *scope, scm_value_t sym, bool recurse ){
	do {
		scope_node_t *temp = scope->nodes;

		for ( ; temp; temp = temp->next ){
			if ( temp->sym == sym ){
				return temp;
			}
		}

		scope = scope->last;
	} while ( scope && recurse );

	return NULL;
}

void scope_add_node( scope_t *scope,
                     scm_value_t sym,
                     unsigned type,
                     unsigned location )
{
	scope_node_t *node = scope_find( scope, sym, false );

	if ( !node ){
		node = calloc( 1, sizeof( scope_node_t ));
		node->sym      = sym;
		node->type     = type;

		node->next = scope->nodes;
		scope->nodes = node;
	}

	node->location = location;


	printf( "    | » added node %s:%u\n",
			loc_strs[type], location );
}

void gen_scope( comp_node_t *comp,
                environment_t *env,
                scope_t *cur_scope,
                scm_value_t syms,
                unsigned sp )
{
	bool is_root_scope = !cur_scope;

	if ( !cur_scope ){
		cur_scope = calloc( 1, sizeof( scope_t ));
	}

	for ( ; is_pair(syms); syms = scm_cdr( syms )){
		unsigned type = is_root_scope? SCOPE_PARAMETER : SCOPE_LOCAL;

		scope_add_node( cur_scope, scm_car( syms ), type, sp++ );
	}

	for ( ; comp; comp = comp->cdr ){
		comp->scope = cur_scope;

		if ( comp->car && is_symbol( comp->car->value )){
			scope_node_t *temp;
			temp = scope_find( cur_scope, comp->car->value, true );

			printf( "    | » looking up symbol %s... ",
					get_symbol( comp->car->value ));

			if ( temp ){
				printf( "found as %s:%u\n",
					loc_strs[temp->type], temp->location );

			} else {
				printf( "not found, likely a closure value\n" );
			}

		} else if ( is_pair( comp->value )){
			gen_scope( comp->car, env, cur_scope, syms, sp++ );
		}
	}
}
