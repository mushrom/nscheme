#include <nscheme/vm.h>
#include <nscheme/vm_ops.h>
#include <nscheme/env.h>
#include <nscheme/syntax-rules.h>
#include <stdlib.h>
#include <stdio.h>

static inline void vm_step_compiled(vm_t *vm) {
	vm_op_t *code = vm->closure->code + vm->ip;

	vm->ip += code->func(vm, code->arg);
}

static scm_closure_t *vm_make_closure(scm_value_t args,
                                      scm_value_t body,
                                      environment_t *env)
{
	scm_closure_t *ret = calloc(1, sizeof(scm_closure_t));

	ret->definition  = body;
	ret->args        = args;
	ret->env         = env;
	ret->compiled    = false;

	return ret;
}

static bool is_valid_lambda(scm_pair_t *pair) {
	return (is_pair(pair->car) || is_null(pair->car))
	       && is_pair(pair->cdr);
}

static inline void vm_handle_lambda(vm_t *vm,
                                    scm_value_t form,
                                    scm_value_t expr)
{
	if (!is_pair(expr)) {
		puts("errrrorr! in lambda expansion");
	}

	scm_pair_t *pair = get_pair(expr);

	if (is_valid_lambda(pair)) {
		scm_value_t args = pair->car;
		scm_value_t body = pair->cdr;
		scm_closure_t *tmp = vm_make_closure(args, body, vm->env);

		vm_stack_push(vm, tag_closure(tmp));
		vm_call_return(vm);

	} else {
		puts("an error! TODO: better error reporting");
	}
}

static inline void vm_handle_define(vm_t *vm,
                                    scm_value_t form,
                                    scm_value_t expr,
                                    unsigned type)
{
	if (!is_pair(expr)) {
		puts("error in define!");
	}

	scm_pair_t *pair = get_pair(expr);

	vm_stack_push(vm,
	              (type == RUN_TYPE_SET)
	              ? vm_func_intern_set()
	              : vm_func_intern_define());

	if (is_symbol(pair->car)) {
		vm_stack_push(vm, pair->car);
		vm->ptr = pair->cdr;

	} else if (is_pair(pair->car)) {
		scm_pair_t *temp = get_pair(pair->car);
		scm_closure_t *clsr =
		    vm_make_closure(temp->cdr, pair->cdr, vm->env);

		vm_stack_push(vm, temp->car);
		vm_stack_push(vm, tag_closure(clsr));

		vm->ptr = SCM_TYPE_NULL;
	}
}

static inline void vm_handle_if(vm_t *vm,
                                scm_value_t form,
                                scm_value_t expr)
{
	if (!is_pair(expr)) {
		puts("error in if!");
		vm_error(vm, "Expected pair");
	}

	scm_pair_t *pair = get_pair(expr);

	vm_stack_push(vm, tag_run_type(RUN_TYPE_SET_PTR));
	vm->ptr = SCM_TYPE_NULL;

	vm_call_eval(vm, SCM_TYPE_NULL);
	vm_stack_push(vm, vm_func_intern_if());

	scm_value_t test = pair->car;

	if (!is_pair(pair->cdr)) {
		vm_error(vm, "Expected pair (first path)");
		return;
	}

	pair = get_pair(pair->cdr);

	vm_stack_push(vm, pair->car);

	if (!is_pair(pair->cdr)) {
		vm_error(vm, "Expected pair (second path)");
		return;
	}

	pair = get_pair(pair->cdr);
	vm_stack_push(vm, pair->car);

	vm_call_eval(vm, test);
}

static inline void vm_handle_begin(vm_t *vm,
                                   scm_value_t form,
                                   scm_value_t expr)
{
	vm_stack_push(vm, vm_func_return_last());
	vm->ptr = expr;
}

static inline void vm_handle_quote(vm_t *vm,
                                   scm_value_t form,
                                   scm_value_t expr)
{
	vm_stack_push(vm, vm_func_return_last());
	vm_stack_push(vm, scm_car(expr));
	vm->ptr = SCM_TYPE_NULL;
}

static inline void vm_handle_syntax_rules(vm_t *vm,
                                          scm_value_t form,
                                          scm_value_t expr)
{
	scm_syntax_rules_t *ret = calloc(1, sizeof(scm_syntax_rules_t));

	// TODO: error checking
	scm_pair_t *pair = get_pair(expr);
	ret->keywords = is_null(pair->car)? NULL : get_pair(pair->car);

	pair = get_pair(pair->cdr);
	ret->patterns = pair;

	vm_stack_push(vm, vm_func_return_last());
	vm_stack_push(vm, tag_heap_type(ret, SCM_TYPE_SYNTAX_RULES));
	vm->ptr = SCM_TYPE_NULL;
}

static void vm_handle_sform(vm_t *vm, scm_value_t form, scm_value_t expr) {
	unsigned type = get_run_type(form);

	switch (type) {
		case RUN_TYPE_LAMBDA:
			vm_handle_lambda(vm, form, expr);
			break;

		case RUN_TYPE_DEFINE:
		case RUN_TYPE_SET:
		case RUN_TYPE_DEFINE_SYNTAX:
			vm_handle_define(vm, form, expr, type);
			break;

		case RUN_TYPE_IF:
			vm_handle_if(vm, form, expr);
			break;

		case RUN_TYPE_BEGIN:
			vm_handle_begin(vm, form, expr);
			break;

		case RUN_TYPE_QUOTE:
			vm_handle_quote(vm, form, expr);
			break;

		case RUN_TYPE_SYNTAX_RULES:
			vm_handle_syntax_rules(vm, form, expr);
			break;

		default:
			printf("unknown type in special form evaluation\n");
			break;
	}
}

static inline void vm_step_interpreter(vm_t *vm) {
	if (is_pair(vm->ptr)) {
		scm_pair_t *pair = get_pair(vm->ptr);

		vm->ptr = pair->cdr;

		if (is_pair(pair->car)) {
			vm_call_eval(vm, pair->car);

		} else if (is_symbol(pair->car)) {
			env_node_t *foo = env_find_recurse(vm->env, pair->car);

			if (foo) {
				if (vm->argnum == 0) {
					// only try to expand special forms and macros
					// if this is the first element in the list
					if (is_special_form(foo->value)) {
						vm_handle_sform(vm, foo->value, pair->cdr);
						puts("have sform");

					} else if (is_syntax_rules(foo->value)) {
						scm_syntax_rules_t *rules = get_syntax_rules(foo->value);
						puts("have syntax rules, expanding");
						vm_stack_push(vm, vm_func_return_last());
						vm_stack_push(vm, SCM_TYPE_NULL);
						vm->ptr = SCM_TYPE_NULL;
						vm_call_eval(vm, expand_syntax_rules(vm, rules, pair));

					} else {
						vm_stack_push(vm, foo->value);
					}

				} else {
					// otherwise just push the underlying value
					vm_stack_push(vm, foo->value);
				}

			} else {
				// TODO: pass current expression to error output
				printf("    symbol '%s' not found\n", get_symbol(pair->car));
				vm_error(vm, "undefined symbol");
				//vm_stack_push(vm, pair->car);
			}

		} else {
			vm_stack_push(vm, pair->car);
		}

	} else if (is_null(vm->ptr)) {
		vm_call_apply(vm);

	} else {
		scm_value_t value = vm->ptr;

		// TODO: there's similar lookup code above, possibly make a function
		//       that handles both cases here, like
		//       'vm_resolve_atom' or something
		if (is_symbol(vm->ptr)) {
			env_node_t *foo = env_find_recurse(vm->env, vm->ptr);

			if (foo) {
				value = foo->value;

			} else {
				printf("    symbol '%s' not found\n", get_symbol(vm->ptr));
				vm_error(vm, "undefined symbol");
			}
		}

		vm->stack[vm->sp] = value;
		vm_call_return(vm);
	}
}

void vm_run(vm_t *vm) {
	//void (*stepfuncs[2])(vm_t *) = { step_vm_interpreter, step_vm_compiled };

	while (vm->running) {
		//if ( vm->closure->is_compiled ){
		if (vm->runmode == RUN_MODE_COMPILED) {
			vm_step_compiled(vm);
		} else {
			vm_step_interpreter(vm);
		}

		// TODO: do some benchmarks to see if indexing into an array of function
		//       pointers is faster than branching, once things are working
		//stepfuncs[vm->closure->is_compiled]( vm );
	}
}

void vm_error(vm_t *vm, const char *msg) {
	vm->running = false;
	vm->errormsg = msg;
	// TODO: current line number, some way to attach other context
	//       information to the code
}

void  vm_clear_error(vm_t *vm) {
	vm->running = true;
	vm->errormsg = NULL;
}

static scm_closure_t *root_closure = NULL;

static environment_t *vm_r7rs_environment(void) {
	static environment_t *ret = NULL;

	if (!ret) {
		ret = env_create(NULL);
	}

	return ret;
}

/*
 * This function assumes that the VM has stack pointers that are all zero
 */
scm_value_t vm_evaluate_expr(vm_t *vm, scm_value_t expr) {
	vm->ptr = expr;
	vm->running = true;
	vm->closure = root_closure;
	vm->closure->definition = expr;
	vm->argnum = 0;
	vm->env = vm_r7rs_environment();
	vm->runmode = RUN_MODE_INTERP;

	vm_run(vm);

	return vm->stack[0];
}

#include <nscheme/symbols.h>
#include <string.h>

static void vm_add_arithmetic_op(vm_t *vm, char *name, vm_func func) {
	scm_closure_t *meh = calloc(1, sizeof(scm_closure_t));
	meh->code = calloc(1, sizeof(vm_op_t[2]));

	meh->code[0].func = func;
	meh->code[1].func = vm_op_return;
	meh->compiled = true;

	// TODO: find some place to put environment init stuff
	scm_value_t foo  = tag_symbol(store_symbol(strdup(name)));
	scm_value_t clsr = tag_closure(meh);
	env_set(vm->env, foo, clsr);
}

void vm_handles_init(vm_handle_stack_t *stack, size_t initial_size) {
	stack->slots = calloc(1, sizeof(vm_handle_t[initial_size]));
	stack->avail = calloc(1, sizeof(int[initial_size]));
	stack->num_avail = initial_size;
	stack->max_avail = initial_size;
	// TODO: check allocations, error

	for (int i = 0; i < initial_size; i++) {
		stack->avail[i] = i;
	}
}

int vm_handle_alloc(vm_t *vm) {
	if (vm->handles.num_avail == 0) {
		// TODO: reallocate
		return -1;
	}

	int ret = --vm->handles.num_avail;
	if (vm->handles.slots[ret].used) {
		// TODO: fatal error, double allocation
		//       (probably the result of a double free)
		return -1;
	}

	vm->handles.slots[ret].used = true;
	return ret;
}

void vm_handle_free(vm_t *vm, int handle) {
	if (vm->handles.num_avail >= vm->handles.max_avail) {
		// TODO: fatal error, should never happen
		return;
	}

	if (handle < 0 || handle >= vm->handles.max_avail) {
		// TODO: fatal error, invalid handle
		return;
	}

	if (vm->handles.slots[handle].used == false) {
		// TODO: fatal error, double free
		return;
	}

	vm->handles.slots[handle].used = true;
	vm->handles.avail[vm->handles.num_avail++] = handle;
}

bool vm_handle_valid(vm_t *vm, int handle) {
	return handle >= 0
		&& handle < vm->handles.max_avail
		&& vm->handles.slots[handle].used;
}

scm_value_t vm_handle_get(vm_t *vm, int handle) {
	if (vm_handle_valid(vm, handle)) {
		return vm->handles.slots[handle].value;

	} else {
		// TODO: error? probably
		return SCM_TYPE_NULL;
	}
}

void vm_handle_set(vm_t *vm, int handle, scm_value_t value) {
	if (vm_handle_valid(vm, handle)) {
		vm->handles.slots[handle].value = value;

	} else {
		// TODO: error
	}
}

vm_t *vm_init(void) {
	vm_t *ret = calloc(1, sizeof(vm_t));
	gc_init(&ret->gc, 0x8000);
	vm_handles_init(&ret->handles, 0x1000);

	//scm_closure_t *root_closure = calloc( 1, sizeof( scm_closure_t ));
	root_closure = calloc(1, sizeof(scm_closure_t));

	ret->stack_size = 0x1000;
	ret->calls_size = 0x1000;
	ret->stack = calloc(1, sizeof(scm_value_t[ret->stack_size]));
	ret->calls = calloc(1, sizeof(vm_callframe_t[ret->calls_size]));
	ret->closure = root_closure;
	ret->env = vm_r7rs_environment();

	// TODO: find some place to put environment init stuff

	vm_add_arithmetic_op(ret, "+", vm_op_add);
	vm_add_arithmetic_op(ret, "-", vm_op_sub);
	vm_add_arithmetic_op(ret, "*", vm_op_mul);
	vm_add_arithmetic_op(ret, "/", vm_op_div);

	vm_add_arithmetic_op(ret, "eq?", vm_op_equal);
	vm_add_arithmetic_op(ret, "<", vm_op_lessthan);
	vm_add_arithmetic_op(ret, ">", vm_op_greaterthan);
	vm_add_arithmetic_op(ret, "null?", vm_op_is_null);
	vm_add_arithmetic_op(ret, "pair?", vm_op_is_pair);

	vm_add_arithmetic_op(ret, "display", vm_op_display);
	vm_add_arithmetic_op(ret, "newline", vm_op_newline);
	vm_add_arithmetic_op(ret, "read", vm_op_read);

	vm_add_arithmetic_op(ret, "cons", vm_op_cons);
	vm_add_arithmetic_op(ret, "car", vm_op_car);
	vm_add_arithmetic_op(ret, "cdr", vm_op_cdr);

	scm_value_t foo;

	foo = tag_symbol(store_symbol(strdup("lambda")));
	env_set(ret->env, foo, tag_run_type(RUN_TYPE_LAMBDA));

	foo = tag_symbol(store_symbol(strdup("define")));
	env_set(ret->env, foo, tag_run_type(RUN_TYPE_DEFINE));

	foo = tag_symbol(store_symbol(strdup("define-syntax")));
	env_set(ret->env, foo, tag_run_type(RUN_TYPE_DEFINE_SYNTAX));

	foo = tag_symbol(store_symbol(strdup("set!")));
	env_set(ret->env, foo, tag_run_type(RUN_TYPE_SET));

	foo = tag_symbol(store_symbol(strdup("if")));
	env_set(ret->env, foo, tag_run_type(RUN_TYPE_IF));

	foo = tag_symbol(store_symbol(strdup("begin")));
	env_set(ret->env, foo, tag_run_type(RUN_TYPE_BEGIN));

	foo = tag_symbol(store_symbol(strdup("quote")));
	env_set(ret->env, foo, tag_run_type(RUN_TYPE_QUOTE));

	foo = tag_symbol(store_symbol(strdup("syntax-rules")));
	env_set(ret->env, foo, tag_run_type(RUN_TYPE_SYNTAX_RULES));

	return ret;
}

void  vm_free(vm_t *vm) {
	if (vm) {
		free(vm->stack);
		free(vm->calls);
		free(vm);
	}
}
