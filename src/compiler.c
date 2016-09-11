#include <nscheme/compiler.h>
#include <nscheme/env.h>

extern void debug_print( scm_value_t value );

typedef struct closure_node {
	scm_value_t sym;
	struct closure_node *next;
	env_node_t *var_ref;
} closure_node_t;

typedef struct comp_state {
	scm_closure_t  *closure;
	closure_node_t *closed_vars;

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
                                         env_node_t *node,
                                         scm_value_t sym )
{
	unsigned ret = state->closure_ptr;

	closure_node_t *new_node = calloc( 1, sizeof( closure_node_t ));

	new_node->sym = sym;
	new_node->next = state->closed_vars;
	new_node->var_ref = node;

	state->closed_vars = new_node;
	state->closure_ptr++;

	return ret;
}

static inline void recurse_code( comp_state_t *state,
                                 scm_value_t code,
                                 scm_value_t args )
{
	while ( is_pair( code )){
		scm_pair_t *pair = get_pair( code );

		if ( is_pair( pair->car )){
			unsigned sp = state->stack_ptr;

			printf( "    | starting call,"
					"                   "
					"starting sp: %u\n", sp );
			recurse_code( state, pair->car, args );
			printf( "    | applying call,"
					"                   "
					"setting sp to %u\n", sp + 1 );

			state->stack_ptr = sp + 1;

		} else {
			printf( "    | got a value,  " );

			if ( is_symbol( pair->car )){
				int lookup = list_index( args, pair->car );

				if ( lookup >= 0 ){
					printf( "p   parameter %d  : ", lookup );

				} else {
					env_node_t *var = env_find_recurse( state->closure->env, pair->car );

					if ( var ){
						if ( is_special_form( var->value )){
							printf( "s special form %u : ", get_run_type( var->value ));

						} else {
							unsigned n = add_closure_node( state, var, pair->car );
							printf( "c closure ref %u  : ", n );
						}

					} else {
						printf( "not found (TODO: error), " );
					}
				}
			} else {
				printf( "                   " );
			}

			debug_print( pair->car );
			printf( "\n" );

			state->stack_ptr++;
		}

		code = pair->cdr;
	}
}

scm_closure_t *vm_compile_closure( vm_t *vm, scm_closure_t *closure ){
	scm_closure_t *ret = NULL;
	comp_state_t state;

	printf( "    + compiling closure at %p\n", closure );
	printf( "    | closure args: (%u) ", list_length( closure->args ));
	debug_print( closure->args );
	printf( "\n" );

	state.closure = closure;
	state.closed_vars = NULL;
	state.stack_ptr = list_length( closure->args );

	recurse_code( &state, closure->definition, closure->args );

	printf( "    | returning from closure\n" );

	printf( "    | - closure ptr: %u\n", state.closure_ptr );
	for ( closure_node_t *temp = state.closed_vars; temp; ){
		closure_node_t *next = temp->next;

		printf( "    | - closure ref: %p : %s\n",
			temp->var_ref, get_symbol( temp->sym ));

		free( temp );
		temp = next;
	}

	printf( "    + done\n" );

	return ret;
}
