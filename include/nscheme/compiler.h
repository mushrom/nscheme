#ifndef _NSCHEME_COMPILER_H
#define _NSCHEME_COMPILER_H 1

#include <nscheme/vm.h>
#include <nscheme/vm_ops.h>

scm_closure_t *vm_compile_closure( vm_t *vm, scm_closure_t *closure );

#endif
