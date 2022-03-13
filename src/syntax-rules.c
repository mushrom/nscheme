#include <nscheme/syntax-rules.h>
#include <nscheme/vm_ops.h>

static inline
bool matches(scm_pair_t *keywords,
             scm_pair_t *pattern,
             scm_pair_t *expr)
{
    return false;
}

static inline scm_pair_t *next(scm_pair_t *pair) {
    return is_pair(pair->cdr)? get_pair(pair->cdr) : NULL;
}

scm_value_t expand_syntax_rules(vm_t *vm,
                                scm_syntax_rules_t *rules,
                                scm_pair_t *expr)
{
    scm_pair_t *it = rules->patterns;

    for (; it; it = next(it)) {
        if (matches(rules->keywords, it, expr)) {
            // expand
        }
    }

	vm_stack_push(vm, vm_func_return_last());
	vm_stack_push(vm, SCM_TYPE_NULL);
    return SCM_TYPE_NULL;
}
