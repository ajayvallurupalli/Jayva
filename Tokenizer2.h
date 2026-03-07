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
	TraitToken,
	ParenthesisToken
} TokenType;

typedef struct {
	TokenType type;
	union {
		char*   string;
		char*   atom;
		char*   trait;
		long    number;
		char*   symbol;
		int     parenthesis;
	} value;
} Token;

void showToken(const Token t);
void showTokenVoid(const void* willBeToken);
void freeToken(void* willBeToken);

/* if it succeeds, the success value will be a linkedList with the tokens*/
ParseSequenceResult tokenize(char* words);

#endif
