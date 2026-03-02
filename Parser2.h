#ifndef Parser2Included
#define Parser2Included

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LinkedList.h"
#include "String.h"
#include "Config.h"

/* `ParserEnvironment` Constants */
#define ERROR_MSG_SIZE 32
#define EOF_ERROR "(EOF): End of File."
#define EOF_PARSER_INDEX 0
#define INVALID_INDEX_ERROR "(II): Invalid Parser Index." 
#define INVALID_INDEX_PARSER_INDEX 1

#define TRUE  1
#define FALSE 0

struct Parser; 
struct ParserEnvironment;

/* -ParserAlgebras-
 * Parsers can only terminate at Pure or Err
 * [[Parser]] denotes fixed point algebraic evaluation
 * Satisfy(success, failure)                  = if pred then [[success]] else [[failure]]
 * Pattern(success, failure)                  = if == word then [[success]] else [[failure]]
 * Pure                                       = parse(string)
 * Err                                        = error
 * Alt(left: [[Pure]], right: [[Pure | Err]]) = left !note: right is not evaluated in this case
 * Alt(left: [[Err]], right: [[Pure]])        = right !note: left is still evaluated in this case, but state is reset for right branch
 * Alt(left: [[Err]], right: [[err]])         = right
 * AltList(x:xs)                              = Alt(x, AltList(xs))
 * [[Reset(next)]]                            = [[next]] !note: reset resets parser state and restarts the parser at next
 * */
enum ParserAlgebra {
	PureAlgebra,
	ErrAlgebra,
	SatisfyAlgebra,
	PatternAlgebra,
	AltAlgebra,
	AltListAlgebra,
	ResetAlgebra
};

typedef int ParserIndex;

typedef int (*Predicate) (struct ParserEnvironment*, void*, char);

/* All Parsers should be created with `mk_` and registered in an 
 * `ParserEnvironment` with `registerParser`
 * then, they can be freed with `freeParserEnvironment`*/

/* sizeof(Satisfy) = 32 
 * Satisfy runs the predicate, where it has access to the current 
 * environment and its own internal context
 * If the pred succeeds and repeat is true, it will keep running until it fails before continuing
 * otherwise it will continue immidately and run success
 * if eat is true, it will add the char predicated to the String to be parsed, otherwise it is skipped
 * If the pred fails, it will run the failure parser*/
typedef struct {
	int            eat; /* boolean */
	int            repeat; /* boolean */
	Predicate      pred;
	void*          context;
	ParserIndex    success;
	ParserIndex    failure;
} Satisfy;

/* sizeof(Pattern) = 32
 * Pattern is like Satisfy but it just tries to match a given string
 * technically you can do this with Satisfy but its super memory inefficent
 * so if you are looking for a specific word just use this*/
typedef struct {
	int         eat;
	int         repeat;
	char        pattern[16];
	ParserIndex success;
	ParserIndex failure;
} Pattern;

/* 32 bytes matches max size for the Parser union 
 * Err is an error message that will terminate the parser, unless if it is alted
 * see description at -ParserAlgebras- for how this works*/
typedef struct {
	char error[ERROR_MSG_SIZE];
} Err;

/* sizeof(Alt) = 8 
 * Similar to alt in Haskell, see -ParserAlgebras- for details*/
typedef struct {
	ParserIndex left;
	ParserIndex right;
} Alt;

/* sizeof(AltList) = 12
 * equivalent to chained alts
 * note: if the keepState flag is enabled for ParserEnvironment, 
 *** the state will not be reset for the final alt in the altList
 *** when the reset is supposed to happen because of an EOF error 
 *** basically the state will not be reset by EOF
 *** and the final alt will designate the surviving state*/
typedef struct {
	ParserIndex* alts;
	int          length;
} AltList;

/* sizeof(Pure) = 8 
 * Pure is the success outcome, it is guarenteed to stop execution 
 * and parse the collected string with .parse
 * optionally using the parserState with the given void*
 * it should allocate and return a void* for collection*/
typedef struct {
	void* (*parse) (void*, String*);
} Pure;

/* sizeof(Reset) = 12 
 * Reset resets the ParserEnvironment's collected string, and restarts it at next
 * Can be useful for when you want to perform an effect but not collect output 
 */
 typedef struct {
	void (*effect) (void*, String*); /*void* is parserState*/
	ParserIndex next;
} Reset;

/* sizeof(Parser) = 40 
 * Parser Union of all in -ParserAlgebras-*/
typedef struct Parser {
	enum ParserAlgebra type;
	ParserIndex      index;
	union {
		Pure      pure;
		Err       err;
		Satisfy   satisfy;
		Pattern   pattern;
		Alt       alt;
		AltList   altList;
		Reset     reset;
	}                value;
} Parser;

/* Pretty Print for the Parser */
void showParser(Parser p);

/* Pretty Print for the Parser, for void*. useful for mapLinkedList */
void showParserVoid(void* willBeParser);

/* -1 if the string being matched on ends before the end of the pattern
 *  0 if the string does not match the pattern
 *  n if the string does match the pattern, where n is the length of the pattern*/
int matchPattern(char* match, char* pattern);

/* Default index for all mk_ functions is -1, it must be properly registered in a 
 * ParserEnvironment before it can be actually used. 
 * See -ParserAlgebras- for info on how they work*/
Parser mkAlt(ParserIndex left, ParserIndex right);
Parser mkAltList(ParserIndex* alts, int length);
Parser mkErr(char error[ERROR_MSG_SIZE]);
Parser mkPure(void* (*parse) (void*, String*));
Parser mkSatisfy(int eat, int repeat, Predicate pred, void* context, ParserIndex success, ParserIndex failure);
Parser mkPattern(int eat, int repeat, char pattern[16], ParserIndex success, ParserIndex failure);
Parser mkReset(ParserIndex nex, void (*effect) (void*, String*));

#ifdef PARSER_STATISTICS 
typedef struct {
	int          parsersExecuted; 
        int          branchesExecuted;
} ParserStatistics;

void showParserStatistics(ParserStatistics stats);
#endif

/* -ParserEnvironment- 
 * `ParserEnvironment` manages parser memory and evaluation
 * all parsers must be registered in `ParserEnvironment` with 
 * `registerParser` before they can be evaluated, otherwise any execution will 
 * terminate with a (II): Invalid Index Err
 * this error is registered in the parser when the environment is made with mkParserEnvironment 
 * and set at index 1. (EOF): End of File is another err which is automatically applied
 * and is set to index 0. 
 * note: the .forward variable is dangerous, and should only be used within the context of a Satisfy.pred predicate
 * it is used to see the current chacter hovered by the parser*/
typedef struct ParserEnvironment {
	ParserIndex  nextIndex;     /* used to ensure proper indexing*/
	ParserIndex  currentParser; /* currently not used? but set properly*/
	unsigned int cursor;        /* starting cursor*/
	unsigned int forward;       /* relative distance from starting cursor*/
	String*      plate;         /* plate for Pure.parse*/
	char*        words;         /* actual data being parsed*/
	LinkedList*  parsers;       /* registered parsers (index is handled in Parser struct*/
	void*        state;         /* parser state*/
	void*        (*copy) (void*); /* needed to copy state to protect it for alts. should return ptr to 
				       * memory allocated on the heap*/
	int          keepState;     /*super specific flag, see AltList for more*/
	#ifdef PARSER_STATISTICS 
	ParserStatistics statistics;
	#endif
} ParserEnvironment;

/* shows all registered parsers*/
void showParsers(ParserEnvironment* env);
/* registers parser in environment, and returns its respective index*/
ParserIndex registerParser(ParserEnvironment* env, Parser parser);
/* creates `ParserEnvironment` and registers starting `Err` parsrs 
 * for (EOF) and (II)*/
ParserEnvironment mkParserEnvironment();
/* adds state to the parser, using the copy function to deal with branching possibilities
 * initialState will be given in the specific parsing functions*/
void addStateToParserEnvironment(ParserEnvironment* env, void* (*copy)(void*));
/* enables a really specific keep flag, see AltList for more*/
void enableKeepFlagToParserEnvironment(ParserEnvironment* env);
/* relies on ParserEnvrionment.forward which is only guarenteed to be accurate for
 * Satisfy.pred, so only use it for that predicate
 * TODO: add ability to search forward more. will require a length part of ParserEnvironment to ensure safety*/
char getNextCharacter(ParserEnvironment* env, int forward);

/* returns the Parser of the associated index, or a NULL ptr if the environment has been freed / improperly set up */
Parser* getParserRef(ParserEnvironment* env, ParserIndex index);

/* like getNextCharacter but it takes forward to ensure saftey */
char getCharacter(ParserEnvironment* env, int forward);

void* getState(ParserEnvironment* env);

/* basically just to free context pointers in Satisfy, if they exist*/
void freeParserResources(Parser* parser);

/* same as `freeParserResources` but casted to void*, for LinkedList*/
void freeParserResourcesAsVoid(void* parser);

/* frees all parsers, and any inner memory they have,
 * after this the `ParserEnvironment` is unusable, and a new one must be created*/
void freeParserEnvironment(ParserEnvironment* env);

/* -ParsingFunctions-*/
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

/* Prints a ParsingResult, using `showSuccess` parameter for showing the void* in success*/
void showParsingResult(ParsingResult shower, void (*showSuccess) (void*));

/* runs a Parser for words, starting at index cursor, until a Pure value or an Err.
 * If its Pure, it returns ParserSuccess from the 
 * 	ParsingResult tagged union, with the void pointer created by its parse method
 * If its Err, it returns ParserError from the 
 * 	ParsingResult tagged union, with the error message from the Err*/
ParsingResult runParser(ParserEnvironment* env, ParserIndex parser, char* words, int cursor);

/* errors if a copy function has not been provided to env
 * create env with mkParserWithState to ensure this does not happen*/
ParsingResult runParserWithState(ParserEnvironment* env, ParserIndex parser, char* words, int cursor, void* initialState);

/* similar to `ParseResult` but supports multiple result
 * also doesn't have the move variable since its not needed*/
typedef struct {
	enum ParsingResultType type;
	union {
		char       error[ERROR_MSG_SIZE];
		LinkedList success;
	}                      value;
} ParseSequenceResult;

/* needs show for value, to be mapped over linked list*/
void showParsingSequenceResult(ParseSequenceResult shower, void (*showSuccess) (void*));

/* Parses until it runs into an error, or the text ends.
 * if it succeeded at least one time, it will return a success with all accumulated successes
 * if strict flag is enabled, it will fail unless if the error given is EOF error*/
ParseSequenceResult runParserUntil(ParserEnvironment* env, ParserIndex parser, char* words, int cursor, int strict);

/* errors if a copy function has not been provided to env
 * create env with mkParserWithState to ensure this does not happen*/
ParseSequenceResult runParserUntilWithState(ParserEnvironment* env, ParserIndex parser, char* words, int cursor, int strict, void* initialState);

/* applies a list of parsers, one after another. if `strict` flag is enabled, it will error unless all parsers succeed,
 * otherwise it will error only if the first parser fails*/
ParseSequenceResult runParsersInSequence(ParserEnvironment* env, ParserIndex* parsers, int length, char* words, int cursor, int strict);

/* errors if a copy function has not been provided to env
 * create env with mkParserWithState to ensure this does not happen*/
ParseSequenceResult runParsersInSequenceWithState(ParserEnvironment* env, ParserIndex* parsers, int length, char* words, int cursor, int strict, void* initialState);

/* -SomeParsers-*/
/* TODO split the specific parsers into another file*/

/* Pure parser that returns the String given, (it of course allocates it)*/
ParserIndex takeAsGiven(ParserEnvironment* env);

/* Pure parser that parses the String into an integer, or 0 if the parsing failed
 * use `numbers` to actually find the numbers to parse*/
ParserIndex pureInt(ParserEnvironment* env);

/* Satisfy predicate for being a digit*/
int isDigitPred(ParserEnvironment* _env, void* _context, char ch);

/* Satisfy parser for any digit, repeats*/
ParserIndex numbers(ParserEnvironment* env, int eat, ParserIndex success);
ParserIndex numbersWithoutError(ParserEnvironment* env, int eat, ParserIndex success, ParserIndex failure);

/* parses exactly the charcter given*/
ParserIndex singleChar(ParserEnvironment* env, char ch, int eat, int repeat, ParserIndex success);
ParserIndex singleCharWithoutError(ParserEnvironment* env, char ch, int eat, int repeat, ParserIndex success, ParserIndex failure);

#endif
