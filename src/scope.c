#include <nscheme/compiler.h>

static const char *loc_strs[] = {
	"parameter",
	"mutable parameter",
	"closure",
	"local",
	"mutable local",
};

scope_node_t *scope_find(scope_t *scope, scm_value_t sym, bool recurse) {
	do {
		scope_node_t *temp = scope->nodes;

		for (; temp; temp = temp->next) {
			if (temp->sym == sym) {
				return temp;
			}
		}

		scope = scope->last;
	} while (scope && recurse);

	return NULL;
}

scope_node_t *scope_add_node(scope_t     *scope,
                             scm_value_t sym,
                             unsigned    type,
                             unsigned    location)
{
	scope_node_t *node = scope_find(scope, sym, false);

	if (!node) {
		node = calloc(1, sizeof(scope_node_t));
		node->sym      = sym;
		node->type     = type;

		node->next = scope->nodes;
		scope->nodes = node;
	}

	// TODO: is this right? overwriting the previous location and type of
	//       the original node? double check
	node->location = location;
	node->type     = type;

	DEBUG_PRINTF("    | » added node %s:%u\n",
	             loc_strs[type], location);

	return node;
}

// returns true if successfully found
bool scope_handle_symbol(comp_state_t *state,
                         scope_t      *scope,
                         comp_node_t  *comp)
{
	scm_value_t sym = comp->car->value;
	scope_node_t *temp = scope_find(scope, sym, true);

	comp->car->scope = scope;

	DEBUG_PRINTF("    | » looking up symbol %s... ", get_symbol(sym));

	if (temp) {
		DEBUG_PRINTF("found as %s:%u\n",
		             loc_strs[temp->type], temp->location);

		comp->car->node = temp;
		return true;

	} else {
		env_node_t *env = env_find_recurse(state->env, sym);

		if (!env) {
			DEBUG_PRINTF("could not resolve, error out or something\n");
			return false;
		}

		unsigned index = add_closure_node(state, env, sym);

		DEBUG_PRINTF("adding as closure:%u\n", index);
		comp->car->node = scope_add_node(scope, sym, SCOPE_CLOSURE, index);
		return true;
	}
}

#include <nscheme/write.h>

// return false on failure:
// - illegal (define ...) expression
// - undefined name
// TODO: error message
bool gen_sub_scope(comp_node_t  *comp,
                   comp_state_t *state,
                   scope_t      *cur_scope,
                   scm_value_t  syms,
                   unsigned     sp)
{
	for (; comp; comp = comp->cdr) {
		comp->scope = cur_scope;

		if (comp->car && is_symbol(comp->car->value)) {
			if (is_if_token(state->env, comp->car->value)) {
				DEBUG_PRINTF("    | » have if statement\n");

			} else if (is_define_statement(state->env, comp)) {
				DEBUG_PRINTF("    | » define statement only allowed at top-level!\n");
				return false;

			} else {
				if (!scope_handle_symbol(state, cur_scope, comp)) {
					return false;
				}
			}

		} else if (is_pair(comp->value)) {
			if (!gen_sub_scope(comp->car, state, cur_scope, syms, sp++)) {
				return false;
			}
		}
	}

	return true;
}

bool gen_top_scope(comp_node_t  *comp,
                   comp_state_t *state,
                   scope_t      *cur_scope,
                   scm_value_t  syms,
                   unsigned     sp)
{
	bool is_root_scope = !cur_scope;

	scope_t *new_scope = calloc(1, sizeof(scope_t));
	new_scope->last = cur_scope;
	cur_scope = new_scope;

	for (; is_pair(syms); syms = scm_cdr(syms)) {
		unsigned type = is_root_scope? SCOPE_PARAMETER : SCOPE_LOCAL;

		scope_add_node(cur_scope, scm_car(syms), type, sp++);
	}

	comp_node_t *start = comp;
	unsigned orig_sp = sp;

	// TODO: this doesn't handle (define (...) ...) expressions yet
	for (; is_define_statement(state->env, comp->car); comp = comp->cdr) {
		scm_value_t name = comp->car->cdr->car->value;

		DEBUG_PRINTF("    | » found a define statement, name: ");
		DEBUG_WRITEVAL(name);
		DEBUG_PRINTF("\n");

		scope_add_node(cur_scope, name, SCOPE_LOCAL, sp++);
	}

	comp = start;
	sp = orig_sp;

	for (; is_define_statement(state->env, comp->car); comp = comp->cdr) {
		scm_value_t name = comp->car->cdr->car->value;
		comp_node_t *body = comp->car->cdr->cdr;

		DEBUG_PRINTF("    | » generating scope for define statement ");
		DEBUG_WRITEVAL(name);
		DEBUG_PRINTF("\n");

		if (!gen_sub_scope(body, state, cur_scope, SCM_TYPE_NULL, sp++)) {
			return false;
		}
	}

	return gen_sub_scope(comp, state, cur_scope, SCM_TYPE_NULL, sp++);
}
