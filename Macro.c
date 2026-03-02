#include "AST.h"
#include "Macro.h"

void showMacro(Macro* macro, int depth) {
	int i;

	for (i = 0; i < depth; i++) printf("\t");
	printf("[Macro: (%s", macro->name);
	for (i = 0; i < macro->patternLength; i++) {
		switch (macro->pattern[i]) {
			case SinglePattern: printf(" *"); break;
			case GroupPattern:  printf(" (...)"); break;
			case QuotePattern:  printf(" (* ...)"); break;
			case RestPattern:   printf(" ..."); break;
		}
	}

	printf(")]\n");
}

/* removes a syntax from the environemnt before freeing it
 * removes all occurences in the syntax env
 * and removes all occurences of childeren syntaxes (ie function arguments)
 * this must be used for all macros that remove parts of the tree
 * to ensure memory saftey and SyntaxEnv correctnesss*/
void cullSyntax(Syntax* syntax, SyntaxEnv* env, int cullChildren, int freeOnDeletion) {
	SyntaxMap* recurse;
	SyntaxMap* before;
	SyntaxMap* found;
	int index;

	printf("syntax culling %s children, %s: \n", cullChildren ? "with" : "without", freeOnDeletion ? "freeing" : "not freeing");
	showSyntax(syntax, 1);

	printf("test\n");
	if (syntax->type == FunctionST || syntax->type == SymbolST) {
		recurse = env->map;
		found = NULL;
		index = syntax->type == FunctionST ? \
			syntax->value.function.symbol.index : \
			syntax->value.symbol.index;
		before = recurse;

		while (recurse && !found) {
			if (recurse->symbol.index == index) found = recurse;
			else before = recurse;
			recurse = recurse->next;
		}

		found->references--;

		/* if we removed something which was the last reference to the symbol*/
		if (!found->references) {
			if (before == found) env->map = recurse;
			else before->next = recurse;
			/* dont need to freeLinkedList since its already empty*/
			free(found->symbol.name);
			free(found);
		}

		/* if it was a function we have to recurse*/
		if (syntax->type == FunctionST && cullChildren) {
			/*just reusing index here*/
			for (index = 0; index < syntax->value.function.argumentCount; index++) \
				cullSyntax(syntax->value.function.arguments[index], env, cullChildren, freeOnDeletion);
		}
	}

	if (freeOnDeletion) free(syntax);
}

/* the min possible items a pattern can take
 * really the only one that has does not have a required match
 * is RestPattern, so this just counts the number that is not that*/
int minPatternLength(MacroPattern* pattern, int length) {
	int i; int result = 0;;

	for (i = 0; i < length; i++) {
		if (pattern[i] != RestPattern) result++;
	}

	return result;
} 

/*returns 1 if it is a valid macro
 * 0 if it is not (because a RestPattern is present but not the last element */
int validMacro(Macro* macro) {
	int i; 

	/* if it finds a rest pattern and its not the last element*/
	for (i = 0; i < macro->patternLength; i++) {
		if (macro->pattern[i] == RestPattern && \
			i != macro->patternLength - 1) return 0;
	}

	return 1;
}

/* should be used with the .arguments of the
 * also assumes the macro is valid
 * returns 1 if the macro matches and 0 otherwise
 * 1 means that the macro uses all given syntaxes, 
 *** and that the entire pattern is consumed*/
int matchMacro(Macro* macro, Syntax* syntaxes, int length) {
	int s = 0;
	int m = 0;

	/* printf("matching macro: \n");
	showMacro(macro, 1);
	printf("with syntax: \n");
	showSyntax(syntaxes, 1);
	*/
	while (m < macro->patternLength && s < length) {
		if ((macro->pattern[m] == SinglePattern) || \
			(macro->pattern[m] == GroupPattern && syntaxes[s].type == FunctionST) || \
			(macro->pattern[m] == QuotePattern && syntaxes[s].type == FunctionST)) {
			m++;
		} 
		s++;
	}	
	if (macro->pattern[m] == RestPattern) m++; /* if rest it must be last, so this always will check for it*/

	if (s == length && m == macro->patternLength) return 1;
	else return 0;
} 

void showMacroResult(MacroResult* result) {
	switch (result->type) {
		case SuccessMacro:
			printf("Macro substitutions succeeded!\n");
			break;
		case InvalidMacro:
			printf("Macro is invalid: \n");
			showMacro(result->value.invalidMacro, 1);
			break;
		case NotEvenAMacro:
			printf("Non-macro evaluated as macro: \n");
			showSyntax(result->value.notEvenAMacro, 1);
			break;
		case InvalidMacroArguments:
			printf("Syntax is invalid for macro: \n");
			showSyntax(result->value.invalidArguments, 1);
			break;
	}
}

MacroDict* mkMacroDict(Macro first) {
	MacroDict* result = malloc(sizeof(MacroDict));
	result->macro = first;
	result->next = NULL;

	return result;
}

void showMacroDict(MacroDict* dict) {
	printf("Macro Dict: \n");
	while (dict) {
		showMacro(&dict->macro, 1);
		dict = dict->next;
	}
}

/* checks if every macro in the dict is valid, according to validMacro*/
MacroResult validMacroDict(MacroDict* dict) {
	MacroDict* recurse = dict;
	MacroResult result;

	result.type = SuccessMacro;

	while (recurse) {
		if (!validMacro(&recurse->macro)) {
			result.type = InvalidMacro;
			result.value.invalidMacro = &recurse->macro;
			recurse = NULL;
		} else recurse = recurse->next;
	}

	return result;
}

/* adds a macro to the dict*/
void addMacro(MacroDict* dict, Macro* macro) {
	MacroDict* new = mkMacroDict(*macro);
	/* kind of wierd insertion after first element
	 * but it means we dont need a light wrapper like for LinkedList*/
	new->next = dict->next;
	dict->next = new;
}

/* mutually recursive with runMacro*/
MacroResult runMacroDict(MacroDict* macros, Syntax* ast, SyntaxEnv* env);

MacroResult runMacro(Syntax* syntax, Macro* macro, MacroDict* all, SyntaxEnv* env) {
	MacroResult result;

	if (syntax->type != FunctionST) {
		result.type = NotEvenAMacro;
		result.value.notEvenAMacro = syntax;
	} else if (!matchMacro(macro, syntax, syntax->value.function.argumentCount)) {
		result.type = InvalidMacroArguments;
		result.value.invalidArguments = syntax;
	} else {
		*syntax = macro->run(syntax, env);
		/* we have to check it any new syntaxes were created*/
		result = runMacroDict(all, syntax, env);
	}

	return result;
}

/* runs macros breadth first recursively through the tree*/
MacroResult runMacroDict(MacroDict* macros, Syntax* ast, SyntaxEnv* env) {
	MacroResult result;
	MacroDict* current;
	result.type = SuccessMacro;
	int i;

	if (ast->type == FunctionST) {
		current = macros;

		while (current) {
			if (matchPattern(ast->value.function.symbol.name, current->macro.name)) {
				printf("Matched macro %s\n", current->macro.name);
				result = runMacro(ast, &current->macro, macros, env);
				current = NULL;
			}
			
			if (current && result.type == SuccessMacro) current = current->next;
			else current = NULL;
		}
	
		if (ast->type == FunctionST) {
			for (i = 0; i < ast->value.function.argumentCount && result.type == SuccessMacro; i++) \
				result = runMacroDict(macros, ast->value.function.arguments[i], env);
		}
	}

	return result;
}

MacroResult runMacroDictOnAST(MacroDict* macros, AST* ast, SyntaxEnv* env) {
	MacroResult result;
	result.type = SuccessMacro;
	int i;

	for (i = 0; i < ast->syntaxesLength && result.type == SuccessMacro; i++) {
		result = runMacroDict(macros, ast->syntaxes[i], env);
	}

	return result;
}
