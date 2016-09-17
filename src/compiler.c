#include <nscheme/compiler.h>
#include <nscheme/env.h>
#include <string.h>

extern void debug_print( scm_value_t value );

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

unsigned add_closure_node( comp_state_t *state,
                           env_node_t   *node,
                           scm_value_t  sym )
{
	unsigned ret = state->closure_ptr;

	closure_node_t *temp = state->closed_vars;
	unsigned i = ret - 1;

	// search for an already existing entry to prevent
	// duplicates in the closure list
	//
	// TODO: might want to use a more efficient search method
	//       in the future, once large procedures start being
	//       written
	while ( temp ){
		if ( temp->sym == sym ){
			return i;
		}

		i--;
		temp = temp->next;
	}

	temp = calloc( 1, sizeof( closure_node_t ));

	temp->sym = sym;
	temp->next = state->closed_vars;
	temp->var_ref = node;

	state->closed_vars = temp;
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
                                  comp_node_t *comp )
{
	printf( "    | got a value,  " );

	//if ( is_symbol( comp->value )){
	if ( comp->node ){
		unsigned loc = comp->node->location;

		switch ( comp->node->type ){
			case SCOPE_PARAMETER:
				printf( "p   parameter %d  : ", loc );
				add_instr_node( state, INSTR_STACK_REF, loc );
				break;

			case SCOPE_CLOSURE:
				printf( "c closure ref %u  : ", loc );
				add_instr_node( state, INSTR_CLOSURE_REF, loc );
				break;

			default:
				break;
		}

	} else {
		printf( "                   " );
		add_instr_node( state, INSTR_PUSH_CONSTANT, comp->value );
	}

	debug_print( comp->value );
	printf( "\n" );

	state->stack_ptr++;
}

static inline bool is_runtime_token( environment_t *env,
                                     scm_value_t sym,
                                     unsigned type )
{
	bool ret = false;

	if ( is_symbol( sym )){
		env_node_t *var = env_find_recurse( env, sym );

		ret = var && var->value == tag_run_type( type );
	}

	return ret;
}

static inline bool is_if_token( environment_t *env, scm_value_t sym ){
	return is_runtime_token( env, sym, RUN_TYPE_IF );
}

static inline bool is_define_token( environment_t *env, scm_value_t sym ){
	return is_runtime_token( env, sym, RUN_TYPE_DEFINE );
}

static inline void compile_if_expression( comp_state_t *state,
                                          comp_node_t *comp,
                                          bool tail );

static inline void compile_expression_list( comp_state_t *state,
                                            comp_node_t *comp,
                                            bool tail );

static inline void compile_if_expression( comp_state_t *state,
                                          comp_node_t *comp,
                                          bool tail )
{
	printf( "    | compiling if expression (tail call: %u)...\n", tail );

	comp_node_t term = {
		.car = NULL,
		.cdr = NULL,
		.value = SCM_TYPE_NULL,
	};

	// TODO: check that list accesses are correct
	comp_node_t codebuf = {
		.cdr = &term,
		.car = comp->cdr->car,
	};

	compile_expression_list( state, &codebuf, false );

	instr_node_t *false_jump = add_instr_node( state, INSTR_JUMP_IF_FALSE, 0 );
	codebuf.car = comp->cdr->cdr->car;

	compile_expression_list( state, &codebuf, tail );

	instr_node_t *end_jump = add_instr_node( state, INSTR_JUMP, 0 );
	unsigned second_expr = state->instr_ptr;

	codebuf.car = comp->cdr->cdr->cdr->car;

	compile_expression_list( state, &codebuf, tail );

	unsigned if_end = state->instr_ptr;

	false_jump->op = second_expr;
	end_jump->op   = if_end;
}

static inline void compile_expression_list( comp_state_t *state,
                                            comp_node_t *comp,
                                            bool tail )
{
	while ( comp ){
		if ( comp->car && is_pair( comp->car->value )) {
			unsigned sp = state->stack_ptr;

			bool is_tail_call;
			is_tail_call = tail && comp->cdr
			            && comp->cdr->value == SCM_TYPE_NULL;

			if ( is_tail_call ){
				printf( "    | have last expression in list: " );
				debug_print( comp->value );
				printf( "\n" );
			}

			if ( is_if_token( state->closure->env, comp->car->car->value )){
				compile_if_expression( state, comp->car, is_tail_call );

			} else {

				printf( "    | starting call,"
						"                   "
						"starting sp: %u\n", sp );

				compile_expression_list( state, comp->car, false );

				if ( is_tail_call ){
					add_instr_node( state, INSTR_DO_TAILCALL, sp );

					printf( "    | doing tailcall,"
							"                   "
							"setting sp to %u\n", sp + 1 );

				} else {
					add_instr_node( state, INSTR_DO_CALL, sp );

					printf( "    | applying call,"
							"                   "
							"setting sp to %u\n", sp + 1 );
				}
			}

			state->stack_ptr = sp + 1;

		} else if ( comp->car ){
			compile_value( state, comp->car );
		}

		comp = comp->cdr;
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
			"do_tailcall",
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
			vm_op_do_tailcall,
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

static inline comp_node_t *wrap_comp_values( scm_value_t value ){
	comp_node_t *ret = calloc( 1, sizeof( comp_node_t ));

	ret->value = value;

	if ( is_pair( value )){
		scm_pair_t *pair = get_pair( value );

		ret->car = wrap_comp_values( pair->car );
		ret->cdr = wrap_comp_values( pair->cdr );
	}

	return ret;
}

static inline void dump_comp_values( comp_node_t *comp, unsigned level ){
	if ( comp ){
		printf( "    | > %*s node : 0x%lx, scope : %p",
			level * 2, "", comp->value, comp->scope );

		if ( !is_pair( comp->value )){
			printf( " : " );
			debug_print( comp->value );
		}

		printf( "\n" );

		dump_comp_values( comp->car, level + 1 );
		dump_comp_values( comp->cdr, level );
	}
}

static inline void free_comp_values( comp_node_t *comp ){
	if ( comp ){
		free_comp_values( comp->car );
		free_comp_values( comp->cdr );
	}

	free( comp );
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
	state.env = closure->env;
	state.closed_vars = NULL;
	state.stack_ptr = list_length( closure->args ) + 1;

	comp_node_t *values = wrap_comp_values( closure->definition );

	gen_scope( values, &state, NULL, closure->args, 1 );
	dump_comp_values( values, 0 );

	compile_expression_list( &state, values, true );
	add_instr_node( &state, INSTR_RETURN, 0 );

	printf( "    | returning from closure\n" );

	store_closed_vars( &state, closure );
	store_instructions( &state, closure );

	free_comp_values( values );

	printf( "    + done\n" );

	closure->compiled = true;

	return ret;
}
