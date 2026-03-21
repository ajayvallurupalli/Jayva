#include "String.h"
#include "Macro.h"
#include "Macros.h"
#include "AST.h"

#define PRIMITIVE_FORCE_INDEX 0
#define PRIMITIVE_DEFINE_INDEX 1
#define PRIMITIVE_LAMBDA_INDEX 2
#define PRIMITIVE_IF_INDEX 3
#define PRIMITIVE_TRUE_INDEX 4
#define PRIMITIVE_FALSE_INDEX 5
#define PRIMITIVE_EQ_INDEX 6
#define UNLIMITED_ARGS (-1)


/* like the 5th tagged union at this point?
 * idk it its too repetitive but it does work well
 * as a design pattern*/
typedef enum {
	NumberValue,
	StringValue,
	AtomValue,
	FunctionValue,
	LazyValue
} ValueType;

struct Value;

typedef enum {
	InternalFunction,
	UserDefinedFunction
} FunctionType;

typedef enum {
	FunctionSuccess,
	FunctionError
} FunctionResult;

struct Namespace;

/* first Value* is where the result should be written to, char* is where error is written
 * this allows us to keep memory allocated on the stack 
 * kind of innovative idea i just invented (first person in the world likely)
 * will be interesting to see if this trick could be used for optimizations in Syntax
 * but I think its already pretty efficent because it commandeers memory (mostly)
 * but maybe macros would benefit?*/
typedef FunctionResult (*RunInternal) (struct Value*, char**, struct Value*, int length, SyntaxEnv* env, struct Namespace* namespace);

typedef struct {
	FunctionType type;
	int args; /* if args < 0, takes unlimited*/
	int macro;
	union {
		RunInternal internal;
		struct {
			Syntax*    syntax;
			struct Namespace* namespace;
		} userDefined;
	} value;
} FunctionVal;

/* note: Value struct is the actual value, while
 * so FunctionST of Syntax describes the syntax of a function
 * while FunctionValue of Value describes its execution*/
typedef struct Value {
	ValueType type;
	union {
		long        number;
		String      string;
		SymbolIndex atom;
		FunctionVal function;
		Syntax*     lazy;
	} value;
} Value;

Value mkInternalFunction(int macro, int args, RunInternal internal);
void showValue(Value* value, int depth);
int isBoolean(Value v);

typedef struct SymbolDict {
	Symbol             symbol;
	Value              value;
	struct SymbolDict* next;
} SymbolDict;

void showSymbolDict(SymbolDict* dict, int depth);
SymbolDict mkSymbolDict(Symbol symbol, Value value);

/* global scope if namepace->outer == NULL*/
typedef struct Namespace {
	SymbolDict*       symbols;
	struct Namespace* outer;
	int               references;
} Namespace;

void addToNamespace(char* name, SymbolType type, Value value, SyntaxEnv* env, Namespace* namespace);
void setNamespace(SymbolIndex index, Value new, Namespace* namespace);
void freeNamespace(Namespace* namespace);
/* adds force, define, lambda, and if primitives*/
Namespace* mkBaseNamespace(SyntaxEnv* env);
void insertBasicFunctions(Namespace* namespace, SyntaxEnv* env);

SymbolDict* searchNamespace(Namespace* namespace, SymbolIndex search);

/* in the future I want errors to be more structured like this*/
typedef struct {
	int argsExpected;
	int argsGiven;
	Syntax* syntax;
} ArityMismatchError;

typedef struct {
	enum {
		InterpretSuccess,
		UndefinedSymbol,
		InvalidArgs,
		ArityMismatch
	} type;
	union {
		Value              success;
		Syntax*            undefined;
		char*              invalidArgs;
		ArityMismatchError arityMismatch;
	} value;
} InterpretResult;
void showInterpretResult(InterpretResult* result, int depth);

/* the magnum opus*/
InterpretResult interpretAST(AST* ast, Namespace* namespace, SyntaxEnv* env);
