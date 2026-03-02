#ifndef TokenizerIncluded
#define TokenizerIncluded

#include "String.h"
#include "Parser2.h"
#include <string.h>
#include "Config.h"

typedef enum {
	StringToken,
	AtomToken,
	NumberToken,
	SymbolToken,
	TraitToken
} TokenType;

typedef struct {
	TokenType type;
	int  parenthesisLevel; 	
	union {
		char*   string;
		char*   atom;
		char*   trait;
		long    number;
		char*   symbol;
		
	} value;
} Token;

void showToken(Token t);
void showTokenVoid(void* willBeToken);

typedef struct {
	int parenthesisLevel;
} ParserState;

#endif
