#include "String.h"
#include "Parser2.h"
#include <string.h>
#include "Tokenizer.h"
#include "LinkedList.h"
#include "Config.h"

void showToken(Token t) {
	switch (t.type) {
		case StringToken:
			printf("[String: %s {paren: %d}]\n", t.value.string, t.parenthesisLevel);
			break;
		case AtomToken:
			printf("[Atom: %s {paren: %d}]\n", t.value.atom, t.parenthesisLevel);
			break;
		case NumberToken:
			printf("[Number: %ld {paren: %d}]\n", t.value.number, t.parenthesisLevel);
			break;
		case SymbolToken:
			printf("[Symbol: %s {paren: %d}]\n", t.value.symbol, t.parenthesisLevel);
			break;
		case TraitToken: 
			printf("[Trait: %s {paren: %d}]\n", t.value.trait, t.parenthesisLevel);
			break;
	}
}


void showTokenVoid(void* willBeToken) {
	showToken(*(Token*) willBeToken); 
}

void freeToken(void* willBeToken) {
	Token* token = (Token*) willBeToken;
	switch (token->type) {
		case NumberToken:
			break;
		case StringToken:
			free(token->value.string);
			break;
		case AtomToken:
			free(token->value.atom);
			break;
		case SymbolToken:
			free(token->value.symbol);
			break;
		case TraitToken:
			free(token->value.trait);
			break;
	}
	free((Token*) willBeToken);
}

void showParserState(ParserState* state) {
	printf("[ParserState: {parenthesisLevel = %d}]\n", state->parenthesisLevel);
}

void* copyParserState(void* old) {
	ParserState* dochOld  = (ParserState*) old;
	ParserState* new      = malloc(sizeof(ParserState));
	new->parenthesisLevel = dochOld->parenthesisLevel;
	return new;
}

void parenthesizeState(void* willBeParserState, String* string) {
	ParserState* state = (ParserState*) willBeParserState;
	if (charAtString(*string, 0) == '(') state->parenthesisLevel += getLength(string);
	else                                 state->parenthesisLevel -= getLength(string);
}

ParserIndex processParenthesis(ParserEnvironment* env, ParserIndex main) {
	return registerParser(env, mkReset(main, &parenthesizeState));
}

void* parseStringOrAtomOrTraitOrSymbol(void* state, String* string) {
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
	
	token->parenthesisLevel = ((ParserState*) state)->parenthesisLevel;
	
	return (void*) token;
}

ParserIndex pureStringOrAtomOrTraitOrSymbol(ParserEnvironment* env) {
	return registerParser(env, mkPure(&parseStringOrAtomOrTraitOrSymbol));
}
void* parseNumber(void* state, String* string) {
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
	
	result->parenthesisLevel = ((ParserState*) state)->parenthesisLevel;

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

int main() {
	ParserEnvironment env  = mkParserEnvironment();
	addStateToParserEnvironment(&env, &copyParserState);
	enableKeepFlagToParserEnvironment(&env);

	ParserIndex defaultErr = registerParser(&env, mkErr("(IT): Invalid Token"));
	
	
	ParserIndex  empty[8]   = { 0 }; 
	ParserIndex  lisp       = registerParser(&env, mkAltList(empty, 8));
	ParserIndex* all        = getParserRef(&env, lisp)->value.altList.alts;

	ParserIndex  paren      = processParenthesis(&env, lisp);
	ParserIndex  parenL     = singleCharWithoutError(&env, '(', TRUE, TRUE, paren, defaultErr);
	ParserIndex  parenR     = singleCharWithoutError(&env, ')', TRUE, TRUE, paren, defaultErr);

	ParserIndex  pureTok    = pureStringOrAtomOrTraitOrSymbol(&env); 
	ParserIndex  lastQuote  = singleCharWithoutError(&env, '\"', FALSE, FALSE, pureTok, defaultErr); 
	ParserIndex  stringIn   = satisfyString(&env, lastQuote, defaultErr);
	ParserIndex  string     = singleCharWithoutError(&env, '\"', TRUE, FALSE, stringIn, defaultErr);

	ParserIndex  atmSymTrt  = satisfyAtomOrTraitOrSymbol(&env, pureTok, defaultErr);
	ParserIndex  atom       = singleCharWithoutError(&env, '\'', TRUE, FALSE, atmSymTrt, defaultErr);

	ParserIndex  pureNum    = pureNumber(&env);
	ParserIndex  number     = numbersWithoutError(&env, TRUE, pureNum, defaultErr);

	ParserIndex  trait      = singleCharWithoutError(&env, ':', TRUE, FALSE, atmSymTrt, defaultErr); 

	ParserIndex  skip       = registerParser(&env, mkReset(lisp, NULL));
	ParserIndex  space      = spaceOrNewline(&env, skip, defaultErr);;

	all[0] = parenL;
	all[1] = string;
	all[2] = atom;
	all[3] = number;
	all[4] = trait;
	all[5] = space;
	all[6] = atmSymTrt;
	all[7] = parenR;

	showParsers(&env);

	char lispCode[] = "(define factorial \n" 
				"(lambda (n) \n"
    					"(if (zerop n) 1 \n"
        				"(* n (factorial (- n 1))))))";
	
	ParserState startingState;
	startingState.parenthesisLevel = 0;
	
	printf("\n\n---Parsing: %s\n\n", lispCode);
	ParseSequenceResult tokens = runParserUntilWithState(&env, lisp, lispCode, 0, 1, &startingState);
	showParsingSequenceResult(tokens, &showTokenVoid);
	
	printf("---Final Parser State: ---\n");
	showParserState((ParserState*) env.state);
	#ifdef PARSER_STATISTICS
	showParserStatistics(env.statistics);
	#endif

	freeParserEnvironment(&env);
	if (tokens.type == ParsingSuccess) {
		mapLinkedList(&tokens.value.success, &freeToken);
		freeLinkedListInners(&tokens.value.success);
	}
	
	return 0;
}
