#include "AST.h"
#include "Macro.h"
#include "Macros.h"

Syntax runConstMacro(Syntax* syntax, SyntaxEnv* env) {
        Syntax result;

	result = *syntax->value.function.arguments[0];
	cullSyntax(syntax, env, FALSE, FALSE);
	cullSyntax(syntax->value.function.arguments[1], env, TRUE, TRUE);


	printf("const returning: \n");
	return result;
}

Macro mkConstMacro() {
        Macro result;

        static MacroPattern pattern[2] = {SinglePattern, SinglePattern};
        result.pattern = pattern;
        result.patternLength = 2;
        result.run = &runConstMacro;

        static char name[] = "const";
        result.name = name;

        return result;
}

Syntax runVargAddMacro(Syntax* syntax, SyntaxEnv* env) {
        static char* innerOp = "+";
        Syntax* new;
        Syntax* result;
        int argCount = syntax->value.function.argumentCount;

        if (argCount > 1) {
                new = mkFunctionST(&innerOp, 2, env, FALSE);

                new->value.function.arguments[1] = \
                        syntax->value.function.arguments[argCount - 1];
                syntax->value.function.argumentCount--;
                argCount--;

                new->value.function.arguments[0] = syntax->value.function.arguments[argCount - 1];
                syntax->value.function.arguments[argCount - 1] = new;
                /* basically uses the end as an accumulator for the expression
                 * so it can be tail recursive to avoid issue with infintely recursion*/
                result = syntax;
        } else {
                result = syntax->value.function.arguments[0];
                cullSyntax(syntax, env, FALSE, FALSE);
        }

        return *result;
}

Macro mkVargAddMacro() {
        Macro result;

        static MacroPattern pattern[2] = {SinglePattern, RestPattern};
        result.pattern = pattern;
        result.patternLength = 2;
        result.run = &runVargAddMacro;

        static char name[] = "sum";
        result.name = name;

        return result;
}

MacroDict* predefinedMacros() {
	MacroDict* result = mkMacroDict(mkConstMacro());
	Macro vargAdd = mkVargAddMacro();
	addMacro(result, &vargAdd);

	return result;
}

