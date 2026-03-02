x#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO
 * The parser ptrs have to be replaced with a parser_index, 
 * so that an environment handles allocation instead of the user
 * I thought that it would add unncesscary complexity, but
 * Im realizing that it will be really nice for 
 * composition, since it doesn't force the user to handle all allocations,
 * and otherwise there would be really complex memory split between heap and stack
 * */

#define ERROR_MSG_SIZE 40
#define TRUE  1
#define FALSE 0
#define ParserIndex int

struct Parser; 

enum ParserValue {
	PureValue,
	ErrValue,
	SatisfyValue,
	AltValue,
	ResetValue
};

typedef int (*Predicate) (void*, char);


/* sizeof(Process) = 40 */
typedef struct {
	int            eat; /* boolean */
	int            repeat; /* boolean */
	Predicate      pred;
	void*          context;
	struct Parser* success;
	struct Parser* failure;
} Satisfy;

/* 32 bytes matches max size for the Parser union */
typedef struct {
	char error[ERROR_MSG_SIZE];
} Err;

/* sizeof(Alt) = 16 */
typedef struct {
	struct Parser* left;
	struct Parser* right;
} Alt;

/* sizeof(Pure) = 8*/
typedef struct {
	void* (*parse) (char*, int);
} Pure;

typedef struct {
	struct Parser* next;
} Reset;

/* sizeof(Parser) = 48 */
typedef struct Parser {
	enum ParserValue type;
	union {
		Pure      pure;
		Err       err;
		Satisfy   satisfy;
		Alt       alt;
		Reset     reset;
	} value;
} Parser;


Parser mkAlt(Parser* left, Parser* right) {
	Alt alt; Parser result;

	alt.left = left;
	alt.right = right;

	result.type = AltValue;
	result.value.alt  = alt;

	return result;
}

Parser mkErr(char error[16]) {
	Err err; Parser result;
	
	strcpy(err.error, error);

	result.type = ErrValue;
	result.value.err = err;

	return result;
}

Parser mkPure(void* (*parse) (char*, int)) {
	Pure pure; Parser result;

	pure.parse = parse;

	result.type = PureValue;
	result.value.pure = pure;

	return result;
}

Parser mkSatisfy(int eat, int repeat, Predicate pred, void* context, Parser* success, Parser* failure) {
	Satisfy satisfy; Parser result;

	satisfy.eat = eat;
	satisfy.repeat = repeat;
	satisfy.pred = pred;
	satisfy.context = context;
	satisfy.success = success;
	satisfy.failure = failure;

	result.type = SatisfyValue;
	result.value.satisfy = satisfy;

	return result;
}

Parser mkReset(Parser* next) {
	Reset reset; Parser result;

	reset.next = next;

	result.type = ResetValue;
	result.value.reset = reset;

	return result;
}

typedef struct Cons {
	void*        val;
	struct Cons* next;
} Cons;

typedef struct {
	struct Cons* list;
} LinkedList;

Cons mkCons(void* val) { 
	Cons result; 
	result.val = val;
	result.next = NULL;
	return result;
}

LinkedList mkLinkedList() {
	LinkedList result;
	result.list = NULL;
	return result;
}

LinkedList consLinkedList(LinkedList list, void* val) {
	Cons* next = malloc(sizeof(Cons));
	*next = mkCons(val);

	next->next = list.list;
	list.list = next;
	return list;
}

void consLinkedListByRef(LinkedList* list, void* val) {
	Cons* next = malloc(sizeof(Cons));
	*next = mkCons(val);

	next->next = list->list;
	list->list = next;
}

enum ParsingResultType {
	ParsingError,
	ParsingSuccess
};

typedef struct {
	int                    move;
	enum ParsingResultType type;
	union {
		char error[ERROR_MSG_SIZE];
		void* success;
	}                      value;

} ParsingResult;

/* Dont't use this function, use runParser or runParserFromStart instead  */
ParsingResult _runParserGo(struct Parser* parser, char* words, int cursor, int forward, int previousSuccess) {
	ParsingResult result;

	if (words[forward] == '\0') {
		result.move = cursor;
		result.type = ParsingError;
		strcpy(result.value.error, "End of File;");
		goto bypass;
	}

	switch(parser->type) {
		case ErrValue:
			result.move = cursor;
			result.type = ParsingError;
			strcpy(result.value.error, parser->value.err.error);
			break;
		case PureValue:
			result.move = forward;
			result.type = ParsingSuccess;
			result.value.success = parser->value.pure.parse(words + cursor, forward - cursor);
			break;
		case AltValue: 
			result = _runParserGo(parser->value.alt.left, words, cursor, forward, 0);
			if (result.type == ParsingError) result = _runParserGo(parser->value.alt.right, words, cursor, forward, 0);
			break;
		case ResetValue: 
			result = _runParserGo(parser->value.reset.next, words, cursor, cursor, 0);
			break;
		case SatisfyValue: 
			if (parser->value.satisfy.pred(parser->value.satisfy.context, words[forward])) { /* success */
				if (parser->value.satisfy.repeat) {
					result = _runParserGo(parser, words, \
							parser->value.satisfy.eat ? cursor : cursor + 1, forward + 1, 1);
				} else {
					result = _runParserGo(parser->value.satisfy.success, words, \
							parser->value.satisfy.eat ? cursor : cursor + 1, forward + 1, 0);
				}
			} else if (previousSuccess) {
				result = _runParserGo(parser->value.satisfy.success, words, cursor, forward, 0);
			} else {
				result = _runParserGo(parser->value.satisfy.failure, words, cursor, cursor, 0);
			}

			break;
	}
	bypass:;

	return result;
}

/* runs a Parser until a Pure value or an Err.
 * If its Pure, it returns ParserSuccess from the 
 * 	ParsingResult tagged union, with the void pointer created by its parse method
 * If its Err, it returns ParserError from the 
 * 	ParsingResult tagged union, with the error message from the Err*/
ParsingResult runParser(struct Parser* parser, char* words, int cursor) {
	return _runParserGo(parser, words, cursor, cursor, 0);
}

/* basically `runParser(Parser* parser, char* words, 0)`  */
ParsingResult runParserFromStart(struct Parser* parser, char* words) {
	return _runParserGo(parser, words, 0, 0, 0);
}

typedef struct {
	enum ParsingResultType type;
	union {
		char       error[ERROR_MSG_SIZE];
		LinkedList success;
	}                      value;
} ParseSequenceResult;

/* Parses until it runs into an error, or the text ends */
ParseSequenceResult runParserUntil(struct Parser* parser, char* words, int cursor, int length) {
	ParsingResult recentResult;
	LinkedList successes = mkLinkedList();
	int successCount = 0;
	ParseSequenceResult result;
	
	while (TRUE) {
		recentResult = runParser(parser, words, cursor);
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

	if (successCount) {
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
ParseSequenceResult runParsersInSequence(struct Parser** parser, int length, char* words, int cursor, int strict) {
	ParsingResult recentResult;
	int index = 0;
	LinkedList successes = mkLinkedList();
	int successCount = 0;
	ParseSequenceResult result;

	while (index < length) {
		recentResult = runParser(parser[index * sizeof(struct Parser*)], words, cursor);
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

int main() {
	


	return 0;
}
