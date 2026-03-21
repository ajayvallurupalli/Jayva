#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "Macros.h"
#include "Macro.h"
#include "AST.h"
#include "Interpret.h"

const char* readFile(const char* path) {
	int file = open(path, O_RDONLY);

	struct stat fileStats;
	fstat(file, &fileStats);
	
	const char* result = mmap(NULL, fileStats.st_size, PROT_READ, MAP_PRIVATE, file, 0);
	close(file);

	return result;
}

/* todo 
 * encode primitives
 * primitives: force (complete), lambda, define (complete)
 * hardcode fixed naming for lambda and define (maybe not neccessary
 * )
 * */
int main() {
	const char* lispCode = readFile("./test.cl");
	printf("read: \n%s", lispCode);
	SyntaxEnv    env = mkSyntaxEnv();
	Namespace*   namespace = mkBaseNamespace(&env);
	SyntaxResult ast = buildAST((char*) lispCode, &env);

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

		insertBasicFunctions(namespace, &env);

		printf("Global namespace: \n");
		showSymbolDict(namespace->symbols, 1);
		printf("Syntax env: \n");
		showSyntaxEnv(&env);

		for (int i = 0; i < ast.value.success.syntaxesLength; i++) {
			showUnAST(ast.value.success.syntaxes[i], TRUE);
		}

		InterpretResult interpret = interpretAST(&ast.value.success, namespace, &env);
		printf("Final result: \n\n");
		showInterpretResult(&interpret, 0);

		//freeNamespace(namespace, FALSE);
	}       
}
