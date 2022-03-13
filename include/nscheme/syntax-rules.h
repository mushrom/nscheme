#ifndef _NSCHEME_SYNTAX_RULES_H
#define _NSCHEME_SYNTAX_RULES_H 1

#include <nscheme/values.h>
#include <nscheme/vm.h>

scm_value_t expand_syntax_rules(vm_t *vm,
                                scm_syntax_rules_t *rules,
                                scm_pair_t *expr);
// TODO: more syntax, macros

#endif
