
CC = gcc
CFLAGS = -Wall -Werror -g

run: Main.x
	clear
	clear
	./Main.x

Main.x: Main.o
	$(CC) Main.o String.o LinkedList.o Parser2.o Tokenizer2.o AST.o Macro.o Macros.o Interpret.o -o Main.x

Main.o: Main.c Macro.o Macros.o AST.o Interpret.o
	$(CC) $(CFLAGS) -c Main.c

String.o: String.c String.h
	$(CC) $(CFLAGS) -c String.c

LinkedList.o: LinkedList.c LinkedList.h
	$(CC) $(CFLAGS) -c LinkedList.c

Parser2.o: Parser2.c Parser2.h LinkedList.o String.o Config.h
	$(CC) $(CFLAGS) -c Parser2.c

Tokenizer2.o: Tokenizer2.c Tokenizer2.h LinkedList.o String.o Parser2.o Config.h
	$(CC) $(CFLAGS) -c Tokenizer2.c

AST.o: AST.c AST.h Tokenizer2.o Parser2.o Config.h
	$(CC) $(CFLAGS) -c AST.c

Macro.o: Macro.c Macro.h AST.o 
	$(CC) $(CFLAGS) -c Macro.c

Macros.o: Macros.c Macros.h Macro.o AST.o
	$(CC) $(CFLAGS) -c Macros.c

Interpret.o: Interpret.c Interpret.h String.o Macro.o Macros.o AST.o
	$(CC) $(CFLAGS) -c Interpret.c

clean:
	rm -f *.[ox]
