#include <nscheme/parse.h>
#include <nscheme/vm.h>
#include <nscheme/write.h>

#include <string.h>
#include <stdio.h>

void repl(vm_t *vm, parse_state_t *input) {
	scm_value_t temp = 0;

	while (!is_eof(temp)) {
		printf(" >> ");
		fflush(stdout);

		temp = parse_expression(input);
		temp = vm_evaluate_expr(vm, temp);

		printf(" => ");
		write_value(temp);
		printf("\n");
		fflush(stdout);
	}
}

// TODO: find a better place to put this function
void evaluate_file(vm_t *vm, parse_state_t *input) {
	scm_value_t temp = 0;

	while (!is_eof(temp)) {
		temp = parse_expression(input);
		temp = vm_evaluate_expr(vm, temp);
	}
}

static inline void print_help(void) {
	printf(
	    "usage: nscheme [options] files ...\n"
	    "   -h: print this help and exit\n"
	);

	exit(1);
}

int main(int argc, char *argv[]) {
	parse_state_t *foo;
	vm_t *vm = vm_init();

	if (argc == 1) {
		foo = make_parse_state(stdin);
		repl(vm, foo);

	} else {
		unsigned i = 1;

		for (; i < argc && *argv[i] == '-'; i++) {
			switch (*(argv[i] + 1)) {
			case 'h':
				print_help();
				break;

			default:
				fprintf(stderr, "warning: unknown option %c\n",
				        *(argv[i] + 1));
				break;
			}
		}

		for (; i < argc; i++) {
			FILE *fp = fopen(argv[i], "r");

			foo = make_parse_state(fp);
			evaluate_file(vm, foo);
		}
	}

	vm_free(vm);

	return 0;
}
