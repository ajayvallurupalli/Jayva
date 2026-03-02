#ifndef MACRO_INCLUDED
#define MACRO_INCLUDED

#include "AST.h"

typedef enum {
        SinglePattern, /* x */
        GroupPattern,  /* (...) */
        QuotePattern, /* like Group but captures function, might be useful for user-made macros*/
        RestPattern    /* ... */
} MacroPattern;

/* every macro matches with a given array of patterns
 * currently only one macro is allowed per name
 * the run function evaluates the macro, returning the new AST
 * it must cull any elements it removes, with `cullSyntax` and the given env
 * and it must register any elements and the given env
 *** it adds with `findOrRegister` (in `AST.h`)
 *** and inserted into the SyntaxEnv
 *** if it it should set the int* given to TRUE*/
typedef struct {
        MacroPattern* pattern;
        int           patternLength;
        Syntax        (*run) (Syntax*, SyntaxEnv*);
        char*         name;
} Macro;

void showMacro(Macro* macro, int depth);


/* removes a syntax from the environemnt before freeing it
 * removes all occurences in the syntax env
 * and removes all occurences of childeren syntaxes (ie function arguments)
 * this must be used for all macros that remove parts of the tree
 * to ensure memory saftey and SyntaxEnv correctnesss
 * maybe should be in AST.h?*/
void cullSyntax(Syntax* syntax, SyntaxEnv* env, int cullChildren, int freeOnDeletion);

int minPatternLength(MacroPattern* pattern, int length);
int validMacro(Macro* macro);
int matchMacro(Macro* macro, Syntax* syntaxes, int length);

typedef enum {
        InvalidMacro, /* a macro was invalid to begin with, by `validMacro`*/
        NotEvenAMacro, /* somehow matched with a non macro, likely by name collision*/
        InvalidMacroArguments, /* pattern match failed*/
        SuccessMacro
} MacroResultType;

typedef struct {
        MacroResultType type;
        union { /* for error types, we give the element that caused the error*/
                Macro*  invalidMacro;
                Syntax* notEvenAMacro;
                Syntax* invalidArguments;
        } value;
} MacroResult;

void showMacroResult(MacroResult* result);

typedef struct MacroDict {
        Macro macro;
        struct MacroDict* next;
} MacroDict;

MacroDict* mkMacroDict(Macro first);
void showMacroDict(MacroDict* dict);
MacroResult validMacroDict(MacroDict* dict);
void addMacro(MacroDict* dict, Macro* macro);

MacroResult runMacro(Syntax* syntax, Macro* macro, MacroDict* all, SyntaxEnv* env);
MacroResult runMacroDict(MacroDict* macros, Syntax* ast, SyntaxEnv* env);
MacroResult runMacroDictOnAST(MacroDict* macros, AST* ast, SyntaxEnv* env);




#endif
