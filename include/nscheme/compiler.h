#ifndef _NSCHEME_COMPILER_H
#define _NSCHEME_COMPILER_H 1

#include <nscheme/vm.h>
#include <nscheme/vm_ops.h>

enum {
	INSTR_NONE,
	INSTR_DO_CALL,
	INSTR_DO_TAILCALL,
	INSTR_JUMP_IF_FALSE,
	INSTR_JUMP,
	INSTR_PUSH_CONSTANT,
	INSTR_CLOSURE_REF,
	INSTR_STACK_REF,
	INSTR_RETURN,
};

enum {
	SCOPE_PARAMETER,
	SCOPE_MUTABLE_PARAMETER,

	SCOPE_CLOSURE,

	SCOPE_LOCAL,
	SCOPE_MUTABLE_LOCAL,
};

typedef struct closure_node {
	scm_value_t sym;
	struct closure_node *next;
	env_node_t *var_ref;
} closure_node_t;

typedef struct instr_node {
	unsigned instr;
	uintptr_t op;

	struct instr_node *prev;
	struct instr_node *next;
} instr_node_t;

typedef struct comp_state {
	scm_closure_t  *closure;
	closure_node_t *closed_vars;
	instr_node_t   *instrs;
	instr_node_t   *last_instr;

	unsigned stack_ptr;
	unsigned closure_ptr;
	unsigned instr_ptr;
} comp_state_t;

typedef struct scope_node {
	struct scope_node *next;
	scm_value_t sym;

	unsigned type;
	unsigned location;
} scope_node_t;

typedef struct scope {
	struct scope *last;
	scope_node_t *nodes;

	unsigned refs;
} scope_t;

typedef struct comp_node {
	struct comp_node *cdr;
	struct comp_node *car;
	scope_t *scope;

	scm_value_t value;
} comp_node_t;

scm_closure_t *vm_compile_closure( vm_t *vm, scm_closure_t *closure );

void gen_scope( comp_node_t *comp,  environment_t *env, scope_t *cur_scope,
				scm_value_t syms, unsigned sp );

#endif
