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

/* first Value* is where the result should be written to, char* is where error is written
 * this allows us to keep memory allocated on the stack 
 * kind of innovative idea i just invented (first person in the world likely)
 * will be interesting to see if this trick could be used for optimizations in Syntax
 * but I think its already pretty efficent because it commandeers memory (mostly)
 * but maybe macros would benefit?*/
typedef FunctionResult (*RunInternal) (struct Value*, char**, struct Value*, int length);

typedef struct {
	FunctionType type;
	int args; /* if args < 0, takes unlimited*/
	int macro;
	union {
		RunInternal internal;
		Syntax*     userDefined;
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

Value mkInternalFunction(int macro, int args, RunInternal internal) {
	Value result;
	FunctionVal func;

	result.type = FunctionValue;
	func.type = InternalFunction;
	func.macro = macro;
	func.args = args;
	func.value.internal = internal;
	result.value.function = func;

	return result;
}

void showValue(Value* value, int depth) {
	int i;

	for (i = 0; i < depth; i++) printf("\t");
	switch(value->type) {
		case NumberValue:
			printf("[NumberVal{value: %ld}]\n", value->value.number);
			break;
		case StringValue:
			printf("[StringVal{value: %s}]\n", value->value.string.data);
			break;
		case AtomValue:
			printf("[AtomVal{value: %d}]\n", (int) value->value.atom);
			break;
		case FunctionValue:
			if (value->value.function.type == InternalFunction) {
				printf("[FunctionVal{Internal %s}]\n", \
						value->value.function.macro ? "Macro" : "Function");
			} else {
				printf("[FunctionVal{User %s, ast: (\n", \
						value->value.function.macro ? "Macro" : "Function");
				showSyntax(value->value.function.value.userDefined, depth + 1);
				for (i = 0; i < depth; i++) printf("\t");
				printf(")}]\n");
			}
			break;
		case LazyValue: 
			printf("[LazyVal{ast: (\n");
			showSyntax(value->value.lazy, depth + 1);
			for (i = 0; i < depth; i++) printf("\t");
			printf(")}]\n");
	}
}

int isBoolean(Value v) {
	return v.type == AtomValue && \
		       (v.value.atom == PRIMITIVE_TRUE_INDEX || v.value.atom == PRIMITIVE_FALSE_INDEX);
}

typedef struct SymbolDict {
	Symbol             symbol;
	Value              value;
	struct SymbolDict* next;
} SymbolDict;

void showSymbolDict(SymbolDict* dict, int depth) {
	int i;

	for (i = 0; i < depth; i++) printf("\t");
	switch (dict->value.type) {
		case NumberValue: 
			printf("[NumberVal@%d{name: %s, value: %ld}]\n", \
					(int) dict->symbol.index, dict->symbol.name, \
					dict->value.value.number);
			break;
		case StringValue: 
			printf("[StringVal@%d{name: %s, value: %s}]\n", \
					(int) dict->symbol.index, dict->symbol.name, \
					dict->value.value.string.data);
			break;
		case AtomValue: 
			printf("[AtomVal@%d{name: %s}]\n", \
					(int) dict->symbol.index, dict->symbol.name);
			break;
		case FunctionValue:
			if (dict->value.value.function.type == InternalFunction) {
				printf("[FunctionVal@%d{name: %s, type: Internal}]\n", \
						(int) dict->symbol.index, dict->symbol.name);
			} else {
				printf("[FunctionVal@%d{name: %s, type: User, ast: (\n", \
						(int) dict->symbol.index, dict->symbol.name);
				showSyntax(dict->value.value.function.value.userDefined, depth + 1); /* cinema*/
				for (i = 0; i < depth; i++) printf("\t");
				printf(")}]\n");
			}
			break;
		case LazyValue:
			printf("[LazyVal@%d{name: %s, ast: (\n", \
					(int) dict->symbol.index, dict->symbol.name);
			showSyntax(dict->value.value.lazy, depth + 1);
			for (i = 0; i < depth; i++) printf("\t");
			printf(")}]\n");
			break;
	}

	if (dict->next) showSymbolDict(dict->next, depth);
}

SymbolDict mkSymbolDict(Symbol symbol, Value value) {
	SymbolDict result;
	result.symbol = symbol;
	result.value = value;
	result.next = NULL;

	return result;
}

/* global scope if namepace->outer == NULL*/
typedef struct Namespace {
	SymbolDict*       symbols;
	struct Namespace* outer;
} Namespace;

void addToNamespace(char* name, SymbolType type, Value value, SyntaxEnv* env, Namespace* namespace) {
	SymbolDict* recurse = namespace->symbols;
	
	if (!recurse) {
		namespace->symbols = malloc(sizeof(SymbolDict));
		*namespace->symbols = mkSymbolDict(findOrRegister(env, &name, type, FALSE, FALSE), value);
	} else {
		while(recurse->next) recurse = recurse->next;
		recurse->next = malloc(sizeof(SymbolDict));
		*recurse->next =  mkSymbolDict(findOrRegister(env, &name, type, FALSE, FALSE), value);
	}
}

/* looks for the index and sets it to new
 * if there are multiple occurences it sets the newest one highest on the function stack*/
void setNamespace(SymbolIndex index, Value new, Namespace* namespace) {
	SymbolDict* recurse = namespace->symbols;
	int set = FALSE;

	while (recurse && !set) {
		if (recurse->symbol.index == index) {
			recurse->value = new;
		} else recurse = recurse->next;
	}

	if (!set && namespace->outer) setNamespace(index, new, namespace->outer);
}

/* local functional programmer learns how to not use recursion for everything*/
void freeNamespace(Namespace* namespace, int recursive) {
	SymbolDict* recurse = namespace->symbols;
	SymbolDict* next = namespace->symbols; /*basically temp*/

	while (recurse) {
		next = recurse->next;
		free(recurse);
		recurse = next;
	} 

	if (recursive && namespace->outer) freeNamespace(namespace->outer, recursive);
}

FunctionResult runIdentity(Value* write, char** error, Value* values, int length) {
	static char* argError = "Identity requires 1 argument";

	if (length < 1) {
		*error = argError;
		return FunctionError;
	} else {
		*write = values[0];
		return FunctionSuccess;
	}
}	

void addForcePrimitive(SyntaxEnv* env, Namespace* namespace) {
	Value force = mkInternalFunction(FALSE, 1, &runIdentity);
	static char* name = "force";

	addToNamespace(name, VariableSymbol, force, env, namespace);
}

void addIdentity(SyntaxEnv* env, Namespace* namespace) {
	Value identity = mkInternalFunction(FALSE, 1, &runIdentity);
	static char* name = "id";

	addToNamespace(name, VariableSymbol, identity, env, namespace);
}

void addDefinePrimitive(SyntaxEnv* env, Namespace* namespace) {
	Value define = mkInternalFunction(FALSE, 2, NULL);
	static char* name = "define";

	addToNamespace(name, VariableSymbol, define, env, namespace);
}

void addLambdaPrimitive(SyntaxEnv* env, Namespace* namespace) {
	Value lambda = mkInternalFunction(FALSE, 2, NULL);
	static char* name = "lambda";

	addToNamespace(name, VariableSymbol, lambda, env, namespace);
}

void addIfPrimitive(SyntaxEnv* env, Namespace* namespace) {
	Value _if = mkInternalFunction(FALSE, 3, NULL);
	static char* name = "if";
	static char* _false = "false";
	static char* _true = "true";

	addToNamespace(name, VariableSymbol, _if, env, namespace);
	findOrRegister(env, &_true, AtomSymbol, FALSE, FALSE);
	findOrRegister(env, &_false, AtomSymbol, FALSE, FALSE);
}

Namespace* mkBaseNamespace(SyntaxEnv* env) {
	Namespace* result = malloc(sizeof(Namespace));
	
	result->outer = NULL;
	addForcePrimitive(env, result);
	addDefinePrimitive(env, result);
	addLambdaPrimitive(env, result);
	addIfPrimitive(env, result);

	return result;
}

SymbolDict* searchNamespace(Namespace* namespace, SymbolIndex search) {
	SymbolDict* recurse = namespace->symbols;
	SymbolDict* result = NULL;

	while (recurse && !result) {
		if (recurse->symbol.index == search) {
			result = recurse;
		} 

		recurse = recurse->next;
	}

	if (result) return result;
	else if (namespace->outer) return searchNamespace(namespace->outer, search);
	else return NULL;
}

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

void showInterpretResult(InterpretResult* result, int depth) {
	int i;

	for (i = 0; i < depth; i++) printf("\t");

	switch (result->type) {
		case InterpretSuccess:
			printf("Interpretation succeded with value: \n");
			showValue(&result->value.success, depth + 1);
			break;
		case UndefinedSymbol:
			printf("Symbol is undefined: \n");
			showSyntax(result->value.undefined, depth + 1);
			break;
		case InvalidArgs:
			printf("Invalid Args: %s\n\n", result->value.invalidArgs);
			break;
		case ArityMismatch:
			printf("Arity Mismatch: \n");
			printf("Expected %d args but given %d, in: \n", \
					result->value.arityMismatch.argsExpected, result->value.arityMismatch.argsGiven);
			showSyntax(result->value.arityMismatch.syntax, 1);
	}
}

InterpretResult interpretSyntax(Syntax* syntax, Namespace* namespace, SyntaxEnv* env, int force);

/* doesn't do any checking what so ever
 * so make sure to check!!!*/
char* getArgumentName(Syntax* function, int index) {
	char* result;

	if (index == 0) result = function->value.function.arguments[0]->value.function.symbol.name;
	else result = function->value.function.arguments[0]->value.function.arguments[index - 1]->value.symbol.name;

	return result;
}

InterpretResult runFunctionInternal(FunctionVal* function, Syntax* syntax, Namespace* namespace, SyntaxEnv* env, int force) {
	InterpretResult result;
	FunctionResult execution;
        Value* arguments = malloc(sizeof(Value) * syntax->value.function.argumentCount);
	Value writeSuccess;
	char* writeError = NULL;
	int i;

	result.type = InterpretSuccess;

	for (i = 0; i < syntax->value.function.argumentCount; i++) {
		result = interpretSyntax(syntax->value.function.arguments[i], \
					namespace, env, TRUE);
		if (result.type == InterpretSuccess) arguments[i] = result.value.success;
		else i = syntax->value.function.argumentCount;
        }

	if (result.type == InterpretSuccess) {
		execution = function->value.internal(&writeSuccess, &writeError, arguments, i);
                
		if (execution == FunctionSuccess) {
                        result.value.success = writeSuccess;
                } else {
                        result.type = InvalidArgs;
                        result.value.invalidArgs = writeError; /* should be not null now by func*/
                }
	}

	free(arguments);

	return result;
}

InterpretResult runFunctionUser(Syntax* function, Syntax* syntax, Namespace* namespace, SyntaxEnv* env, int force) {
	InterpretResult result;
	int i;

	Namespace* new = malloc(sizeof(Namespace));
	new->symbols = NULL;
	new->outer = namespace;

	for (i = 0; i < syntax->value.function.argumentCount; i++) {
		result = interpretSyntax(syntax->value.function.arguments[i], \
				namespace, env, force);
		if (result.type == InterpretSuccess) \
			addToNamespace(getArgumentName(function, i), VariableSymbol, result.value.success, env, new);
		else i = syntax->value.function.argumentCount;
	}

	if (result.type == InterpretSuccess) \
		result = interpretSyntax(function->value.function.arguments[1], new, env, force);


	freeNamespace(new, FALSE);
	free(new);

	return result;
}

InterpretResult runFunction(FunctionVal* function, SymbolIndex index, Syntax* syntax, Namespace* namespace, SyntaxEnv* env, int force) {
	InterpretResult result;

	result.type = InterpretSuccess;

	if (function->type == InternalFunction) result = runFunctionInternal(function, syntax, namespace, env, force);
	else result = runFunctionUser(function->value.userDefined, syntax, namespace, env, force);


	return result;
}

InterpretResult interpretDefine(Syntax* syntax, Namespace* namespace, SyntaxEnv* env) {
	InterpretResult result;
	Function* function = &syntax->value.function; /*just an alias*/
	static char invalidArgs[] = "define is of the form (define [symbol] *)";

	if (function->arguments[0]->type != SymbolST) {
		result.type = InvalidArgs;
		result.value.invalidArgs = invalidArgs;
	} else {
		result = interpretSyntax(function->arguments[1], namespace, env, FALSE);
		if (result.type == InterpretSuccess) {
			addToNamespace(function->arguments[0]->value.symbol.name, VariableSymbol, \
					result.value.success, env, namespace);
		}
	}

	return result;
}

InterpretResult interpretLambda(Syntax* syntax, Namespace* namespace, SyntaxEnv* env) {
	InterpretResult result;
	Function* function = &syntax->value.function;
	static char invalidArgs[] = "lambda is of the form (lambda (arg1 arg2 ...) (function ...))";

	if (function->arguments[0]->type != FunctionST || 
			function->arguments[1]->type != FunctionST) {
		result.type = InvalidArgs;
		result.value.invalidArgs = invalidArgs;
	} else {
		result.type = InterpretSuccess;
		/* i can't help but feel that the tagged unions are a little too much*/
		result.value.success.value.function.type = UserDefinedFunction;
		result.value.success.value.function.args = function->arguments[0]->value.function.argumentCount + 1;
		result.value.success.value.function.value.userDefined = syntax;
	}

	return result;
}

InterpretResult interpretIf(Syntax* syntax, Namespace* namespace, SyntaxEnv* env, int force) {
	InterpretResult result;
	Function* function = &syntax->value.function;
	static char invalidArgs[] = "if cond mist evaluate to 'true or 'false";

	result = interpretSyntax(function->arguments[0], namespace, env, TRUE);
	if (result.type == InterpretSuccess) {
		if (!isBoolean(result.value.success)) {
			result.type = InvalidArgs;
			result.value.invalidArgs = invalidArgs;
		} else result = \
			result.value.success.value.atom == PRIMITIVE_TRUE_INDEX ? \
				interpretSyntax(function->arguments[1], namespace, env, force) : \
				interpretSyntax(function->arguments[2], namespace, env, force);
	}

	return result;
}

/* must be given a FunctionST or it explodes */
InterpretResult interpretFunctionST(Syntax* syntax, Namespace* namespace, SyntaxEnv* env, int force) {
	InterpretResult result;
	Value success;
	SymbolDict* search;
	SymbolIndex index = syntax->value.function.symbol.index;

	if (index == PRIMITIVE_IF_INDEX) {
		result = interpretIf(syntax, namespace, env, force);
	} else if (index == PRIMITIVE_LAMBDA_INDEX) {
		result = interpretLambda(syntax, namespace, env);	
	} else if (index == PRIMITIVE_DEFINE_INDEX) {
		result = interpretDefine(syntax, namespace, env);	
	} else if (force || index == PRIMITIVE_FORCE_INDEX) {
		search = searchNamespace(namespace, index);
		if (!search) {
			result.type = UndefinedSymbol;
			result.value.undefined = syntax;
		} else {
			if (search->value.value.function.args != syntax->value.function.argumentCount && \
					search->value.value.function.args >= 0) { /* <= means not unlimited*/
				result.type = ArityMismatch;
				result.value.arityMismatch.argsGiven = syntax->value.function.argumentCount;
				result.value.arityMismatch.argsExpected = search->value.value.function.args;
				result.value.arityMismatch.syntax = syntax;
			} else {
				result = runFunction(&search->value.value.function, search->symbol.index, syntax, namespace, env, force);
		
			}
		}
	} else {
		success.type = LazyValue;
		success.value.lazy = syntax;
		result.type = InterpretSuccess;
		result.value.success = success;
	}

	return result;
} 

InterpretResult interpretSymbolST(Syntax* syntax, Namespace* namespace, SyntaxEnv* env, int force) {
	InterpretResult result;
	SymbolDict* search;

	search = searchNamespace(namespace, syntax->value.symbol.index);
	if (!search) {
		result.type = UndefinedSymbol;
		result.value.undefined = syntax;
	} else {
		if (force && search->value.type == LazyValue) {
			result = interpretSyntax(search->value.value.lazy, namespace, env, TRUE);
		} else {
			result.type = InterpretSuccess;
			result.value.success = search->value;
		}
	}	

	return result;
}

InterpretResult interpretSyntax(Syntax* syntax, Namespace* namespace, SyntaxEnv* env, int force) {
	InterpretResult result;
	Value success;

	switch (syntax->type) { /* commandeers pointer*/
		case StringST:
			success.type = StringValue;
			success.value.string.data = syntax->value.string;
			success.value.string.length = lengthOfChars(syntax->value.string);
			success.value.string.capacity = success.value.string.length;
			result.value.success = success;
			result.type = InterpretSuccess;
			break;
		case NumberST:
			success.type = NumberValue;
			success.value.number = syntax->value.number;
			result.value.success = success;
			result.type = InterpretSuccess;
			break;
		case AtomST:
			success.type = AtomValue;
			success.value.atom = syntax->value.atom.index;
			result.value.success = success;
			result.type = InterpretSuccess;
			break;
		case FunctionST:
			result = interpretFunctionST(syntax, namespace, env, force);
			break;
		case SymbolST:
			result = interpretSymbolST(syntax, namespace, env, force);
			break;

	}

	return result;
}

InterpretResult interpretAST(AST* ast, Namespace* namespace, SyntaxEnv* env) {
	InterpretResult result;
	int i;

	result.type = InterpretSuccess;

	for (i = 0; i < ast->syntaxesLength && result.type == InterpretSuccess; i++) \
		result = interpretSyntax(ast->syntaxes[i], namespace, env, FALSE);
	

	return result;
}

FunctionResult addFun(Value* success, char** error, Value* values, int length) {
	int i;
	long count = 0;
	static char onlyNums[] = "Cannot add non-numbers";

	for (i = 0; i < length; i++) {
		if (values[i].type != NumberValue) {
			*error = onlyNums;
			return FunctionError;
		} else count += values[i].value.number;

	}

	success->type = NumberValue;
	success->value.number = count;
	return FunctionSuccess;
}

void insertAdd(SyntaxEnv* env, Namespace* namespace) {
	Value add = mkInternalFunction(FALSE, UNLIMITED_ARGS, &addFun);
	
	static char* sym = "+";
	addToNamespace(sym, VariableSymbol, add, env, namespace);
}

FunctionResult mulFun(Value* success, char** error, Value* values, int length) {
	int i; 
	long count = 1;
	static char onlyNums[] = "Cannot mul non-numbers";

	for (i = 0; i < length; i++) {
		if (values[i].type != NumberValue) {
			*error = onlyNums;
			return FunctionError;
		} else count *= values[i].value.number;
	}

	success->type = NumberValue;
	success->value.number = count;
	return FunctionSuccess;
}

void insertMul(SyntaxEnv* env, Namespace* namespace) {
        Value mul = mkInternalFunction(FALSE, UNLIMITED_ARGS, &mulFun);

        static char* sym = "*";
        addToNamespace(sym, VariableSymbol, mul, env, namespace);
}

FunctionResult subFun(Value* success, char** error, Value* values, int length) {
	static char onlyNums[] = "Cannot sub non-numbers";
	FunctionResult result;

	if (values[0].type != NumberValue || values[1].type != NumberValue) {
		*error = onlyNums;
		result = FunctionError;
	} else {
		success->type = NumberValue;
		success->value.number = values[0].value.number - values[1].value.number;
		result = FunctionSuccess;
	}

	return result;
}

void insertSub(SyntaxEnv* env, Namespace* namespace) {
        Value sub = mkInternalFunction(FALSE, 2, &subFun);

        static char* sym = "-";
        addToNamespace(sym, VariableSymbol, sub, env, namespace);
}

FunctionResult divFun(Value* success, char** error, Value* values, int length) {
	static char onlyNums[] = "Cannot div non-numbers";
	static char divZero[] = "Cannot divide by 0";
	FunctionResult result;

	if (values[0].type != NumberValue || values[1].type != NumberValue) {
		*error = onlyNums;
		result = FunctionError;
	} else if (values[1].value.number == 0L) {
		*error = divZero;
		result = FunctionError;
	} else {
		success->type = NumberValue;
		success->value.number = values[0].value.number / values[1].value.number;
		result = FunctionSuccess;
	}

	return result;
}

void insertDiv(SyntaxEnv* env, Namespace* namespace) {
        Value div = mkInternalFunction(FALSE, 2, &divFun);

        static char* sym = "/";
        addToNamespace(sym, VariableSymbol, div, env, namespace);
}

FunctionResult positivePred(Value* success, char** error, Value* values, int length) {
	static char onlyNums[] = "positive? only works on numbers";
	FunctionResult result;

	if (values[0].type != NumberValue) {
		*error = onlyNums;
		result = FunctionError;
	} else {
		success->type = AtomValue;
		success->value.atom = values[0].value.number > 0 ? \
				      PRIMITIVE_TRUE_INDEX : PRIMITIVE_FALSE_INDEX;
		result = FunctionSuccess;
	}

	return result;
}

void insertPositivePred(SyntaxEnv* env, Namespace* namespace) {
	Value positive = mkInternalFunction(FALSE, 1, &positivePred);

	static char* sym = "positive?";
	addToNamespace(sym, VariableSymbol, positive, env, namespace);
}

FunctionResult lessThan(Value* success, char** error, Value* values, int lrngth) {
	static char onlyNums[] = "< only works on numbers";
	FunctionResult result;

	if (values[0].type != NumberValue || values[1].type != NumberValue) {
		*error = onlyNums;
		result = FunctionError;
	} else {
		success->type = AtomValue;
		success->value.atom = values[0].value.number < values[1].value.number ? \
		      PRIMITIVE_TRUE_INDEX : PRIMITIVE_FALSE_INDEX;
		result = FunctionSuccess;
	}

	return result;
}

FunctionResult lessThanOrEqual(Value* success, char** error, Value* values, int lrngth) {
        static char onlyNums[] = "<= only works on numbers";
        FunctionResult result;


        if (values[0].type != NumberValue || values[1].type != NumberValue) {
                *error = onlyNums;
                result = FunctionError;
        } else {
                success->type = AtomValue;
                success->value.atom = values[0].value.number <= values[1].value.number ? \
                      PRIMITIVE_TRUE_INDEX : PRIMITIVE_FALSE_INDEX;
                result = FunctionSuccess;
        }

        return result;
}

FunctionResult greaterThan(Value* success, char** error, Value* values, int lrngth) {
        static char onlyNums[] = "> only works on numbers";
        FunctionResult result;

        if (values[0].type != NumberValue || values[1].type != NumberValue) {
                *error = onlyNums;
                result = FunctionError;
        } else {
                success->type = AtomValue;
                success->value.atom = values[0].value.number > values[1].value.number ? \
                      PRIMITIVE_TRUE_INDEX : PRIMITIVE_FALSE_INDEX;
                result = FunctionSuccess;
        }

        return result;
}

FunctionResult greaterThanOrEqual(Value* success, char** error, Value* values, int lrngth) {
        static char onlyNums[] = ">= only works on numbers";
        FunctionResult result;

        if (values[0].type != NumberValue || values[1].type != NumberValue) {
                *error = onlyNums;
                result = FunctionError;
        } else {
                success->type = AtomValue;
                success->value.atom = values[0].value.number >= values[1].value.number ? \
                      PRIMITIVE_TRUE_INDEX : PRIMITIVE_FALSE_INDEX;
                result = FunctionSuccess;
        }

        return result;
}

void insertComparisons(SyntaxEnv* env, Namespace* namespace) {
        Value lt  = mkInternalFunction(FALSE, 2, &lessThan);
	Value lte = mkInternalFunction(FALSE, 2, &lessThanOrEqual);
	Value gt  = mkInternalFunction(FALSE, 2, &greaterThan);
	Value gte = mkInternalFunction(FALSE, 2, &greaterThanOrEqual);

	static char* lts  = "<";
        static char* ltes = "<=";
	static char* gts  = ">";
	static char* gtes = ">=";
        addToNamespace(lts, VariableSymbol, lt, env, namespace);
	addToNamespace(ltes, VariableSymbol, lte, env, namespace);
	addToNamespace(gts, VariableSymbol, gt, env, namespace);
	addToNamespace(gtes, VariableSymbol, gte, env, namespace);
}

/* todo 
 * encode primitives
 * primitives: force (complete), lambda, define (complete)
 * hardcode fixed naming for lambda and define (maybe not neccessary
 * )
 * */
int main() {
        char lispCode[] = "(define fib (lambda (x) (if (> x 2) (+ (fib (- x 1)) (fib (- x 2))) 1))) (force (fib 10))";
        SyntaxEnv    env = mkSyntaxEnv();
	Namespace* namespace = mkBaseNamespace(&env);
        SyntaxResult ast = buildAST(lispCode, &env);
        
        MacroDict* dict = predefinedMacros();
        
        showMacroDict(dict);
        
        MacroResult result = validMacroDict(dict);
        showMacroResult(&result);
        
        if (ast.type == SyntaxSuccess) {
                
                result = runMacroDictOnAST(dict, &ast.value.success, &env);        
                printf("ran macros: \n");
                showMacroResult(&result);
                showSyntaxResult(&ast);
                showSyntaxEnv(&env);

		addIdentity(&env, namespace);
		insertAdd(&env, namespace);
		insertMul(&env, namespace);
		insertSub(&env, namespace);
		insertDiv(&env, namespace);
		insertPositivePred(&env, namespace);
		insertComparisons(&env, namespace);
		printf("Global namespace: \n");
		showSymbolDict(namespace->symbols, 1);
		printf("Syntax env: \n");
		showSyntaxEnv(&env);
		
		InterpretResult interpret = interpretAST(&ast.value.success, namespace, &env);
		printf("Final result: \n\n");
		showInterpretResult(&interpret, 0);
	}       
}  
