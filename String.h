#ifndef STRING_INCLUDED
#define STRING_INCLUDED

#define CAPACITYSCALING 2


int lengthOfChars(char* str); 

typedef struct {
	char* data;
	unsigned int length;
	unsigned int capacity;
} String;

/* does not set length, just reallocates and sets new capacity */
void _resizeString(String* string, unsigned int length);
void _checkResizeString(String* string, unsigned int length);

void setString(String* string, unsigned int length, char* data);
void appendCharsToString(String* string, char* chars);
void appendCharToString(String* string, char char_);
void appendStringToString(String* a, String* b);
void prependCharsToString(String* string, char* chars);
void prependStringToString(String* a, String* b);
void intercalateString(String* string, char* inter);
// returns ' ' if invalid index
char charAtString(String string, unsigned int index);
void changeCharAtString(String* string, unsigned int index, char ch);

String mkString(char* data);
String copyString(String* string);
int getLength(String* string);
char* getData(String* string);
int checkEqualString(String a, String b);

/* frees original memmory of original string*/
void commandeerString(String* ship, String* boarding);
void freeString(String* cadaver);

void showString(String str);
void showStringVoid(void* string);
void showStringWithDetails(String str);

#endif
