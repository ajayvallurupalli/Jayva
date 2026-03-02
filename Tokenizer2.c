#include "String.h"
#include "Parser2.h"
#include <string.h>
#include "Tokenizer2.h"
#include "LinkedList.h"
#include "Config.h"

void showToken(Token t) {
	switch (t.type) {
		case StringToken:
			printf("[String: %s]\n", t.value.string);
			break;
		case AtomToken:
			printf("[Atom: %s]\n", t.value.atom);
			break;
		case NumberToken:
			printf("[Number: %ld]\n", t.value.number);
			break;
		case SymbolToken:
			printf("[Symbol: %s]\n", t.value.symbol);
			break;
		case TraitToken: 
			printf("[Trait: %s]\n", t.value.trait);
			break;
		case ParenthesisToken:
			printf("[Parenthesis: %d]\n", t.value.parenthesis);
			break;
	}
}


void showTokenVoid(void* willBeToken) {
	if (!willBeToken) {
		printf("[Token:NULL]\n");
	} else showToken(*(Token*) willBeToken); 
}

void freeToken(void* willBeToken) {
	Token* token = (Token*) willBeToken;
	printf("Freeing: ");
	showToken(*token);
	switch (token->type) {
		case NumberToken:
		case ParenthesisToken:
			break;
		case StringToken:
			if (token->value.string) free(token->value.string);
			break;
		case AtomToken:
			if (token->value.atom) free(token->value.atom);
			break;
		case SymbolToken:
			if (token->value.symbol) free(token->value.symbol);
			break;
		case TraitToken:
			if (token->value.trait) free(token->value.trait);
			break;
	}
	if (token) free((Token*) willBeToken);
	willBeToken = NULL;
}

void* parseParenthesis(void* _state, String* string) {
	Token* token = malloc(sizeof(Token));
	if (charAtString(*string, 0) == '(') token->value.parenthesis = string->length;
	else token->value.parenthesis = -(string->length);
	token->type = ParenthesisToken;

	return (void*) token;
}

ParserIndex pureParenthesis(ParserEnvironment* env) {
	return registerParser(env, mkPure(&parseParenthesis));
}

void* parseStringOrAtomOrTraitOrSymbol(void* _state, String* string) {
	Token* token = malloc(sizeof(Token));
	char   first = charAtString(*string, 0);
	if (first == '\"') {
		token->type = StringToken;
		token->value.string = malloc(sizeof(char) * string->length); /* no need for +1 cuz we are moving up one char already*/
		strcpy(token->value.string, string->data + 1);
	} else if (first == ':') {
		token->type = TraitToken;
		token->value.trait = malloc(sizeof(char) * string->length);
		strcpy(token->value.trait, string->data + 1);
	} else if (first == '\'') {
		token->type = AtomToken;
		token->value.atom = malloc(sizeof(char) * string->length);
		strcpy(token->value.atom, string->data + 1);
	} else {
		token->type = SymbolToken;
		token->value.symbol = malloc(sizeof(char) * (string->length + 1));
		strcpy(token->value.symbol, string->data);
	}
	
	return (void*) token;
}

ParserIndex pureStringOrAtomOrTraitOrSymbol(ParserEnvironment* env) {
	return registerParser(env, mkPure(&parseStringOrAtomOrTraitOrSymbol));
}
void* parseNumber(void* _state, String* string) {
        Token* result = malloc(sizeof(Token));
	int    digit  = 0;

	result->type = NumberToken;
	result->value.number = 0;
        
        for (int i = 0; i < getLength(string); i++) {
                digit = (int) string->data[i] - (int) '0';
                if (digit > 9) { /* not a number */
                        break;
                } else {
                        result->value.number = result->value.number * 10L;
                        result->value.number = result->value.number + (long) digit;
                }
	}
	
        return (void*) result;
}

ParserIndex pureNumber(ParserEnvironment* env) {
	return registerParser(env, mkPure(&parseNumber));
}

/* should only terminate on " that is not an escape character*/
int stringPredicate(ParserEnvironment* env, void* _context, char ch) {
	if (ch == '\"') {
		return getNextCharacter(env, -1) == '\\'; /*can take quote if it is escaped*/
	} else {
		return TRUE;
	}
}


ParserIndex satisfyString(ParserEnvironment* env, ParserIndex success, ParserIndex failure) {
	return registerParser(env, mkSatisfy(TRUE, TRUE, &stringPredicate, NULL, success, failure));
}

int atomOrTraitOrSymbolPredicate(ParserEnvironment* env, void* _context, char ch) {
	return (ch != '(' && ch != ')' && ch != ' ' && ch != '\n' && ch != '\"');
}

ParserIndex satisfyAtomOrTraitOrSymbol(ParserEnvironment* env, ParserIndex success, ParserIndex failure) {
	return registerParser(env, mkSatisfy(TRUE, TRUE, &atomOrTraitOrSymbolPredicate, NULL, success, failure));
}

int isSpaceOrNewline(ParserEnvironment* _env, void* _context, char ch) {
	return ch == ' ' || ch == '\n';
}

ParserIndex spaceOrNewline(ParserEnvironment* env, ParserIndex success, ParserIndex failure) {
	return registerParser(env, mkSatisfy(FALSE, TRUE, &isSpaceOrNewline, NULL, success, failure));
}

ParseSequenceResult tokenize(char* words) {
	ParserEnvironment env  = mkParserEnvironment();

	ParserIndex defaultErr = registerParser(&env, mkErr("(IT): Invalid Token"));
	
	ParserIndex  paren      = pureParenthesis(&env);
	ParserIndex  parenL     = singleCharWithoutError(&env, '(', TRUE, FALSE, paren, defaultErr);
	ParserIndex  parenR     = singleCharWithoutError(&env, ')', TRUE, FALSE, paren, defaultErr);

	ParserIndex  pureTok    = pureStringOrAtomOrTraitOrSymbol(&env); 
	ParserIndex  lastQuote  = singleCharWithoutError(&env, '\"', FALSE, FALSE, pureTok, defaultErr); 
	ParserIndex  stringIn   = satisfyString(&env, lastQuote, defaultErr);
	ParserIndex  string     = singleCharWithoutError(&env, '\"', TRUE, FALSE, stringIn, defaultErr);

	ParserIndex  atmSymTrt  = satisfyAtomOrTraitOrSymbol(&env, pureTok, defaultErr);
	ParserIndex  atom       = singleCharWithoutError(&env, '\'', TRUE, FALSE, atmSymTrt, defaultErr);

	ParserIndex  pureNum    = pureNumber(&env);
	ParserIndex  number     = numbersWithoutError(&env, TRUE, pureNum, defaultErr);

	ParserIndex  trait      = singleCharWithoutError(&env, ':', TRUE, FALSE, atmSymTrt, defaultErr); 

	ParserIndex  skip       = registerParser(&env, mkReset(trait + 3, NULL));
	ParserIndex  space      = spaceOrNewline(&env, skip, defaultErr);;

	ParserIndex all[8]      = {space, parenL, string, atom, number, trait, atmSymTrt, parenR};
	ParserIndex lisp        = registerParser(&env, mkAltList(all, 8));

	showParsers(&env);

	ParseSequenceResult tokens = runParserUntil(&env, lisp, words, 0, 1);
	
	#ifdef PARSER_STATISTICS
	showParserStatistics(env.statistics);
	#endif

	#ifdef DEBUG
	showParsingSequenceResult(tokens, &showTokenVoid);
	#endif

	freeParserEnvironment(&env);
	
	return tokens;
}
