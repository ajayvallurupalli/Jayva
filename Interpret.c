#include "String.h"
#include "Macro.h"
#include "Macros.h"
#include "AST.h"

#define PRIMITIVE_FORCE_INDEX 0

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

Value mkInternalFunction(int macro, RunInternal internal) {
	Value result;
	FunctionVal func;

	result.type = FunctionValue;
	func.type = InternalFunction;
	func.macro = macro;
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

void addForcePrimitive(Namespace* namespace, SyntaxEnv* env) {
	Value force = mkInternalFunction(FALSE, &runIdentity);
	static char* name = "force";

	addToNamespace(name, VariableSymbol, force, env, namespace);
}

Namespace* mkBaseNamespace(SyntaxEnv* env) {
	Namespace* result = malloc(sizeof(Namespace));
	
	result->outer = NULL;
	addForcePrimitive(result, env);

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

typedef struct {
	enum {
		InterpretSuccess,
		UndefinedSymbol,
		InvalidArgs
	} type;
	union {
		Value   success;
		Syntax* undefined;
		char*   invalidArgs;
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
	}
}

InterpretResult interpretSyntax(Syntax* syntax, Namespace* namespace, SyntaxEnv* env, int force);

InterpretResult runFunction(FunctionVal* function, SymbolIndex index, Syntax* syntax, Namespace* namespace, SyntaxEnv* env, int force) {
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
		else {
			i = syntax->value.function.argumentCount;
		}
	}

	if (result.type == InterpretSuccess) {
		if (function->type == InternalFunction) {
			execution = function->value.internal(&writeSuccess, &writeError, arguments, i);
			if (execution == FunctionSuccess) {
				result.value.success = writeSuccess;
			}
			else {
				result.type = InvalidArgs;
				result.value.invalidArgs = writeError;
			}
		} else {
			printf("TODO: \n");
		}
	}

	free(arguments);

	return result;
}

InterpretResult interpretFunctionST(Syntax* syntax, Namespace* namespace, SyntaxEnv* env, int force) {
	InterpretResult result;
	Value success;
	SymbolDict* search;

	if (force || syntax->value.function.symbol.index == PRIMITIVE_FORCE_INDEX) {
		search = searchNamespace(namespace, syntax->value.function.symbol.index);
		if (!search) {
			printf("Search failed\n");
			result.type = UndefinedSymbol;
			result.value.undefined = syntax;
		} else result = runFunction(&search->value.value.function, search->symbol.index, syntax, namespace, env, force);
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
		result.type = InterpretSuccess;
		result.value.success = search->value;
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

FunctionResult addFun(Value* success, char** error, Value* values, int length) {
	int i;
	long count = 0;
	static char onlyNums[] = "Cannot add non-numbers";

	for (i = 0; i < length; i++) {
		if (values[i].type != NumberValue) {
			*error = onlyNums;
			return FunctionError;
		} else {
			count += values[i].value.number;
		}
	}

	success->type = NumberValue;
	success->value.number = count;
	return FunctionSuccess;
}

void insertAdd(SyntaxEnv* env, Namespace* namespace) {
	Value add = mkInternalFunction(FALSE, &addFun);
	
	static char* sym = "+";
	addToNamespace(sym, VariableSymbol, add, env, namespace);
}

/* todo 
 * encode primitives
 * primitives: force, lambda, define
 * hardcode fixed naming for lambda and define
 * */
int main() {
        char lispCode[] = "(force (sum 1 2 3 4 5 6 7 8 9 10))";
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

		printf("Interpreting!\n");
		showSyntax(ast.value.success.syntaxes[0], 1);
	
		insertAdd(&env, namespace);
		printf("Global namespace: \n");
		showSymbolDict(namespace->symbols, 1);
		
		InterpretResult interpret = interpretSyntax(ast.value.success.syntaxes[0], namespace, &env, FALSE);
		showInterpretResult(&interpret, 0);
        }       
}  
