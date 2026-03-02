#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "String.h"

#define CAPACITYSCALING 2


int lengthOfChars(char* str) {
	unsigned int result = 0; 
	while(str[result] != '\0') result++;
	return result;
}

// does not set length, just reallocates and sets new capacity
void _resizeString(String* string, unsigned int length) {
	string->capacity = length * CAPACITYSCALING;
	char* newData = malloc(sizeof(char) * (string->capacity + 1));
	if (string->data != NULL) {
		strcpy(newData, string->data);
		free(string->data);
	}
	string->data = newData;
}

void _checkResizeString(String* string, unsigned int length) {
	if (length == 0 && string->data == NULL) {
		_resizeString(string, 0);
	} else if (string->capacity < length) {
		_resizeString(string, length);
	}
}

void setString(String* string, unsigned int length, char* data) {
	_checkResizeString(string, length);
	for(unsigned int i = 0; i <= length; i++) {
		string->data[i] = data[i];
	}
	string->data[length] = '\0';
	string->length = length;	
}


void appendCharsToString(String* string, char* chars) {
	unsigned int newLength = lengthOfChars(chars) + string->length;
	_checkResizeString(string, newLength);
	strcpy(string->data + string->length, chars);
	//string->data[newLength] = '\0'; terminator is copied from strcpy
	string->length = newLength;
}

void appendCharToString(String* string, char char_) {
	_checkResizeString(string, string->length + 1);
	string->data[string->length] = char_;
	string->data[string->length + 1] = '\0';
	string->length++;
}

void appendStringToString(String* a, String* b) {
	appendCharsToString(a, b->data);
}

void prependCharsToString(String* string, char* chars) {
	unsigned int charsLength = lengthOfChars(chars);
	unsigned int newLength = charsLength + string->length;
	_checkResizeString(string, newLength);
	memmove(string->data + charsLength, string->data, string->length + 1);
	memcpy(string->data, chars, charsLength);
	//string->data[newLength] = '\0'; terminator is copied from memmove
	string->length = newLength;
}

void prependStringToString(String* a, String* b) {
	prependCharsToString(a, b->data);
}

void intercalateString(String* string, char* inter) {
	int multiplier = lengthOfChars(inter) + 1;
	_checkResizeString(string, string->length * multiplier);

	for(int i = string->length - 1; i >= 0; i--) {
		string->data[i * multiplier] = string->data[i];
		//copies the rest of the pattern in
		memcpy(string->data + (i * multiplier + 1), inter, multiplier - 1);
	}

	string->length = string->length * multiplier;
	string->data[string->length] = '\0';
}



// returns ' ' if invalid index
char charAtString(String string, unsigned int index) {
	if (index >= string.length || index < 0) return ' ';
	return string.data[index];
}

void changeCharAtString(String* string, unsigned int index, char ch) {
	if (index >= string->length || index < 0) return;
	string->data[index] = ch;
}

String mkString(char* data) {
	String result;
	result.data = NULL;
	result.capacity = 0;
	result.length = 0;

	setString(&result, lengthOfChars(data), data);
	return result;
}

String copyString(String* string) {
	return mkString(string->data); /*setString will copy memory*/
}

int getLength(String* string) {
	return string->length;
}

char* getData(String* string) {
	return string->data;
}

/* frees original memmory of original string*/
void commandeerString(String* ship, String* boarding) {
	freeString(ship);
	ship->data = boarding->data;
	ship->length = boarding->length;
	ship->capacity = boarding->capacity;
}

void freeString(String* cadaver) {
	if (cadaver != NULL) {
		if (cadaver->data != NULL) free(cadaver->data);
	}
}

void showString(String str) {
	printf("\"%s\"\n", str.data);
}

void showStringVoid(void* string) {
	showString(*(String*) string);
}

void showStringWithDetails(String str) {
	printf("String \"%s\" of size %d and capacity %d\n", str.data, str.length, str.capacity);
}



int _main() {
	String string = mkString("Hello ");
	printf("length of "" %d\n", lengthOfChars(""));
	//showStringWithDetails(nother);

	String nother = mkString("");

	appendStringToString(&string, &nother);
	showStringWithDetails(string);

	printf("Char at index 6: %c\n", charAtString(string, 6));
	changeCharAtString(&string, 6, 'B');
	showStringWithDetails(string);

	printf("Out of bounds checking: \n");
	showStringWithDetails(string);
	changeCharAtString(&string, string.length, '!');
	showStringWithDetails(string);

	printf("Intercalate: ");
	intercalateString(&string, " ");
	showStringWithDetails(string);

	printf("Prepend: ");
	prependCharsToString(&string, "Greeting: ");
	showStringWithDetails(string);

	return 1;
}

