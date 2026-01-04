#pragma once
#include <stdint.h>

// String length
inline size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

// String compare
inline int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// String copy
inline char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}

// String concatenate
inline char* strcat(char* dest, const char* src) {
    char* original_dest = dest;
    while (*dest) dest++;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}


// String compare (first n characters)
inline int strncmp(const char* str1, const char* str2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (str1[i] != str2[i] || str1[i] == '\0') {
            return (unsigned char)str1[i] - (unsigned char)str2[i];
        }
    }
    return 0;
}

inline char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;

    // Copy characters from src to dest
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }

    // If we haven't reached 'n', fill the rest with null bytes
    // This is required by the C standard
    for (; i < n; i++) {
        dest[i] = '\0';
    }

    return dest;
}