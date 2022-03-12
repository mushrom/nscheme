#ifndef _NSCHEME_COMPILER_H
#define _NSCHEME_COMPILER_H 1

#include <nscheme/vm.h>
#include <nscheme/vm_ops.h>

// macros for toggling debug output
#ifdef VM_VERBOSE_COMPILE
#define DEBUG_PRINTF( ... )   printf( __VA_ARGS__ );
#define DEBUG_WRITEVAL( ... ) write_value( __VA_ARGS__ );
#else
#define DEBUG_PRINTF( ... )   /* __VA_ARGS__ */
#define DEBUG_WRITEVAL( ... ) /* __VA_ARGS__ */
#endif

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
	environment_t  *env;
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

	// scope info, only used for unquoted symbols
	scope_node_t *node;
} comp_node_t;

scm_closure_t *vm_compile_closure(vm_t *vm, scm_closure_t *closure);

bool gen_top_scope(comp_node_t*, comp_state_t*, scope_t*, scm_value_t, unsigned);
unsigned add_closure_node(comp_state_t *, env_node_t *, scm_value_t);

static inline bool is_runtime_token(environment_t *env,
                                    scm_value_t sym,
                                    unsigned type)
{
	bool ret = false;

	if (is_symbol(sym)) {
		env_node_t *var = env_find_recurse(env, sym);

		ret = var && var->value == tag_run_type(type);
	}

	return ret;
}

static inline bool is_if_token(environment_t *env, scm_value_t sym) {
	return is_runtime_token(env, sym, RUN_TYPE_IF);
}

static inline bool is_define_token(environment_t *env, scm_value_t sym) {
	return is_runtime_token(env, sym, RUN_TYPE_DEFINE);
}

static inline bool is_begin_token(environment_t *env, scm_value_t sym) {
	return is_runtime_token(env, sym, RUN_TYPE_BEGIN);
}

typedef bool (*runtype_checker)(environment_t *env, scm_value_t sym);

static inline bool is_run_statement(environment_t *env,
                                    comp_node_t *node,
                                    runtype_checker is_type)
{
	return node && node->car
	       && is_symbol(node->car->value)
	       && is_type(env, node->car->value);
}

static inline bool is_if_statement(environment_t *env, comp_node_t *node) {
	return is_run_statement(env, node, is_if_token);
}

static inline bool is_define_statement(environment_t *env, comp_node_t *node) {
	return is_run_statement(env, node, is_define_token);
}

static inline bool is_begin_statement(environment_t *env, comp_node_t *node) {
	return is_run_statement(env, node, is_begin_token);
}

#endif
