#ifndef AST_INCLUDED
#define AST_INCLUDED

#include "Tokenizer2.h"
#include "Parser2.h"

typedef enum {
        FunctionST,
        StringST,
        NumberST,
        AtomST,
        SymbolST,
} SyntaxToken;

/* atoms will be handled the same as symbols
 * since they need similar turning into index*/
typedef enum {
        AtomSymbol,
        VariableSymbol
} SymbolType;

typedef int SymbolIndex;

/* .name pointer is taken from token, and is shared by all
 * with the same symbol*/
typedef struct {
        char*       name;
        SymbolType  type;
        SymbolIndex index;
} Symbol;

typedef struct {
        int            argumentCount;
        struct Syntax** arguments;
        Symbol         symbol;
} Function;

typedef struct Syntax {
        SyntaxToken type;
        union {
                Function function;
                char*    string;
                long     number;
                Symbol   atom;
                Symbol   symbol;
        } value;
} Syntax;

void showSyntax(const Syntax* syntax, int depth);
void showUnAST(Syntax* syntax, int newline);
Syntax* mkStringST(char* value);
Syntax* mkNumberST(long value);

/* defined flag starts at false for buildAST
 * so it can form a sort of checklist for seeing if every function is properly defined*/
typedef struct SyntaxMap {
        Symbol            symbol;
        int               defined;
	int               references;
        struct SyntaxMap* next;
} SyntaxMap;

typedef struct {
        SyntaxMap*  map;
        SymbolIndex nextIndex;
} SyntaxEnv;

/* rest of the mk{SyntaxToken} functions that rely on SyntaxEnv**/
Syntax* mkAtomST(char** value, SyntaxEnv* env, int plsFree);
Syntax* mkSymbolST(char** value, SyntaxEnv* env, int plsFree);
Syntax* mkFunctionST(char** value, int argumentCount, SyntaxEnv* env, int plsFree);



void showSyntaxEnv(SyntaxEnv* env);
SyntaxEnv mkSyntaxEnv();
/* returns the corresponding symbol to the SyntaxEnv, 
 * or adds it if it's not a member
 * if plsFree is enabled it will free the duplicate name*/
Symbol findOrRegister(SyntaxEnv* env, char** search, SymbolType type, int plsFree, int incr);

typedef struct {
        enum {
                ProcessSuccess,
                ProcessError
        }      type;
        Cons*  next; /* should be rest of tokens after previous token*/
        Token* previousToken;
        union {
                char    error[ERROR_MSG_SIZE];
                Syntax* success;
        } value;
} ProcessResult;

void showProcessResult(const ProcessResult* pr);

/* create the environment with mkSyntaxEnv
 * and tokens with tokenize from Tokenizer.c*/
ProcessResult processTokens(Cons* tokens, SyntaxEnv* env);

typedef struct {
        Syntax**   syntaxes;
        int        syntaxesLength;
} AST;

typedef enum {
        TokenError, /* error carried over from tokenization*/
        SyntaxError,
        SyntaxSuccess
} SyntaxResultType;

typedef struct {
        SyntaxResultType type;
        union {
                char tokenError[ERROR_MSG_SIZE];
                char syntaxError[ERROR_MSG_SIZE];
                AST  success;
        } value;
} SyntaxResult;

void showSyntaxResult(const SyntaxResult* sr);
SyntaxResult buildAST(char* words, SyntaxEnv* env);

#endif
