#ifndef _NSCHEME_WRITE_H
#define _NSCHEME_WRITE_H 1

#include <nscheme/values.h>

void write_list(scm_pair_t *pair);
void write_value(scm_value_t value);

#endif
