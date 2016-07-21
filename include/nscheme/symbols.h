#ifndef _NSCHEME_SYMBOLS_H
#define _NSCHEME_SYMBOLS_H 1
#include <nscheme/values.h>
#include <stdbool.h>

const char *store_symbol( const char *symbol );
const char *lookup_symbol_address( const char *symbol );

#endif
