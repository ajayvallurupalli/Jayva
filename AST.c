#include "Tokenizer2.h"
#include "Parser2.h"
#include "AST.h"

void showSyntax(Syntax* syntax, int depth) {
	int i;
	
	for (i = 0; i < depth; i++) printf("\t");
	switch (syntax->type) {
		case StringST:
			printf("[StringST-{%s}]\n", syntax->value.string);
			break;
		case NumberST:
			printf("[NumberST-{%ld}]\n", syntax->value.number);
			break;
		case AtomST:
			printf("[AtomST-{name: %s, index: %d}]\n", syntax->value.atom.name, syntax->value.atom.index);
			break;
		case SymbolST:
			printf("[SymbolST-{name: %s, index: %d}]\n", syntax->value.symbol.name, syntax->value.symbol.index);
			break;
		case FunctionST:
			printf("[FunctionST-{name: %s, index: %d, argCount: %d, arguments: [", \
					syntax->value.function.symbol.name, syntax->value.function.symbol.index, \
					syntax->value.function.argumentCount);
			if (syntax->value.function.argumentCount) {
				printf("\n");
				for (i = 0; i < syntax->value.function.argumentCount; i++) \
					showSyntax(*(syntax->value.function.arguments + i), depth + 1);
				for (i = 0; i < depth; i++) printf("\t");
			}
			printf("]]\n");
			break;
	}
}

void freeSyntaxVoid(void* futureSyntax) {
	Syntax* syntax = (Syntax*) futureSyntax;
	int i;

	switch (syntax->type) {
		case StringST: 
		case NumberST:
		case AtomST:
		case SymbolST:
			break;
		case FunctionST:
			for (i = 0; i < syntax->value.function.argumentCount; i++) {
				freeSyntaxVoid(*(syntax->value.function.arguments + i));
			}
			free(syntax->value.function.arguments);
			break;
	}

	free(syntax);
}

SyntaxEnv mkSyntaxEnv() {
	SyntaxEnv result;
	result.map = NULL;
	result.nextIndex = 0;

	return result;
}


void showSyntaxEnv(SyntaxEnv* env) {
	SyntaxMap* map = env->map;

	printf("Current Syntax Environment: \n");
	while (map) {
		printf("\t[%s-{name: %s, defined: %s, index: %d, references: %d]\n", \
			map->symbol.type == AtomSymbol ? "Atom" : "Variable", \
			map->symbol.name, map->defined ? "true" : "false", \
			map->symbol.index, map->references);
		map = map->next;
	}
}

void freeSyntaxMap(SyntaxMap* map) {
	free(map->symbol.name);
	if (map->next) freeSyntaxMap(map);
}

void freeSyntaxEnv(SyntaxEnv* env) {
	if (env->map) freeSyntaxMap(env->map);
}

Symbol findOrRegisterGo(SyntaxEnv* env, SyntaxMap* map, char** search, SymbolType type, int plsFree, int incr) {
	SyntaxMap* newMap;
	Symbol result;

	if (type == map->symbol.type && matchPattern(map->symbol.name, *search) > 0) {
		result = map->symbol;
		if (incr) map->references++;
		if (plsFree) {
			free(*search); /* we can free it because it is a duplicate*/
			*search = NULL;
		}
	} else if (!map->next) {
		/* no more means that we need to add*/
		newMap = malloc(sizeof(SyntaxMap));
		newMap->defined = FALSE;
		if (plsFree) newMap->symbol.name = *search;
		else {
			newMap->symbol.name = malloc(sizeof(char) * lengthOfChars(*search));
			strcpy(newMap->symbol.name, *search);
		}
		newMap->symbol.type = type;
		newMap->symbol.index = env->nextIndex;
		newMap->references = incr ? 1 : 0;

		env->nextIndex++;
		map->next = newMap;
		result = newMap->symbol;
	} else {
		result = findOrRegisterGo(env, map->next, search, type, plsFree, incr);
	}

	return result;
}

/* if plsFree then derefed pointer will be freed and nulld if repeat
 * and shallow copied to other places if new
 * otherwise it will not free and make a deep copy on heap if new
 * needs char** so we also have the ability to set it to NULL
 * so that it can free while preventing double free*/
Symbol findOrRegister(SyntaxEnv* env, char** search, SymbolType type, int plsFree, int incr) {
	SyntaxMap* newMap;
	Symbol result;

	if (!env->map) {
		newMap = malloc(sizeof(SyntaxMap));
		newMap->defined = FALSE;
		if (plsFree) newMap->symbol.name = *search;
		else {
			newMap->symbol.name = malloc(sizeof(char) * lengthOfChars(*search));
			strcpy(newMap->symbol.name, *search);
		}
		newMap->symbol.type = type;
		newMap->symbol.index = env->nextIndex;
		newMap->references = incr ? 1 : 0;

		env->nextIndex++;
		env->map = newMap;
		result = newMap->symbol;
	} else {
		result = findOrRegisterGo(env, env->map, search, type, plsFree, incr);
	}

	return result;
}

Syntax* mkStringST(char* value) {
	Syntax* result = malloc(sizeof(Syntax));
	result->type = StringST;
	result->value.string = value;
	return result;
}

Syntax* mkNumberST(long value) {
	Syntax* result = malloc(sizeof(Syntax));
	result->type = NumberST;
	result->value.number = value;
	return result;
}

Syntax* mkAtomST(char** value, SyntaxEnv* env, int plsFree) {
	Syntax* result = malloc(sizeof(Syntax));
	result->type = AtomST;
	result->value.atom = findOrRegister(env, value, AtomSymbol, plsFree, TRUE);

	return result;
}

Syntax* mkSymbolST(char** value, SyntaxEnv* env, int plsFree) {
        Syntax* result = malloc(sizeof(Syntax));
        result->type = SymbolST;
        result->value.symbol = findOrRegister(env, value, VariableSymbol, plsFree, TRUE);

        return result;
}

/* also allocates space for arguments, but they remain unfilled*/
Syntax* mkFunctionST(char** name, int argumentCount, SyntaxEnv* env, int plsFree) {
	Syntax* result = malloc(sizeof(Syntax));
	result->type = FunctionST;
	result->value.function.argumentCount = argumentCount;
	result->value.function.arguments = malloc(sizeof(Syntax*) * argumentCount);
	result->value.function.symbol = findOrRegister(env, name, VariableSymbol, plsFree, TRUE);
	
	return result;
}

void showProcessResult(ProcessResult* pr) {
	switch (pr->type) {
		case ProcessSuccess:
			printf("Syntax process succeeded with value: \n");
			showSyntax(pr->value.success, 0);
			break;
		case ProcessError:
			printf("Syntax process failed with error: \n %s\n", pr->value.error);
	}
}

ProcessResult levelError() {
	ProcessResult result;
	result.type = ProcessError;
	result.next = NULL;
	strcpy(result.value.error, "Only functions allowed at top");

	return result;
}

/* these functions are mutually recursive*/
void processFunction(ProcessResult* write, Token* main, Cons* next, SyntaxEnv* env);
ProcessResult _processTokens(Cons* tokens, Token* previousToken, SyntaxEnv* env);

void processFunction(ProcessResult* write, Token* main, Cons* next, SyntaxEnv* env) {
	Syntax*       success;
	int           failure = 0;
	int           argumentCount = 0;
	Cons*         list = next;
	Token*        currentToken;
	Token*        previousToken = main;
	ProcessResult recurse;
	LinkedList    successes = mkLinkedList();


	/* failure = 1 means no end parenthesis, need to assign error
	 * failure = 2 meanse error in argument token, error is already assigned*/
	while (TRUE) {
		if (!list) {
			failure = 1; /* no closing paren, error*/
			break;
		}

		currentToken = (Token*) list->val;	
		
		if (currentToken->type == ParenthesisToken && currentToken->value.parenthesis < 0) \
			break; /*closing parenteheis, success*/
		
		recurse = _processTokens(list, previousToken, env);
		if (recurse.type == ProcessError) {
			*write = recurse;
			failure = 2;
			break;
		}
		
		consLinkedListByRef(&successes, (void*) recurse.value.success);
		argumentCount++;
		previousToken = (Token*) list->val;
		list = recurse.next;
		previousToken = recurse.previousToken;
	}

	if (failure == 1) {
		printf("no closing parenthesis!\n");
		write->type = ProcessError;
		write->next = NULL;
		strcpy(write->value.error, "No closing parenthesis");
		mapLinkedList(&successes, &freeSyntaxVoid);
	} else if (failure == 0) { /* success*/
		success = mkFunctionST(&main->value.symbol, argumentCount, env, TRUE);

		write->previousToken = currentToken;
		write->next = list->next;
		list = successes.list; /* just reusing the variable, kinda improper but convenient*/
		for (int i = argumentCount - 1; i >= 0; i--) {
			success->value.function.arguments[i] = ((Syntax*) list->val);
			list = list->next;
		}

		write->type = ProcessSuccess;
		write->value.success = success;
	}

	freeLinkedListInners(&successes);

}

ProcessResult _processTokens(Cons* tokens, Token* previousToken, SyntaxEnv* env) {
	ProcessResult  result;
	Cons*   list = tokens;
	Token*  uncons;

	if (!list) {
		result.type = ProcessError;
		strcpy(result.value.error, "No more tokens");
		return result;
	}

	uncons = (Token*) list->val;

	showToken(*uncons);
	switch (uncons->type) {
		case StringToken: 
			result.type = ProcessSuccess;
			result.value.success = mkStringST(uncons->value.string);
			break;
		case NumberToken:
			result.type = ProcessSuccess; 
                        result.value.success = mkNumberST(uncons->value.number);
			break;
		case AtomToken:
			result.type = ProcessSuccess;
			result.value.success = mkAtomST(&uncons->value.atom, env, TRUE);
			break;
		case SymbolToken:
			if (previousToken->type == ParenthesisToken) {
				/* (pattern = function*/
				processFunction(&result, uncons, tokens->next, env); 
			} else {
			 	/* pattern pattern = variable*/
				result.type = ProcessSuccess;
				result.value.success = mkSymbolST(&uncons->value.symbol, env, TRUE);
			}
			break;
		case ParenthesisToken:
			result = _processTokens(tokens->next, uncons, env);
			break;
		case TraitToken:
			result.type =  ProcessError;
			strcpy(result.value.error, "Trait unimplemented");
			return result;
	}

	if (result.type == ProcessSuccess && result.value.success->type != FunctionST) {
		result.next = tokens->next;
		result.previousToken = uncons;
	}

	return result;
}

ProcessResult processTokens(Cons* tokens, SyntaxEnv* env) {
	return _processTokens(tokens, NULL, env);
}

void showSyntaxResult(SyntaxResult* sr) {
	switch (sr->type) {
		case TokenError:
			printf("Tokenization error: %s\n", sr->value.tokenError);
			break;
		case SyntaxError:
			printf("Building Syntax error: %s\n", sr->value.syntaxError);
			break;
		case SyntaxSuccess:
			printf("Building Syntaxes succeded, built %d syntaxes: \n", \
					sr->value.success.syntaxesLength);
			for (int i = 0; i < sr->value.success.syntaxesLength; i++) \
				showSyntax(sr->value.success.syntaxes[i], 0);
			break;
	}
}

SyntaxResult buildAST(char* words, SyntaxEnv* env) {
	SyntaxResult result;
	ProcessResult recent;
	LinkedList accum = mkLinkedList();
	Cons* tokens;
	int fail = FALSE;
	int syntaxes = 0;	

	ParseSequenceResult tokenResult = tokenize(words);
	if (tokenResult.type == ParsingError) {
		result.type = TokenError;
		strcpy(result.value.tokenError, tokenResult.value.error);
		return result;
	}

	reverseLinkedList(&tokenResult.value.success);
	tokens = tokenResult.value.success.list;

	while (tokens && !fail) {
		recent = processTokens(tokens, env);
	
		if (recent.type == ProcessSuccess) {
			consLinkedListByRef(&accum, (void*) recent.value.success);
			tokens = recent.next;
			syntaxes++;
		} else fail = TRUE;
	}

	if (fail) {
		printf("failed at end!\n");
		result.type = SyntaxError;
		strcpy(result.value.syntaxError, recent.value.error);
		mapLinkedList(&accum, &freeSyntaxVoid);
		mapLinkedList(&tokenResult.value.success, &freeToken);
		freeLinkedListInners(&tokenResult.value.success);
		/* need to free here*/
	} else {
		result.type = SyntaxSuccess;
		result.value.success.syntaxesLength = syntaxes;
		result.value.success.syntaxes = malloc(sizeof(Syntax*) * syntaxes);
		
		tokens = accum.list;
		for (int i = syntaxes - 1; i >= 0; i--) {
			result.value.success.syntaxes[i] = ((Syntax*) tokens->val);
			tokens = tokens->next;
		}
	}


	freeLinkedListInners(&accum);


	#ifdef DEBUG
	showSyntaxResult(&result);
	showSyntaxEnv(env);
	#endif

	return result;
}

