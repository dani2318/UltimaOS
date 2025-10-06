#pragma once
#include <stddef.h>
const char* strchr(const char* str, char chr);
char* strcpy(char* dst, const char* src);
int strlen(const char* str);
int strcmp(const char* a, const char* b);

wchar_t* UTF16ToCodepoint(wchar_t* string, int* codepoint);
char* CodepointToUTF16(int codepoint, char* stringOut);