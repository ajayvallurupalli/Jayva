#ifndef MACROS_DEFINED
#define MACROS_DEFINED

#include "Macro.h"

/* yeah that's it
 * Currently includes: 
 * 	(Const a b) -> a
 * 	(Sum a) -> a
 * 	(Sum ... a b) -> (Sum ... (+ a b))*/
MacroDict* predefinedMacros();

#endif
