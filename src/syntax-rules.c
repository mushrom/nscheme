#include <nscheme/syntax-rules.h>
#include <nscheme/vm_ops.h>
#include <nscheme/write.h>

static inline scm_pair_t *next(scm_pair_t *pair) {
    return is_pair(pair->cdr)? get_pair(pair->cdr) : NULL;
}

bool symbol_in_list(scm_pair_t *keywords, const char *symbol) {
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

			if (keywords && symbol_in_list(keywords, p_symbol)) {
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

static inline
scm_value_t *expand(scm_pair_t *keywords,
                    scm_pair_t *pattern,
                    scm_pair_t *expr)
{
	return NULL;
}

scm_value_t expand_syntax_rules(vm_t *vm,
                                scm_syntax_rules_t *rules,
                                scm_pair_t *expr)
{
	scm_pair_t *it = rules->patterns;

	bool found = false;
	for (; it; it = next(it)) {
		scm_pair_t *p = get_pair(it->car);

		// TODO: need to check that syntax-rules has a valid form when defining patterns
		scm_pair_t *pattern = get_pair(p->car);
		//scm_pair_t *expand  = get_pair(p->cdr);

		if (matches(rules->keywords, pattern, expr)) {
			puts("have matching pattern:");
			write_list(pattern); putchar('\n');

			puts("for expression:");
			write_list(expr); putchar('\n');

			// TODO: expand
			found = true;
			break;
		}
	}

	if (!found) {
		vm_error(vm, "no matching syntax definition");
	}

	vm_stack_push(vm, vm_func_return_last());
	vm_stack_push(vm, SCM_TYPE_NULL);
    return SCM_TYPE_NULL;
}
