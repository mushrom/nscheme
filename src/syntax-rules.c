#include <nscheme/syntax-rules.h>
#include <nscheme/vm_ops.h>
#include <nscheme/write.h>

#include <string.h>

static inline scm_pair_t *next(scm_pair_t *pair) {
    return (pair && is_pair(pair->cdr))? get_pair(pair->cdr) : NULL;
}

static inline
bool is_ellipsis(scm_value_t value) {
	if (!is_symbol(value))
		return false;

	const char *sym = get_symbol(value);
	return strcmp(sym, "...") == 0;
}

bool symbol_in_list(scm_pair_t *keywords, const char *symbol) {
	if (!keywords)
		return false;

	for (scm_pair_t *it = keywords; it; it = next(it)) {
		if (!is_symbol(it->car))
			continue;

		const char *keyword = get_symbol(it->car);

		// symbols are unique, don't need to compare string values
		if (keyword == symbol)
			return true;
	}

	return false;
}

static inline
bool matches(scm_pair_t *keywords,
             scm_pair_t *pattern,
             scm_pair_t *expr)
{
	scm_pair_t *p_it = pattern;
	scm_pair_t *e_it = expr;

	for (; p_it; p_it = next(p_it), e_it = next(e_it)) {
		if (next(p_it) && is_ellipsis(next(p_it)->car))
			// always match, even if e_it is empty
			return true;

		if (!e_it)
			// if the pattern isn't variable-length and the expression has reached
			// the end, then it can't match
			return false;

		if (is_symbol(p_it->car)) {
			const char *p_symbol = get_symbol(p_it->car);

			if (symbol_in_list(keywords, p_symbol)) {
				if (!is_symbol(e_it->car))
					// symbol values must match if the pattern is a keyword value
					return false;

				const char *e_symbol = get_symbol(e_it->car);

				if (e_symbol != p_symbol)
					// keywords don't match
					return false;
			}

			// otherwise, values match, expression value will be bound to pattern symbol

		} else if (is_pair(p_it->car)) {
			if (!is_pair(e_it->car))
				return false;

			scm_pair_t *sub_pat = get_pair(p_it->car);
			scm_pair_t *sub_exp = get_pair(e_it->car);

			if (!matches(keywords, sub_pat, sub_exp))
				return false;

		} else if (p_it->car != e_it->car) {
			// check for matching literals
			return false;
		}
	}

	// if both lists have reached the end then all tokens have matched
	return !(p_it || e_it);
}

struct binding {
	const char *name;
	scm_value_t value;
	struct binding *next;
	bool is_variable_length;
};

struct binding_list {
	struct binding *root;
};

static inline
struct binding *make_binding(void) {
	return calloc(1, sizeof(struct binding));
}

static inline
struct binding_list *make_binding_list(void) {
	return calloc(1, sizeof(struct binding_list));
}

static inline
void free_binding_list(struct binding_list *bindings) {
	struct binding *root = bindings->root;

	while (root) {
		struct binding *temp = root->next;
		free(root);
		root = temp;
	}

	free(bindings);
}

static inline
void add_binding(struct binding_list *list, struct binding *ptr) {
	ptr->next = list->root;
	list->root = ptr;
}

static inline
struct binding *find_binding(struct binding_list *list, const char *name) {
	for (struct binding *it = list->root; it; it = it->next) {
		if (it->name == name) {
			return it;
		}
	}

	return 0;
}

// pattern is assumed to match the expression here, leaves out some error checking
static inline
void build_bindings(scm_pair_t *keywords,
                    scm_pair_t *pattern,
                    scm_pair_t *expr,
                    struct binding_list *bindings)
{
	scm_pair_t *p_it = pattern;
	scm_pair_t *e_it = expr;

	for (; p_it; p_it = next(p_it), e_it = next(e_it)) {
		if (next(p_it) && is_ellipsis(next(p_it)->car)) {
			const char *p_symbol = get_symbol(p_it->car);
			printf("Adding variable-length binding: '%s'\n", p_symbol);

			struct binding *bind = make_binding();
			bind->name  = p_symbol;
			bind->value = e_it? tag_pair(e_it) : SCM_TYPE_NULL;
			bind->is_variable_length = true;

			add_binding(bindings, bind);

			// there shouldn't be anything left to match in this list after the ellipsis
			// TODO: wait, is that allowed?
			return;
		}

		if (!e_it) return;

		if (is_symbol(p_it->car)) {
			const char *p_symbol = get_symbol(p_it->car);

			if (symbol_in_list(keywords, p_symbol)) {
				// don't bind to keywords
				continue;
			}

			struct binding *bind = make_binding();
			bind->name  = p_symbol;
			bind->value = e_it->car;

			add_binding(bindings, bind);

		} else if (is_pair(p_it->car)) {
			scm_pair_t *sub_pat = get_pair(p_it->car);
			scm_pair_t *sub_exp = get_pair(e_it->car);

			build_bindings(keywords, sub_pat, sub_exp, bindings);
		}
	}
}

// pattern is assumed to match the expression here
static inline
scm_value_t expand(struct binding_list *bindings,
                   scm_pair_t *expansion)
{
	scm_value_t retcar = SCM_TYPE_NULL;
	scm_value_t retcdr = SCM_TYPE_NULL;

	if (is_symbol(expansion->car)) {
		const char *sym = get_symbol(expansion->car);
		struct binding *bind = find_binding(bindings, sym);

		if (bind) {
			retcar = bind->value;
			if (bind->is_variable_length) {
				printf("have variable-length symbol: '%s'\n", bind->name);
				return bind->value;

			} else {
				retcar = bind->value;
			}

		} else {
			// leave symbol in expansion to be evaluated at runtime
			retcar = expansion->car;
		}
	}

	else if (is_pair(expansion->car)) {
		retcar = expand(bindings, get_pair(expansion->car));
	}

	else {
		// copy values of other types
		retcar = expansion->car;
	}

	if (is_pair(expansion->cdr)) {
		if (retcdr != SCM_TYPE_NULL) {
			puts("detected values after expanding a variable-length match! need to check for this when defining syntax rules");
		}

		retcdr = expand(bindings, get_pair(expansion->cdr));
	}

	// TODO: does this need to handle improper lists?

	return construct_pair(retcar, retcdr);
}

scm_value_t expand_syntax_rules(vm_t *vm,
                                scm_syntax_rules_t *rules,
                                scm_pair_t *expr)
{
	scm_value_t ret = SCM_TYPE_NULL;
	scm_pair_t *it = rules->patterns;

	bool found = false;
	for (; it; it = next(it)) {
		scm_pair_t *p = get_pair(it->car);

		// TODO: need to check that syntax-rules has a valid form when defining patterns
		scm_pair_t *pattern   = get_pair(p->car);
		scm_pair_t *expansion = get_pair(p->cdr);

		if (matches(rules->keywords, pattern, expr)) {
			found = true;

			puts("have matching pattern:");
			write_list(pattern); putchar('\n');

			puts("for expression:");
			write_list(expr); putchar('\n');

			struct binding_list *bindings = make_binding_list();
			build_bindings(rules->keywords, pattern, expr, bindings);
			ret = expand(bindings, expansion);
			free_binding_list(bindings);

			puts("expanded to:");
			write_value(ret); putchar('\n');

			break;
		}
	}

	if (!found) {
		vm_error(vm, "no matching syntax definition");
	}

	vm_stack_push(vm, vm_func_return_last());
	vm_stack_push(vm, SCM_TYPE_NULL);

    return ret;
}
