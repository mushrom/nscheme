#include <nscheme/compiler.h>
#include <nscheme/env.h>
#include <string.h>

extern void debug_print( scm_value_t value );

enum {
	INSTR_NONE,
	INSTR_DO_CALL,
	INSTR_JUMP_IF_FALSE,
	INSTR_JUMP,
	INSTR_PUSH_CONSTANT,
	INSTR_CLOSURE_REF,
	INSTR_STACK_REF,
	INSTR_RETURN,
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

static inline unsigned list_length( scm_value_t value ){
	unsigned length = 0;

	if ( is_pair( value )){
		scm_pair_t *pair = get_pair( value );
		length = 1;

		while ( is_pair( pair->cdr )){
			length++;
			pair = get_pair( pair->cdr );
		}
	}

	return length;
}

static inline int list_index( scm_value_t value, scm_value_t target ){
	int index = -1;
	int temp  = 0;

	while ( is_pair( value )){
		scm_pair_t *pair = get_pair( value );

		if ( pair->car == target ){
			index = temp;
			break;
		}

		temp++;
		value = pair->cdr;
	}

	return index;
}

static inline unsigned add_closure_node( comp_state_t *state,
                                         env_node_t   *node,
                                         scm_value_t  sym )
{
	unsigned ret = state->closure_ptr;

	closure_node_t *new_node = calloc( 1, sizeof( closure_node_t ));

	// TODO: search through closure list before adding a new node
	//       to prevent having duplicate entries in the list
	new_node->sym = sym;
	new_node->next = state->closed_vars;
	new_node->var_ref = node;

	state->closed_vars = new_node;
	state->closure_ptr++;

	return ret;
}

static inline instr_node_t *add_instr_node( comp_state_t *state,
                                            unsigned     instruction,
                                            uintptr_t    argument )
{
	instr_node_t *node = calloc( 1, sizeof(instr_node_t));

	node->instr = instruction;
	node->op    = argument;

	if ( state->last_instr ){
		node->prev = state->last_instr;
		state->last_instr->next = node;
	}

	if ( !state->instrs ){
		state->instrs = node;
	}

	state->last_instr = node;
	state->instr_ptr++;

	return node;
}

static inline void compile_value( comp_state_t *state,
                                  scm_value_t args,
                                  scm_value_t value )
{
	printf( "    | got a value,  " );

	if ( is_symbol( value )){
		int lookup = list_index( args, value );

		if ( lookup >= 0 ){
			// add one to account for closure on stack before arguments
			lookup += 1;
			printf( "p   parameter %d  : ", lookup );
			add_instr_node( state, INSTR_STACK_REF, lookup );

		} else {
			env_node_t *var = env_find_recurse( state->closure->env, value );

			if ( var ){
				if ( is_special_form( var->value )){
					printf( "s special form %u : ", get_run_type( var->value ));

				} else {
					unsigned n = add_closure_node( state, var, value );
					printf( "c closure ref %u  : ", n );
					add_instr_node( state, INSTR_CLOSURE_REF, n );
				}

			} else {
				printf( "not found (TODO: error), " );
			}
		}

	} else {
		printf( "                   " );
		add_instr_node( state, INSTR_PUSH_CONSTANT, value );
	}

	debug_print( value );
	printf( "\n" );

	state->stack_ptr++;
}

static inline bool is_if_token( environment_t *env, scm_value_t sym ){
	bool ret = false;

	if ( is_symbol( sym )){
		env_node_t *var = env_find_recurse( env, sym );

		ret = var && var->value == tag_run_type( RUN_TYPE_IF );
	}

	return ret;
}

static inline void compile_expression_list( comp_state_t *state,
                                            scm_value_t code,
                                            scm_value_t args );

static inline scm_value_t scm_car( scm_value_t value ){
	scm_value_t ret = SCM_TYPE_NULL;

	if ( is_pair( value )){
		scm_pair_t *pair = get_pair( value );
		ret = pair->car;
	}

	return ret;
}

static inline scm_value_t scm_cdr( scm_value_t value ){
	scm_value_t ret = SCM_TYPE_NULL;

	if ( is_pair( value )){
		scm_pair_t *pair = get_pair( value );
		ret = pair->cdr;
	}

	return ret;
}

static inline void compile_if_expression( comp_state_t *state,
                                          scm_value_t code,
                                          scm_value_t args )
{
	printf( "    | compiling if expression...\n" );

	scm_pair_t codebuf_pair = {
		.car = scm_car( scm_cdr( code )),
		.cdr = SCM_TYPE_NULL,
	};

	scm_value_t codebuf = tag_pair( &codebuf_pair );

	compile_expression_list( state, codebuf, args );

	instr_node_t *false_jump = add_instr_node( state, INSTR_JUMP_IF_FALSE, 0 );
	codebuf_pair.car = scm_car( scm_cdr( scm_cdr( code )));

	compile_expression_list( state, codebuf, args );

	instr_node_t *end_jump = add_instr_node( state, INSTR_JUMP, 0 );
	unsigned second_expr = state->instr_ptr;

	//codebuf_pair.car = scm_car( scm_cdr( scm_cdr( scm_cdr( code ))));
	codebuf_pair.car = scm_car( scm_cdr( scm_cdr( scm_cdr( code ))));

	compile_expression_list( state, codebuf, args );

	unsigned if_end = state->instr_ptr;

	false_jump->op = second_expr;
	end_jump->op   = if_end;
}

static inline void compile_expression_list( comp_state_t *state,
                                            scm_value_t code,
                                            scm_value_t args )
{
	while ( is_pair( code )){
		scm_pair_t *pair = get_pair( code );

		if ( is_pair( pair->car )){
			unsigned sp = state->stack_ptr;

			if ( is_if_token( state->closure->env, scm_car( pair->car ))){
				compile_if_expression( state, pair->car, args );

			} else {

				printf( "    | starting call,"
						"                   "
						"starting sp: %u\n", sp );

				compile_expression_list( state, pair->car, args );

				add_instr_node( state, INSTR_DO_CALL, sp );

				printf( "    | applying call,"
						"                   "
						"setting sp to %u\n", sp + 1 );
			}

			state->stack_ptr = sp + 1;

		} else {
			compile_value( state, args, pair->car );
		}

		code = pair->cdr;
	}
}

static inline void store_closed_vars( comp_state_t *state,
                                      scm_closure_t *closure )
{
	printf( "    | - closure ptr: %u\n", state->closure_ptr );

	closure->closures = calloc( 1, sizeof( env_node_t *[state->closure_ptr] ));

	closure_node_t *temp = state->closed_vars;
	unsigned i = state->closure_ptr - 1;

	while ( temp ){
		closure_node_t *next = temp->next;

		printf( "    | - closure ref: %p : %s\n",
			temp->var_ref, get_symbol( temp->sym ));

		closure->closures[i--] = temp->var_ref;

		free( temp );
		temp = next;
	}
}

static inline void store_instructions( comp_state_t *state,
                                       scm_closure_t *closure )
{
	printf( "    | - instruction ptr: %u\n", state->instr_ptr );

	closure->code = calloc( 1, sizeof( vm_op_t[state->instr_ptr] ));

	unsigned i = 0;
	for ( instr_node_t *node = state->instrs; node; ){
		instr_node_t *next = node->next;
		const char *opnames[] = {
			"none",
			"do_call",
			"jump_if_false",
			"jump",
			"push_const",
			"closure_ref",
			"stack_ref",
			"return",
		};

		vm_func opfuncs[] = {
			NULL,
			vm_op_do_call,
			vm_op_jump_if_false,
			vm_op_jump,
			vm_op_push_const,
			vm_op_closure_ref,
			vm_op_stack_ref,
			vm_op_return_last,
		};

		closure->code[i].func = opfuncs[node->instr];
		closure->code[i].arg  = node->op;

		printf( "    | - instruction %3u: %14s (%p) : %lu\n",
			i++,
			opnames[node->instr],
			opfuncs[node->instr],
			node->op );

		free( node );
		node = next;
	}
}

scm_closure_t *vm_compile_closure( vm_t *vm, scm_closure_t *closure ){
	scm_closure_t *ret = NULL;
	comp_state_t state;

	printf( "    + compiling closure at %p\n", closure );
	printf( "    | closure args: (%u) ", list_length( closure->args ));
	debug_print( closure->args );
	printf( "\n" );

	memset( &state, 0, sizeof(state));
	state.closure = closure;
	state.closed_vars = NULL;
	state.stack_ptr = list_length( closure->args ) + 1;

	compile_expression_list( &state, closure->definition, closure->args );
	add_instr_node( &state, INSTR_RETURN, 0 );

	printf( "    | returning from closure\n" );

	store_closed_vars( &state, closure );
	store_instructions( &state, closure );

	printf( "    + done\n" );

	closure->compiled = true;

	return ret;
}
