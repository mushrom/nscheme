#include <nscheme/syntax-rules.h>
#include <nscheme/vm_ops.h>
#include <nscheme/write.h>

static inline scm_pair_t *next(scm_pair_t *pair) {
    return is_pair(pair->cdr)? get_pair(pair->cdr) : NULL;
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

	for (; p_it && e_it; p_it = next(p_it), e_it = next(e_it)) {
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

			// TODO: check for ellipsis
			// otherwise, values match, expression value will be bound to pattern symbol

		} else if (is_pair(p_it->car)) {
			if (!is_pair(e_it->car))
				return false;

			scm_pair_t *sub_pat = get_pair(p_it->car);
			scm_pair_t *sub_exp = get_pair(e_it->car);

			if (!matches(keywords, sub_pat, sub_exp))
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

// pattern is assumed to match the expression here
static inline
void build_bindings(scm_pair_t *keywords,
                    scm_pair_t *pattern,
                    scm_pair_t *expr,
                    struct binding_list *bindings)
{
	scm_pair_t *p_it = pattern;
	scm_pair_t *e_it = expr;

	for (; p_it && e_it; p_it = next(p_it), e_it = next(e_it)) {
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

			// TODO: handle ellipsis

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

		} else {
			// leave symbol in expansion to be evaluated at runtime
			retcar = expansion->car;
		}

		// TODO: handle ellipsis
	}

	else if (is_pair(expansion->car)) {
		retcar = expand(bindings, get_pair(expansion->car));
	}

	if (is_pair(expansion->cdr)) {
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
