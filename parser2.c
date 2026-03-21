#include <stdio.h>
#include <stdlib.h>
#include "LinkedList.h"
#include "String.h"
#include "Parser2.h"
#include "Config.h"

void showParser(const Parser p) {
	switch (p.type) {
		case PureAlgebra:
			printf("[Pure@%d]\n", p.index);
			break;
		case ErrAlgebra:
			printf("[Err@%d{err:'%s'}]\n", p.index, p.value.err.error);
			break;
		case SatisfyAlgebra:
			printf("[Satisfy@%d{eat:%s, repeat: %s, success index: %d, failure index %d}]\n", p.index, \
					p.value.satisfy.eat ? "true" : "false", \
					p.value.satisfy.repeat ? "true" : "false", p.value.satisfy.success, p.value.satisfy.failure);
			break;
		case PatternAlgebra: 
			printf("[Pattern@%d{eat: %s, repeat: %s, pattern: %s, success index: %d, failure index: %d}]\n", p.index, \
					p.value.pattern.eat ? "true" : "false", \
					p.value.pattern.repeat ? "true" : "false", p.value.pattern.pattern, \
					p.value.pattern.success, p.value.pattern.failure);
			break;
		case AltAlgebra:
			printf("[Alt@%d{left index: %d, right index: %d]\n", p.index, p.value.alt.left, p.value.alt.right);
			break;
		case AltListAlgebra:
			printf("[AltList@%d{length: %d, alts: [", p.index, p.value.altList.length);
			for (int i = 0; i < p.value.altList.length; i++) {
				printf("%d, ", p.value.altList.alts[i]);
			}
			printf("]}]\n");
			break;
		case ResetAlgebra:
			printf("[Reset@%d{next index: %d}]\n", p.index, p.value.reset.next);
			break;
	}
}

void showParserVoid(void* willBeParser) {
	showParser(*(Parser*) willBeParser);
}

Parser mkAlt(ParserIndex left, ParserIndex right) {
	Alt alt; Parser result;

	alt.left = left;
	alt.right = right;

	result.type = AltAlgebra;
	result.value.alt = alt;

	return result;
}

Parser mkAltList(ParserIndex* alts, int length) {
	AltList altList; Parser result;

	altList.alts = malloc(sizeof(ParserIndex) * length);
	altList.length = length;
	for (int i = 0; i < length; i++) {
		altList.alts[i] = alts[i];
	}

	result.type = AltListAlgebra;
	result.index = -1;
	result.value.altList = altList;

	return result;
}

Parser mkErr(char error[ERROR_MSG_SIZE]) {
	Err err; Parser result;
	
	strcpy(err.error, error);

	result.type = ErrAlgebra;
	result.index = -1;
	result.value.err = err;

	return result;
}

Parser mkPure(void* (*parse) (void*, String*)) {
	Pure pure; Parser result;

	pure.parse = parse;

	result.type = PureAlgebra;
	result.index = -1;
	result.value.pure = pure;

	return result;
}

Parser mkSatisfy(int eat, int repeat, Predicate pred, void* context, ParserIndex success, ParserIndex failure) {
	Satisfy satisfy; Parser result;

	satisfy.eat = eat;
	satisfy.repeat = repeat;
	satisfy.pred = pred;
	satisfy.context = context;
	satisfy.success = success;
	satisfy.failure = failure;

	result.type = SatisfyAlgebra;
	result.index = -1;
	result.value.satisfy = satisfy;

	return result;
}

Parser mkPattern(int eat, int repeat, char patternChar[16], ParserIndex success, ParserIndex failure) {
	Pattern pattern; Parser result;

	pattern.eat = eat;
	pattern.repeat = repeat;
	strcpy(pattern.pattern, patternChar);
	pattern.success = success;
	pattern.failure = failure;

	result.type = PatternAlgebra;
	result.index = -1;
	result.value.pattern = pattern;

	return result;
}

Parser mkReset(ParserIndex next, void (*effect) (void*, String*)) {
	Reset reset; Parser result;

	reset.next = next;
	reset.effect = effect;

	result.type = ResetAlgebra;
	result.index = -1;
	result.value.reset = reset;

	return result;
}

void showParsers(const ParserEnvironment* env) {
	if (!env->parsers) {
		printf("Parser Environment has not yet been configured.\n");
	} else {
		printf("---Registered Parsers---\n");
		mapLinkedList(env->parsers, &showParserVoid);
	}
}

#ifdef PARSER_STATISTICS 
void showParserStatistics(const ParserStatistics stats) {
	printf("---Parsers  Executed: %d ---\n", stats.parsersExecuted);
        printf("---Branches Executed: %d ---\n", stats.branchesExecuted);
}
#endif

ParserIndex registerParser(ParserEnvironment* env, Parser parser) {
	Parser* add = malloc(sizeof(Parser));
	*add = parser;

	add->index = env->nextIndex;
	env->nextIndex++;

	consLinkedListByRef(env->parsers, (void*) add);

	return add->index;
}

ParserEnvironment mkParserEnvironment() {
	ParserEnvironment result;
	LinkedList* list = malloc(sizeof(LinkedList));
	*list = mkLinkedList();


	result.nextIndex = 0;
	result.currentParser = -2;
	result.cursor = 0;
	result.forward = 0;
	result.plate = NULL;
	result.words = NULL;
	result.parsers = list;
	result.state = NULL;
	result.copy = NULL;
	result.keepState = FALSE;
	#ifdef PARSER_STATISTICS 
	result.statistics.parsersExecuted = 0;
	result.statistics.branchesExecuted = 0;
	#endif

	registerParser(&result, mkErr(EOF_ERROR));
	registerParser(&result, mkErr(INVALID_INDEX_ERROR));

	return result;
}

void addStateToParserEnvironment(ParserEnvironment* env, void* (*copy)(void*)) {
	env->copy = copy;
}

/* relies on ParserEnvrionment.forward which is only guarenteed to be accurate for
 * Satisfy.pred, so only use it for that predicate*/
char getNextCharacter(ParserEnvironment* env, int forward) {
	printf("env->forward: %d, forward: %d, env->cursor: %d\n", env->forward, forward, env->cursor);
	if (env->forward + forward < env->cursor) {
		return '\0';
	} else {
		return env->words[env->cursor + env->forward + forward];
	}
}

int _getParserPredicate(void* actuallyAIndex, void* actuallyAParser) {
	return (*(Parser*) actuallyAParser).index == (*(ParserIndex*) actuallyAIndex);
}
	
Parser* getParserRef(ParserEnvironment* env, ParserIndex index) {
	if (!env->parsers) {
		printf("Parser Environment has not been configured!\n");
		return NULL;
	} else return (Parser*) getElementOfLinkedList(env->parsers, &index, &_getParserPredicate);
}

char getCharacter(ParserEnvironment* env, int forward) {
	return env->words[env->cursor + forward];
}

void freeParserResources(Parser* parser) {
	switch(parser->type) {
		case PureAlgebra:
		case ErrAlgebra:
		case AltAlgebra:
		case ResetAlgebra:
		case PatternAlgebra:
			break;
		case AltListAlgebra:
			if (!parser->value.altList.alts) free(parser->value.altList.alts);
			break;
		case SatisfyAlgebra:
			if (!parser->value.satisfy.context) free(parser->value.satisfy.context);
			break;
	}
}

void freeParserResourcesAsVoid(void* parser) {
	freeParserResources((Parser*) parser);
}

void freeParserEnvironment(ParserEnvironment* env) {
	mapLinkedList(env->parsers, &freeParserResourcesAsVoid);
	freeLinkedListInners(env->parsers);
	free(env->parsers);
	env->parsers = NULL;
}

void showParsingResult(const ParsingResult shower, void (*showSuccess) (const void*)) {
	switch (shower.type) {
		case ParsingError:
			printf("Parsing failed with error: %s\n", shower.value.error);
			break;
		case ParsingSuccess:
			printf("Parsing succeeded with value: ");
			showSuccess(shower.value.success);
			printf("\n");
			break;
	}
}

/* -1 if the string being matched on ends before the end of the pattern
 *  0 if the string does not match the pattern
 *  n if the string does match the pattern, where n is the length of the pattern*/
int matchPattern(char* match, char* pattern) {
	int index = 0;
	while (pattern[index] != '\0') {
		if (match[index] == '\0') {
			return -1;
		} else if (match[index] == pattern[index]) {
			index++;
		} else {
			return FALSE;
		}
	}

	return index;
}

/* 0 if the string does not match the pattern
 * n if the string completely matches the pattern, where n = length of pattern = length of match*/
int matchPatternStrict(char* match, char* pattern) {
	int index = 0;
	int fail = FALSE;
	int loop = TRUE;
	while (loop) {
		if (match[index] == '\0' && pattern[index] == '\0') loop = FALSE;
		else if (match[index] == pattern[index]) index++;
	 	else {
			fail = TRUE;
			loop = FALSE;
		}
	}

	return fail ? 0 : index;
}

/* camel case is kinda ugly here*/
int isEOFerror(char* error) {
	/* literally the most trivial implementation you could think of 
	 * even if its a little bandaidy
	 * tagged union errors would have been nice but for now this is fine*/
	return (error[1] == 'E' && error[2] == 'O' && error[3] == 'F');
}

/* Dont't use this function, use runParser or runParserFromStart instead  */
ParsingResult _runParserGo(ParserEnvironment* env, ParserIndex index, int forward, int state) {
	ParsingResult result;
	Parser* parser = getParserRef(env, index);
	int matchPatternResult = 0;
	int stableMatchPattern = 0;
	int altListIndex = 0;
	String altPlate;
	void* altState = NULL;
	char nextChar;

	if (!parser) {
		parser = getParserRef(env, INVALID_INDEX_PARSER_INDEX);
	}

	env->currentParser = parser->index;

	#ifdef DEBUG
	showParser(*parser); /*debug*/
	#endif
	#ifdef PARSER_STATISTICS 
	env->statistics.parsersExecuted++; 
	#endif
	switch(parser->type) {
		case ErrAlgebra:
			result.move = 0;
			result.type = ParsingError;
			strcpy(result.value.error, parser->value.err.error);
			break;
		case PureAlgebra:
			result.move = forward;
			result.type = ParsingSuccess;
			result.value.success = parser->value.pure.parse(state ? env->state : NULL, env->plate);
			break;
		case AltAlgebra: 
			altPlate = copyString(env->plate);
			if (state) altState = env->copy(env->state);
			result = _runParserGo(env, parser->value.alt.left, forward, state);
		
			#ifdef PARSER_STATISTICS
                        env->statistics.branchesExecuted++;
                        #endif

			if (result.type == ParsingError) {
				#ifdef PARSER_STATISTICS
				env->statistics.branchesExecuted++;
				#endif
				
				commandeerString(env->plate, &altPlate);
				if (state) free(env->state);
				if (state) env->state = altState;
				
				result = _runParserGo(env, parser->value.alt.right, forward, state);
			} else {
				freeString(&altPlate); /*Still need to free inner, but shouldn't if it boards env->plate */
				if (altState && state) free(altState); /* not sure when altState is NULL but state is true, but to be safe*/
			}
			break;
		case AltListAlgebra:
			while (altListIndex != parser->value.altList.length) {
				altPlate = copyString(env->plate);
				if (state) altState = env->copy(env->state);
				result = _runParserGo(env, parser->value.altList.alts[altListIndex], forward, state);
				
				#ifdef PARSER_STATISTICS
                                env->statistics.branchesExecuted++;
                                #endif

				if (result.type == ParsingError) {
					#ifdef PARSER_STATISTICS
					env->statistics.branchesExecuted++;
					#endif
					
					commandeerString(env->plate, &altPlate);
					altListIndex++;
					if (state && !(env->keepState && altListIndex == parser->value.altList.length && isEOFerror(result.value.error))) {
						free(env->state);
						env->state = altState;
					}
				} else {
					freeString(&altPlate);
					altListIndex = parser->value.altList.length; /*to exit the loop*/
					if (altState && state) free(altState);
				}
			}
			break;
		case ResetAlgebra: 
			if (parser->value.reset.effect) parser->value.reset.effect(state ? env->state : NULL, env->plate);
			setString(env->plate, 0, "");
			result = _runParserGo(env, parser->value.reset.next, forward, state);
			break;
		case SatisfyAlgebra: 
			nextChar = getCharacter(env, forward);
			if (nextChar == '\0') {
				result = _runParserGo(env, EOF_PARSER_INDEX, forward, state);
				return result; /*need to skip all that later stuff*/
			}

			if (parser->value.satisfy.pred(env, parser->value.satisfy.context, nextChar)) { /* success */
				if (parser->value.satisfy.eat) appendCharToString(env->plate, nextChar);

				if (parser->value.satisfy.repeat) {
					forward = forward + 1;
					env->forward = forward;
					nextChar = getCharacter(env, forward);

					while (nextChar != '\0' && parser->value.satisfy.pred(env, parser->value.satisfy.context, nextChar)) {
						if (parser->value.satisfy.eat) appendCharToString(env->plate, nextChar);
						forward = forward + 1;
						env->forward = forward;
						nextChar = getCharacter(env, forward);
					}

					result = _runParserGo(env, parser->value.satisfy.success, forward, state);
				} else {
					result = _runParserGo(env, parser->value.satisfy.success, forward + 1, state);
				}
			} else result = _runParserGo(env, parser->value.satisfy.failure, 0, state);
			break;
		case PatternAlgebra: 
			matchPatternResult = matchPattern(env->words + env->cursor + forward, parser->value.pattern.pattern);
			if (matchPatternResult == -1) result = _runParserGo(env, EOF_PARSER_INDEX, forward, state);
			else if (!matchPatternResult) result = _runParserGo(env, parser->value.pattern.failure, 0, state);
			else {
				/*we use the pattern since it will be the same by match, and it is actually null terminated*/
				if (parser->value.satisfy.eat) appendCharsToString(env->plate, parser->value.pattern.pattern); 
				forward = forward + matchPatternResult;

				if (parser->value.pattern.repeat) {
					stableMatchPattern = matchPatternResult;
					while (TRUE) {
						matchPatternResult = matchPattern(env->words + env->cursor + forward, parser->value.pattern.pattern);
						if (matchPatternResult <= 0) break; /* -1 (EOF) or 0 (invalid match)*/
						if (parser->value.pattern.eat) appendCharsToString(env->plate, parser->value.pattern.pattern);
						forward = forward + matchPatternResult;
					}

					forward = forward - stableMatchPattern;
				}

				result = _runParserGo(env, parser->value.pattern.success, forward, state);
			}
			break;	
	}

	return result;
}

/* runs a Parser until a Pure value or an Err.
 * If its Pure, it returns ParserSuccess from the 
 * 	ParsingResult tagged union, with the void pointer created by its parse method
 * If its Err, it returns ParserError from the 
 * 	ParsingResult tagged union, with the error message from the Err*/
ParsingResult runParser(ParserEnvironment* env, ParserIndex parser, char* words, int cursor) {
	String plate = mkString("");
	env->words = words;
	env->cursor = cursor;
	env->plate = &plate;
	ParsingResult result = _runParserGo(env, parser, 0, FALSE);
	env->cursor = env->cursor + env->forward;
	env->forward = 0;
	freeString(&plate);

	return result;
}

ParsingResult runParserWithState(ParserEnvironment* env, ParserIndex parser, char* words, int cursor, void* initialState) {
	ParsingResult result;

	if (!env->copy) {
		result.move = 0;
		result.type = ParsingError;
		strcpy(result.value.error, "State copy not configured");
	} else {
		String plate = mkString("");
		env->words = words;
		env->cursor = cursor;
		env->plate = &plate;

		env->state = env->copy(initialState);

		result = _runParserGo(env, parser, 0, TRUE);
		env->cursor = env->cursor + env->forward;
		env->forward = 0;
		freeString(&plate);
	}

	return result;
}

void showParsingSequenceResult(const ParseSequenceResult shower, void (*showSuccess) (const void*)) {
	switch (shower.type) {
		case ParsingError:
			printf("Parsing failed with error: %s\n", shower.value.error);
			break;
		case ParsingSuccess:
			printf("Parsing succeeded with value: \n");
			forEachLinkedList(&shower.value.success, showSuccess);
			printf("\n");
			break;
	}
}

/* Parses until it runs into an error, or the text ends */
ParseSequenceResult runParserUntil(ParserEnvironment* env, ParserIndex parser, char* words, int cursor, int strict) {
	ParsingResult recentResult;
	LinkedList successes = mkLinkedList();
	int successCount = 0;
	ParseSequenceResult result;
	
	while (TRUE) {
		recentResult = runParser(env, parser, words, cursor);
		switch (recentResult.type) {
			case ParsingSuccess:
				successCount++;
				cursor = cursor + recentResult.move;
				successes = consLinkedList(successes, recentResult.value.success);			
				break;
			case ParsingError:
				goto loopEnd;
				break;
		}
	}
	loopEnd:;

	if (successCount && !(strict && recentResult.type == ParsingError && words[env->cursor] != '\0')) {
		result.type = ParsingSuccess;
		result.value.success = successes;
	} else {
		result.type = ParsingError;
		strcpy(result.value.error, recentResult.value.error);
	}

	return result;
}

ParseSequenceResult runParserUntilWithState(ParserEnvironment* env, ParserIndex parser, char* words, int cursor, int strict, void* initialState) {
        ParsingResult recentResult;
        LinkedList successes = mkLinkedList();
        int successCount = 0;
        ParseSequenceResult result;
        String plate;

        if (!env->copy) {
                result.type = ParsingError;
                strcpy(result.value.error, "State copy not configured");
                return result;
        }

        plate = mkString("");
        env->words = words;
        env->plate = &plate;
	env->cursor = cursor;
	env->state = env->copy(initialState);

        while (TRUE) {          
		setString(&plate, 0, "");
                recentResult = _runParserGo(env, parser, 0, TRUE);
		env->cursor = env->cursor + env->forward;
		env->forward = 0;
                switch (recentResult.type) {
                        case ParsingSuccess:
                                successCount++;
                                successes = consLinkedList(successes, recentResult.value.success);
                                break;
                        case ParsingError:
                                goto loopEnd;
                                break;
                }
        }
        loopEnd:;

	freeString(&plate);

        if (successCount && !(strict && recentResult.type == ParsingError && words[env->cursor] != '\0')) {
                result.type = ParsingSuccess;
                result.value.success = successes;
        } else {
                result.type = ParsingError;
                strcpy(result.value.error, recentResult.value.error);
        }

        return result;

}

/* applies a list of parsers, one after another. if `strict` flag is enabled, it will error unless all parsers succeed,
 * otherwise it will error only if the first parser fails*/
ParseSequenceResult runParsersInSequence(ParserEnvironment* env, ParserIndex* parsers, int length, char* words, int cursor, int strict) {
	ParsingResult recentResult;
	int index = 0;
	LinkedList successes = mkLinkedList();
	int successCount = 0;
	ParseSequenceResult result;

	while (index < length) {
		recentResult = runParser(env, parsers[index], words, cursor);
		switch (recentResult.type) {
			case ParsingSuccess:
				successCount++;
				env->cursor = env->cursor + recentResult.move;
				successes = consLinkedList(successes, recentResult.value.success);
				break;
			case ParsingError:
				goto loopEnd;
				break;
		}
	} /*while*/
	loopEnd:;

	if ((successCount && !strict) || (successCount == length)) {
		result.type = ParsingSuccess;
		result.value.success = successes;
	} else {
		result.type = ParsingError;
		strcpy(result.value.error, recentResult.value.error);
	}

	return result;

}

ParseSequenceResult runParsersInSequenceWithState(ParserEnvironment* env, ParserIndex* parsers, int length, char* words, int cursor, int strict, void* initialState) {
        ParseSequenceResult result;
	result.type = ParsingError;
	strcpy(result.value.error, "function not impl");
	return result;
}    

/* TODO split the specific parsers into another file*/


void* takeAsGivenParse(void* _state, String* string) {
	String* result = malloc(sizeof(String));
	*result = copyString(string);

	return (void*) result;
}


ParserIndex takeAsGiven(ParserEnvironment* env) {
	return registerParser(env, mkPure(&takeAsGivenParse));
}

void* parseAsInt(void* _state, String* string) {
	int* result = malloc(sizeof(int));
	int  digit  = 0;
	*result     = 0;

	for (int i = 0; i < getLength(string); i++) {
		digit = (int) string->data[i] - (int) '0';
		if (digit > 9) { /* not a number */
			break;
		} else {
			*result = *result * 10;
			*result = *result + digit;
		}
	}

	return (void*) result;
}

ParserIndex pureInt(ParserEnvironment* env) {
	return registerParser(env, mkPure(&parseAsInt));
}

int isDigitPred(ParserEnvironment* _env, void* _context, char ch) {
	return ch >= '0' && ch <= '9';
}

ParserIndex numbers(ParserEnvironment* env, int eat, ParserIndex success) {
	char errMsg[] = "(CNF) No nums in text";
	Parser errParser = mkErr(errMsg);
	ParserIndex errParserIndex = registerParser(env, errParser);

	Parser mainParser = mkSatisfy(eat, TRUE, &isDigitPred, NULL, success, errParserIndex);
	ParserIndex mainParserIndex = registerParser(env, mainParser);

	return mainParserIndex;
}

ParserIndex numbersWithoutError(ParserEnvironment* env, int eat, ParserIndex success, ParserIndex failure) {
	return registerParser(env, mkSatisfy(eat, TRUE, &isDigitPred, NULL, success, failure));
}

int _singleCharPred(ParserEnvironment* _env, void* context, char ch) {
	return (*(char*) context) == ch;
}

ParserIndex singleChar(ParserEnvironment* env, char ch, int eat, int repeat, ParserIndex success) {
	char* context = malloc(sizeof(char));
	*context = ch;

	char errMsg[] = "(CNF): No ! in text";
	errMsg[10] = ch;
	Parser errParser = mkErr(errMsg);

	ParserIndex errParserIndex = registerParser(env, errParser);
	
	Parser mainParser = mkSatisfy(eat, repeat, &_singleCharPred, context, success, errParserIndex);
	ParserIndex mainParserIndex = registerParser(env, mainParser);


	return mainParserIndex;
}

ParserIndex singleCharWithoutError(ParserEnvironment* env, char ch, int eat, int repeat, ParserIndex success, ParserIndex failure) {
	char* context = malloc(sizeof(char));
	*context = ch;

	return registerParser(env, mkSatisfy(eat, repeat, &_singleCharPred, context, success, failure));
}

int parser2main() {
	char words[] = "1234512345123";
	printf("sizeof(Parser) = %lu\n", sizeof(Parser));
	printf("Text: %s\n", words);
		
	ParserEnvironment env = mkParserEnvironment();

	ParserIndex pint  = pureInt(&env);
	ParserIndex pater = registerParser(&env, mkErr("Pattern not found"));
	ParserIndex pat  = registerParser(&env, mkPattern(TRUE, TRUE, "12345", pint, pater));
	ParserIndex alts[] = {pater, pater, pat};
	ParserIndex altListTest = registerParser(&env, mkAltList(alts, 3));	

	showParsers(&env);
	
	printf("---Parsing---\n");
	ParsingResult result = runParser(&env, altListTest, words, 0);
	(void) result;

	freeParserEnvironment(&env);

	printf("Bye now!\n");

	return 0;
}
