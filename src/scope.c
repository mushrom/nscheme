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

scope_node_t *scope_add_node( scope_t     *scope,
                              scm_value_t sym,
                              unsigned    type,
                              unsigned    location )
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
	node->type     = type;

	printf( "    | » added node %s:%u\n",
			loc_strs[type], location );

	return node;
}

void scope_handle_symbol( comp_state_t *state,
                          scope_t      *scope,
                          comp_node_t  *comp )
{
	scm_value_t sym = comp->car->value;
	scope_node_t *temp = scope_find( scope, sym, true );

	comp->car->scope = scope;

	printf( "    | » looking up symbol %s... ", get_symbol( sym ));

	if ( temp ){
		printf( "found as %s:%u\n",
				loc_strs[temp->type], temp->location );

		comp->car->node = temp;

	} else {
		env_node_t *env = env_find_recurse( state->env, sym );

		if ( !env ){
			printf( "could not resolve, error out or something\n" );
			return;
		}

		unsigned index = add_closure_node( state, env, sym );

		printf( "adding as closure:%u\n", index );
		comp->car->node = scope_add_node( scope, sym, SCOPE_CLOSURE, index );
	}
}

#include <nscheme/write.h>

void gen_sub_scope( comp_node_t  *comp,
                    comp_state_t *state,
                    scope_t      *cur_scope,
                    scm_value_t  syms,
                    unsigned     sp )
{
	for ( ; comp; comp = comp->cdr ){
		comp->scope = cur_scope;

		if ( comp->car && is_symbol( comp->car->value )){
			if ( is_if_token( state->env, comp->car->value )){
				printf( "    | » have if statement\n" );

			} else if ( is_define_statement( state->env, comp )){
				printf( "    | » define statement only allowed at top-level!\n" );

			} else {
				scope_handle_symbol( state, cur_scope, comp );
			}

		} else if ( is_pair( comp->value )){
			gen_sub_scope( comp->car, state, cur_scope, syms, sp++ );
		}
	}
}

void gen_top_scope( comp_node_t  *comp,
                    comp_state_t *state,
                    scope_t      *cur_scope,
                    scm_value_t  syms,
                    unsigned     sp )
{
	bool is_root_scope = !cur_scope;

	scope_t *new_scope = calloc( 1, sizeof( scope_t ));
	new_scope->last = cur_scope;
	cur_scope = new_scope;

	for ( ; is_pair(syms); syms = scm_cdr( syms )){
		unsigned type = is_root_scope? SCOPE_PARAMETER : SCOPE_LOCAL;

		scope_add_node( cur_scope, scm_car( syms ), type, sp++ );
	}

	comp_node_t *start = comp;
	unsigned orig_sp = sp;

	// TODO: this doesn't handle (define (...) ...) expressions yet
	for ( ; is_define_statement( state->env, comp->car ); comp = comp->cdr ){
		scm_value_t name = comp->car->cdr->car->value;

		printf( "    | » found a define statement, name: " );
		write_value( name );
		printf( "\n" );

		scope_add_node( cur_scope, name, SCOPE_LOCAL, sp++ );
	}

	comp = start;
	sp = orig_sp;

	for ( ; is_define_statement( state->env, comp->car ); comp = comp->cdr ){
		scm_value_t name = comp->car->cdr->car->value;
		comp_node_t *body = comp->car->cdr->cdr;

		printf( "    | » generating scope for define statement " );
		write_value( name );
		printf( "\n" );

		gen_sub_scope( body, state, cur_scope, SCM_TYPE_NULL, sp++ );
	}

	gen_sub_scope( comp, state, cur_scope, SCM_TYPE_NULL, sp++ );
}
